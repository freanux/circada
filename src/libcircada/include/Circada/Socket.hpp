/*
 *  Socket.hpp
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

#ifndef _CIRCADA_SOCKET_HPP_
#define _CIRCADA_SOCKET_HPP_

#include <Circada/Exception.hpp>

#include <vector>
#include <string>
#include <sys/time.h>
#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>

namespace Circada {

class SocketException : public Exception {
public:
	SocketException(const char *msg) : Exception(msg) { }
	SocketException(std::string msg) : Exception(msg) { }
};

class Socket {
private:
	Socket(const Socket& rhs);
	Socket operator=(const Socket& rhs);

public:
	Socket();
	virtual ~Socket();

	void set_tls(const std::string& ca_file) throw (SocketException);
	void reset_tls();
	void connect(const char *ip_address, unsigned short port) throw (SocketException);
	bool activity(time_t sec, suseconds_t usec) throw (SocketException);
	void listen(const char *address, unsigned short port, int backlog) throw (SocketException);
	void listen(const char *address, unsigned short port) throw (SocketException);
	void listen(unsigned short port, int backlog) throw (SocketException);
	void listen(unsigned short port) throw (SocketException);
	void accept(const Socket& socket) throw (SocketException);
	void close();

	size_t send(const char *buffer, size_t size) throw (SocketException);
	size_t send(const std::string& buffer) throw (SocketException);
	size_t receive(void *buffer, size_t size) throw (SocketException);
	bool get_error() const;
	bool is_connected() const;
	unsigned short get_port() throw (SocketException);
	unsigned long get_address() throw (SocketException);

private:
	static const int DefaultBacklog;

	int socket;
	bool connected;
	bool listening;
	bool error;
	bool tls;
	bool disconnecting;
	gnutls::client_session session;
	gnutls::certificate_credentials credentials;

	void check_states() throw (SocketException);
};

} /* namespace Circada */

#endif /* _CIRCADA_SOCKET_HPP_ */
