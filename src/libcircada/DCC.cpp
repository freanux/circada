/*
 *  DCC.cpp
 *
 *  Created by freanux on Feb 28, 2015
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

#include <Circada/DCC.hpp>
#include <Circada/DCCManager.hpp>
#include <Circada/Utils.hpp>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>

namespace Circada {

/*
 * IMPORTANT NOTES:
 * the in/out suffix (eg. XferIn, ChatOut) is from the point of view of the socket.
 * this means:
 *  -> in is listening, we are offering, the opposite has to connect.
 *  -> out is incoming, the opposite is offering, we have to connect.
 */

/******************************************************************************
 * DCCIO
 ******************************************************************************/
DCCIO::DCCIO(DCCDirection direction, unsigned long address, unsigned short port)
    : iorunning(true), timeout(300), direction(direction), address(address), port(port) { }

DCCIO::~DCCIO() { }

unsigned long DCCIO::get_address() const {
    return address;
}

unsigned short DCCIO::get_port() const {
    return port;
}

Socket& DCCIO::get_socket() {
    return socket;
}

DCCDirection DCCIO::get_direction() const {
    return direction;
}

void DCCIO::set_timeout(int s) {
    timeout = s;
}

void DCCIO::stop() {
    iorunning = false;
}

void DCCIO::set_port(unsigned short port) {
    this->port = port;
}

void DCCIO::set_address(unsigned long address) {
    this->address = address;
}

/******************************************************************************
 * DCC
 ******************************************************************************/
DCC::DCC(Session *s, DCCManager& mgr, DCCIO& io, DCCType type, const std::string& nick)
    : mgr(mgr), s(s), io(io), type(type), my_nick(s->get_nick()), his_nick(nick),
      running(false), connected(false), will_be_killed(false) { }

DCC::~DCC() { }

DCCIO& DCC::get_dccio() const {
    return io;
}

DCCType DCC::get_type() const {
    return type;
}

bool DCC::is_running() const {
    return running;
}

bool DCC::is_connected() const {
    return connected;
}

void DCC::finished() {
    running = false;

}
const std::string& DCC::get_my_nick() const {
    return my_nick;
}

const std::string& DCC::get_his_nick() const {
    return his_nick;
}

Session *DCC::get_session() const {
    return s;
}

void DCC::reset_session() {
    s = 0;
}

void DCC::set_his_nick(const std::string& new_nick) {
    this->his_nick = new_nick;
}

void DCC::set_my_nick(const std::string& new_nick) {
    this->my_nick = new_nick;
}

void DCC::set_will_be_killed() {
    will_be_killed = true;
}

bool DCC::get_will_be_killed() const {
    return will_be_killed;
}

void DCC::start() throw (DCCException) {
    if (!running) {
        running = true;
        if (!thread_start()) {
            throw DCCException("Starting thread failed.");
        }
    }
}

void DCC::stop() {
    if (running) {
        running = false;
        try {
            io.stop();
            io.get_socket().close();
        } catch (const DCCException& e) {
            /* chomp */
        }
        thread_join();
    }
}

void DCC::thread() {
    try {
        int to = atoi(mgr.get_configuration().get_value("", "dcc_timeout", "300").c_str());
        io.set_timeout(to);
        io.connect();
        connected = true;
        begin_handler();
        while (running) {
            process_or_idle();
        }
        connected = false;
        end_handler();
    } catch (const DCCTimedoutException& e) {
        connected = false;
        mgr.dcc_mgr_timedout(this, e.what());
    } catch (const DCCChatStoppedException& e) {
        connected = false;
        mgr.dcc_mgr_chat_ended(this, e.what());
    } catch (const DCCException& e) {
        connected = false;
        mgr.dcc_mgr_failed(this, e.what());
    }
}

/******************************************************************************
 * DCCIn
 ******************************************************************************/
DCCIn::DCCIn() throw (DCCException)
    : DCCIO(DCCDirectionIncoming, 0, 0), listening(false)
{
    socket.listen(0);
    try {
        set_port(socket.get_port());
        set_address(socket.get_address());
    } catch (const SocketException& e) {
        throw DCCException(e.what());
    }
}

DCCIn::~DCCIn() { }

void DCCIn::connect() throw (DCCException) {
    try {
        int timeout_counter = 0;
        while (iorunning) {
            if (timeout_counter >= timeout * 10) {
                throw DCCTimedoutException();
            }
            if (socket.activity(0, 100000)) {
                socket.accept(socket);
                break;
            }
            timeout_counter++;
        }
        if (!iorunning) {
            throw DCCException("Operation aborted.");
        }
    } catch (const SocketException& e) {
        throw DCCException(e.what());
    }
}

/******************************************************************************
 * DCCOut
 ******************************************************************************/
DCCOut::DCCOut(unsigned long address, unsigned short port)
    : DCCIO(DCCDirectionOutgoing, address, port) { }

DCCOut::~DCCOut() { }

void DCCOut::connect() throw (DCCException) {
    try {
        char addr[128];
        unsigned int address = get_address();
        inet_ntop(AF_INET, &address, addr, sizeof(addr));
        socket.connect(addr, get_port());
    } catch (const SocketException& e) {
        throw DCCException(e.what());
    }
}

/******************************************************************************
 * DCCChat
 ******************************************************************************/
DCCChat::DCCChat(Session *s, DCCManager& mgr, DCCIO& io, const std::string& nick)
    : DCC(s, mgr, io, DCCTypeChat, nick), fetcher("") { }

DCCChat::~DCCChat() { }

void DCCChat::begin_handler() {
    mgr.dcc_mgr_chat_begins(this);
}

void DCCChat::process_or_idle() throw (DCCException) {
    try {
        Socket& socket = get_dccio().get_socket();
        if (socket.activity(0, 100000)) {
            LineFetcher::Lines lines;
            fetcher.fetch(socket, lines);
            for (LineFetcher::Lines::iterator it = lines.begin(); it != lines.end(); it++) {
                std::string& line = *it;
                std::string ctcp;
                if (line.length() && line[0] == '\01') {
                    line = line.substr(1);
                    if (line.length() && line[line.length() - 1] == '\01') {
                        line = line.substr(0, line.length() - 1);
                        size_t pos = line.find(' ');
                        if (pos != std::string::npos) {
                            ctcp = line.substr(0, pos);
                            line = line.substr(pos + 1);
                        }
                    }
                }
                mgr.dcc_mgr_message(this, get_his_nick(), ctcp, line);
            }
        }
    } catch (const SocketException& e) {
        throw DCCChatStoppedException(e.what());
    }
}

void DCCChat::send(const std::string& msg) throw (DCCException) {
    try {
        Socket& socket = get_dccio().get_socket();
        socket.send(msg + "\r\n");
    } catch (const SocketException& e) {
        throw DCCException(e.what());
    }
}

/******************************************************************************
 * DCCXfer
 ******************************************************************************/
DCCXfer::DCCXfer(Session *s, DCCManager& mgr, DCCIO& io, const std::string& nick,
      const std::string& filename, u32 filesize)
    : DCC(s, mgr, io, DCCTypeXfer, nick), filename(filename), filesize(filesize),
      startpos(0), state(XferStateStart), f(0), successful(false) { }

DCCXfer::~DCCXfer() {
    if (f) {
        fclose(f);
    }
}

void DCCXfer::begin_handler() {
    mgr.dcc_mgr_xfer_begins(this);
}

void DCCXfer::end_handler() {
    if (successful) {
        mgr.dcc_mgr_xfer_ended(this);
    }
}

const std::string& DCCXfer::get_filename() const {
    return filename;
}

void DCCXfer::set_resume_position(u32 startpos) {
    this->startpos = startpos;
}

u32 DCCXfer::get_total_acknownledged() const {
    return total_acknowledged;
}

u32 DCCXfer::get_filesize() const {
    return filesize;
}

/******************************************************************************
 * DCCChatIn
 ******************************************************************************/
DCCChatIn::DCCChatIn(Session *s, DCCManager& mgr, const std::string& nick)
    : DCCChat(s, mgr, static_cast<DCCIO&>(*this), nick), DCCIn() { }

DCCChatIn::~DCCChatIn() { }

/******************************************************************************
 * DCCChatOut
 ******************************************************************************/
DCCChatOut::DCCChatOut(Session *s, DCCManager& mgr, const std::string& nick,
      unsigned long address, unsigned short port)
    : DCCChat(s, mgr, static_cast<DCCIO&>(*this), nick), DCCOut(address, port) { }

DCCChatOut::~DCCChatOut() { }

/******************************************************************************
 * DCCXferIn
 ******************************************************************************/
DCCXferIn::DCCXferIn(Session *s, DCCManager& mgr, const std::string& nick, const std::string& filename, u32 filesize)
    : DCCXfer(s, mgr, static_cast<DCCIO&>(*this), nick, filename, filesize), DCCIn() { }

DCCXferIn::~DCCXferIn() { }

u32 DCCXferIn::get_total_sent() const {
    return total_sent;
}

void DCCXferIn::process_or_idle() throw (DCCException) {
    try {
        switch (state) {
            case XferStateStart:
            {
                f = fopen(filename.c_str(), "rb");
                if (!f) {
                    throw DCCException("Cannot open file for reading: " + filename);
                }
                total_sent = startpos;
                total_size_read = 0;
                mgr.dcc_mgr_send_progress(this, total_sent, filesize);
                last_time = time(0);
                state = XferStateTransfer;
                break;
            }

            case XferStateTransfer:
            {
                /* send chunk to client */
                if (total_sent < filesize) {
                    fseek(f, total_sent, SEEK_SET);
                    size_t sz_to_send = fread(buffer, 1, BufferLength, f);
                    total_sent += static_cast<uint32_t>(socket.send(buffer, sz_to_send));
                }
                /* get total received */
                while (socket.activity(0, 1)) {
                    char *sz_buf = reinterpret_cast<char *>(&total_acknowledged);
                    void *v = static_cast<void *>(&sz_buf[total_size_read]);
                    total_size_read = socket.receive(v, sizeof(u32) - total_size_read);
                    /* finished */
                    if (total_size_read >= sizeof(u32)) {
                        total_size_read = 0;
                        total_acknowledged = ntohl(total_acknowledged);
                        if (total_acknowledged >= filesize) {
                            successful = true;
                            mgr.dcc_mgr_send_progress(this, total_acknowledged, filesize);
                            finished();
                        } else {
                            time_t current_time = time(0);
                            if (current_time != last_time) {
                                last_time = current_time;
                                mgr.dcc_mgr_send_progress(this, total_acknowledged, filesize);
                            }
                        }
                    }
                }
                break;
            }
        }
    } catch (const Exception& e) {
        throw DCCException(e.what());
    }
}

/******************************************************************************
 * DCCXferOut
 ******************************************************************************/
DCCXferOut::DCCXferOut(Session *s, DCCManager& mgr, const std::string& nick,
      const std::string& filename, u32 filesize, unsigned long address, unsigned short port)
    : DCCXfer(s, mgr, static_cast<DCCIO&>(*this), nick, filename, filesize), DCCOut(address, port) { }

DCCXferOut::~DCCXferOut() { }

void DCCXferOut::process_or_idle() throw (DCCException) {
    try {
        switch (state) {
            case XferStateStart:
            {
                std::string outfile = mgr.get_storage(filename);
                part_filename = mgr.get_storage(mgr.get_part_filename(filename));
                last_acknowledged = 0;

                /* rename old existing file, if exists */
                if (!file_exists(part_filename)) {
                    if (file_exists(outfile)) {
                        char appbuf[16];
                        int appendix = 1;
                        std::string outfile_to;
                        while (true) {
                            sprintf(appbuf, "%d", appendix);
                            outfile_to = outfile + ".";
                            outfile_to += appbuf;
                            if (!file_exists(outfile_to)) {
                                rename(outfile.c_str(), outfile_to.c_str());
                                break;
                            }
                            appendix++;
                            if (appendix > 99999) {
                                throw DCCException("Hey. This is absolutely mad, isn't it? Please clean up your transfer directory.");
                            }
                        }
                    }
                }

                /* create partial file */
                f = fopen(part_filename.c_str(), "wb");
                if (!f) {
                    throw DCCException("Cannot create partial file: " + part_filename +" (" + std::string(strerror(errno)) + ")");
                }
                fclose(f);
                f = 0;

                /* open output file in append or in read/update mode */
                if (!file_exists(outfile)) {
                    f = fopen(outfile.c_str(), "ab");
                } else {
                    f = fopen(outfile.c_str(), "r+b");
                }
                if (!f) {
                    throw DCCException("Cannot open file for writing: " + outfile +" (" + std::string(strerror(errno)) + ")");
                }
                total_acknowledged = startpos;
                fseek(f, total_acknowledged, SEEK_SET);
                mgr.dcc_mgr_receive_progress(this, total_acknowledged, filesize);
                last_time = time(0);
                state = XferStateTransfer;
                break;
            }

            case XferStateTransfer:
            {
                /* get total received */
                if (socket.activity(0, 100000)) {
                    size_t sz = socket.receive(buffer, BufferLength);
                    if (filesize && total_acknowledged < filesize) {
                        total_acknowledged += fwrite(buffer, 1, sz, f);
                        *buffer = htonl(total_acknowledged);
                        socket.send(buffer, sizeof(u32));
                    }
                }
                time_t current_time = time(0);
                if (current_time != last_time && last_acknowledged != total_acknowledged) {
                    last_acknowledged = total_acknowledged;
                    last_time = current_time;
                    mgr.dcc_mgr_receive_progress(this, total_acknowledged, filesize);
                }
                break;
            }
        }
    } catch (const SocketException& e) {
        /* sender closes socket, if file successfully transferred */
        if ((filesize && total_acknowledged >= filesize) || !filesize) {
            successful = true;
            remove(part_filename.c_str());
            if (last_acknowledged != total_acknowledged) {
                mgr.dcc_mgr_receive_progress(this, total_acknowledged, filesize);
            }
            finished();
        } else {
            throw DCCException(e.what());
        }
    } catch (const Exception& e) {
        throw DCCException(e.what());
    }
}

/******************************************************************************
 * DCCHandle
 * this handle is public
 ******************************************************************************/
DCCHandleBase::DCCHandleBase(DCCManager& dcc_mgr, const DCC *dcc) : dcc_mgr(dcc_mgr), dcc(dcc) { }

DCCHandleBase::~DCCHandleBase() { }

DCC *DCCHandleBase::get_handle() {
    return const_cast<DCC *>(dcc);
}

void DCCHandleBase::check_handle() const throw (DCCInvalidHandleException) {
    if (!dcc_mgr.is_handle_valid_nolock(dcc)) {
        throw DCCInvalidHandleException();
    }
}

DCCHandle::DCCHandle(DCCManager& dcc_mgr, const DCC *dcc) : DCCHandleBase(dcc_mgr, dcc) { }

DCCHandle::~DCCHandle() { }

DCCHandle DCCHandle::operator=(DCCHandle rhs) {
    return rhs;
}

const std::string& DCCHandle::get_his_nick() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->get_his_nick();
}

const std::string& DCCHandle::get_my_nick() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->get_my_nick();
}

DCCType DCCHandle::get_type() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->get_type();
}

DCCDirection DCCHandle::get_direction() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->get_dccio().get_direction();
}

bool DCCHandle::is_running() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->is_running();
}

bool DCCHandle::is_connected() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    return dcc->is_connected();
}

DCCChatHandle::DCCChatHandle(DCCManager& dcc_mgr, const DCC *dcc) : DCCHandle(dcc_mgr, dcc) { }

DCCChatHandle::~DCCChatHandle() { }

DCCXferHandle::DCCXferHandle(DCCManager& dcc_mgr, const DCC *dcc) : DCCHandle(dcc_mgr, dcc) { }

DCCXferHandle::~DCCXferHandle() { }

const std::string& DCCXferHandle::get_filename() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    const DCCXfer *dcc_xfer = static_cast<const DCCXfer *>(dcc);
    return dcc_xfer->get_filename();
}

unsigned int DCCXferHandle::get_transferred_bytes() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    unsigned int sz = 0;
    if (dcc->get_dccio().get_direction() == DCCDirectionIncoming) {
        const DCCXferIn *dcc_xfer = static_cast<const DCCXferIn *>(dcc);
        sz = static_cast<unsigned int>(dcc_xfer->get_total_sent());
    } else {
    const DCCXfer *dcc_xfer = static_cast<const DCCXfer *>(dcc);
    sz = static_cast<unsigned int>(dcc_xfer->get_total_acknownledged());
    }

    return sz;
}

unsigned int DCCXferHandle::get_filesize() const throw (DCCInvalidHandleException) {
    ScopeMutex lock(&dcc_mgr.get_mutex());
    check_handle();
    const DCCXfer *dcc_xfer = static_cast<const DCCXfer *>(dcc);
    return static_cast<unsigned int>(dcc_xfer->get_filesize());
}

} /* namespace Circada */
