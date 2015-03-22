/*
 *  Socket.cpp
 *
 *  Created by freanux on Feb 15, 2015
 *  Copyright 2015 Circada Team. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Circada/Socket.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

namespace Circada {

const int Socket::DefaultBacklog = 5;

Socket::Socket() : socket(0), connected(false), listening(false), error(false), tls(false) { }

Socket::~Socket() {
    if (connected) {
        close();
    }
}

void Socket::set_tls(const std::string& ca_file) throw (SocketException) {
    check_states();
    try {
        this->tls = true;
        credentials.set_x509_trust_file(ca_file.c_str(), GNUTLS_X509_FMT_PEM);
        session.set_credentials(credentials);
        session.set_priority("NORMAL", 0);
    } catch (const std::exception& e) {
        this->tls = false;
        throw SocketException(e.what());
    }
}

void Socket::reset_tls() {
    if (!connected) {
        this->tls = false;
    }
}

void Socket::connect(const char *ip_address, unsigned short port) throw (SocketException) {
    unsigned long address;
    struct sockaddr_in server;
    struct hostent *host_info;
    int flags, rv;

    /* check states */
    check_states();

    /* create socket */
    if ((socket = ::socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw SocketException("Failed to create socket.");
    }

    /* convert string to Internet address */
    if ((address = inet_addr(ip_address)) != INADDR_NONE) {
        memcpy(&server.sin_addr, &address, sizeof(address));
    } else {
        host_info = gethostbyname(ip_address);
        if (!host_info) {
            ::close(socket);
            throw SocketException("Unknown server: " + std::string(ip_address));
        }
        memcpy(&server.sin_addr, host_info->h_addr, host_info->h_length);
    }
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    /* set to non blocking mode */
    flags = fcntl(socket, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(socket, F_SETFL, flags);

    /* open connection */
    disconnecting = false;
    rv = ::connect(socket, reinterpret_cast<struct sockaddr *>(&server), sizeof(server));
    if (rv < 0) {
        if (errno == EINPROGRESS) {
            int valopt;
            struct timeval tv;
            fd_set myset;
            socklen_t lon;
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            while (true) {
                FD_ZERO(&myset);
                FD_SET(socket, &myset);
                if (select(socket + 1, 0, &myset, 0, &tv) > 0) {
                   lon = sizeof(int);
                   getsockopt(socket, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon);
                   if (valopt) {
                       std::string err("Cannot connect to server: ");
                       err.append(strerror(valopt));
                       throw SocketException(err);
                   } else {
                       connected = true;
                       break;
                   }
                }
                usleep(50000);
                if (disconnecting) return;
            }
            if (!connected) {
                throw SocketException("Cannot connect to server, timed out.");
            }
        } else {
            std::string err("Cannot connect to server: ");
            err.append(strerror(errno));
            throw SocketException(err);
        }
    }

    /* set to blocking mode again */
    flags = fcntl(socket, F_GETFL, 0);
    flags &= (~O_NONBLOCK);
    fcntl(socket, F_SETFL, flags);

    /* tls? */
    if (tls) {
        bool failed = false;
        try {
            session.set_transport_ptr((gnutls_transport_ptr_t) (ptrdiff_t)socket);
            if (session.handshake() < 0) {
                failed = true;
            }
        } catch (const std::exception& e) {
            failed = true;
        }
        if (failed) {
            ::close(socket);
            connected = false;
            throw SocketException("TLS handshake failed.");
        }
    }
}

bool Socket::activity(time_t sec, suseconds_t usec) throw (SocketException) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(socket, &fds);

    timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    if (tls) {
        /* data in tls buffer?                   */
        /* this cannot be monitored via select() */
        if (session.check_pending() > 0) {
            return true;
        }
    }

    int rv = select(socket + 1, &fds, 0, 0, &timeout);

    if (rv < 0) {
        throw SocketException("Activity monitoring failed.");
    }

    if (listening && !connected) {
        return (FD_ISSET(socket, &fds) ? true : false);
    }

    if (FD_ISSET(socket, &fds)) {
        int n = 0;
        ioctl(socket, FIONREAD, &n);
        if (!n) {
            error = true;
            throw SocketException("Socket has closed.");
        }
        return true;
    }

    return false;
}

void Socket::listen(const char *address, unsigned short port, int backlog) throw (SocketException) {
    struct sockaddr_in server;
    int rv;

    const int yes = 1;

    /* check states */
    check_states();

    /* create socket */
    if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw SocketException("Cannot create socket.");
    }

    /* setup listener socket */
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    server.sin_family = AF_INET;
    if (address) {
        server.sin_addr.s_addr = inet_addr(address);
    } else {
        server.sin_addr.s_addr = htons(INADDR_ANY);
    }
    server.sin_port = htons(port);
    rv = bind(socket, reinterpret_cast<struct sockaddr *>(&server), sizeof(server));
    if (rv < 0) {
        ::close(socket);
        throw SocketException("Bind failed: " + std::string(strerror(errno)));
    }

    /* listen */
    listening = true;
    rv = ::listen(socket, backlog);
    if (rv < 0) {
        listening = connected = false;
        ::close(socket);
        throw SocketException("Listen failed: " + std::string(strerror(errno)));
    }
}

void Socket::listen(const char *address, unsigned short port) throw (SocketException) {
    listen(address, port, DefaultBacklog);
}

void Socket::listen(unsigned short port, int backlog) throw (SocketException) {
    listen(0, port, backlog);
}

void Socket::listen(unsigned short port) throw (SocketException) {
    listen(0, port, DefaultBacklog);
}

void Socket::accept(const Socket& socket) throw (SocketException) {
    struct sockaddr_in client;
    socklen_t client_len;

    /* check states */
    check_states();
    if (!socket.listening) {
        throw SocketException("Invalid listener socket.");
    }

    /* accept socket */
    client_len = sizeof(client_len);
    this->socket = ::accept(socket.socket, reinterpret_cast<struct sockaddr *>(&client), &client_len);
    if (this->socket < 0) {
        throw SocketException("Accept failed: " + std::string(strerror(errno)));
    }
    connected = true;
}

void Socket::close() {
    disconnecting = true;
    if (connected) {
        if (tls) {
            try {
                if (connected) {
                    session.bye(GNUTLS_SHUT_RDWR);
                }
            } catch (...) {
                /* chomp */
            }
        }
        ::shutdown(socket, SHUT_RDWR);
        ::close(socket);
        connected = listening = false;
    } else if (listening) {
        ::shutdown(socket, SHUT_RDWR);
        ::close(socket);
        connected = listening = false;
    }
}

size_t Socket::send(const char *buffer, size_t size) throw (SocketException) {
    if (error || !connected) {
        throw SocketException("Send failed, socket is closed.");
    }

    int rv;
    if (tls) {
        try {
            rv = session.send(buffer, size);
        } catch (const std::exception& e) {
            throw SocketException(e.what());
        }
    } else {
        rv = ::send(socket, buffer, size, 0);
    }
    if (rv < 0) {
        throw SocketException("Send failed: " + std::string(strerror(errno)));
    }

    return static_cast<size_t>(rv);
}

size_t Socket::send(const std::string& buffer) throw (SocketException) {
    return send(buffer.c_str(), buffer.length());
}

size_t Socket::receive(void *buffer, size_t size) throw (SocketException) {
    if (error || !connected) {
        throw SocketException("Receive failed, socket is closed.");
    }

    int rv;
    if (tls) {
        try {
            rv = session.recv(buffer, size);
        } catch (const std::exception& e) {
            throw SocketException(e.what());
        }
    } else {
        rv = recv(socket, buffer, size, 0);
    }
    if (rv < 0) {
        throw SocketException("Receive failed: " + std::string(strerror(errno)));
    }

    return static_cast<size_t>(rv);
}

bool Socket::get_error() const {
    return error;
}

bool Socket::is_connected() const {
    return connected;
}

unsigned short Socket::get_port() throw (SocketException) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket, (struct sockaddr *)&sin, &len) == -1) {
        throw SocketException("Can't get port: " + std::string(strerror(errno)));
    }

    return ntohs(sin.sin_port);
}

unsigned long Socket::get_address() throw (SocketException) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(socket, (struct sockaddr *)&sin, &len) == -1) {
        throw SocketException("Can't get port: " + std::string(strerror(errno)));
    }

    return sin.sin_addr.s_addr;
}

void Socket::check_states() throw (SocketException) {
    /* already connected? */
    if (connected) {
        throw SocketException("Socket already in use.");
    }
}

} /* namespace Circada */
