/*
 *  DCC.hpp
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

#ifndef _CIRCADA_DCC_HPP_
#define _CIRCADA_DCC_HPP_

#include "Circada/Exception.hpp"
#include "Circada/Types.hpp"
#include "Circada/Socket.hpp"
#include "Circada/Thread.hpp"
#include "Circada/LineFetcher.hpp"

#include <vector>
#include <string>

namespace Circada {

    class DCCException : public Exception {
    public:
        DCCException(const char *msg) : Exception(msg) { }
        DCCException(std::string msg) : Exception(msg) { }
    };

    class DCCTimedoutException : public DCCException {
    public:
        DCCTimedoutException() : DCCException("Timeout.") { }
    };

    class DCCChatStoppedException : public DCCException {
    public:
        DCCChatStoppedException(const char *msg) : DCCException(msg) { }
        DCCChatStoppedException(std::string msg) : DCCException(msg) { }
    };

    class DCCInvalidHandleException : public DCCException {
    public:
        DCCInvalidHandleException() : DCCException("Invalid DCC handle, it may be expired.") { }
    };

    class DCCOperationNotPermittedException : public DCCException {
    public:
        DCCOperationNotPermittedException() : DCCException("Operation is not permitted.") { }
    };

    enum DCCDirection {
        DCCDirectionIncoming,
        DCCDirectionOutgoing
    };

    enum DCCType {
        DCCTypeNone,
        DCCTypeChat,
        DCCTypeXfer
    };

    enum XferState {
        XferStateStart,
        XferStateTransfer
    };

    class DCCManager;
    class Session;

    class DCCIO {
    public:
        DCCIO(DCCDirection direction, unsigned long address, unsigned short port);
        virtual ~DCCIO();

        unsigned long get_address() const;
        unsigned short get_port() const;
        Socket& get_socket();
        DCCDirection get_direction() const;
        void set_timeout(int s);

        virtual void connect() = 0;
        void stop();

    protected:
        Socket socket;
        bool iorunning;
        int timeout;

        void set_port(unsigned short port);
        void set_address(unsigned long address);

    private:
        DCCDirection direction;
        unsigned int address;
        unsigned short port;
    };

    class DCC : public Thread {
    public:
        typedef std::vector<DCC *> List;

        DCC(Session *s, DCCManager& mgr, DCCIO& io, DCCType type, const std::string& nick);
        virtual ~DCC();

        void start();
        void stop();
        DCCIO& get_dccio() const;
        DCCType get_type() const;
        bool is_running() const;
        bool is_connected() const;
        void finished();
        const std::string& get_my_nick() const;
        const std::string& get_his_nick() const;
        Session *get_session() const;
        void reset_session();
        void set_his_nick(const std::string& new_nick);
        void set_my_nick(const std::string& new_nick);
        void set_will_be_killed();
        bool get_will_be_killed() const;

    protected:
        DCCManager& mgr;

    private:
        Session *s;
        DCCIO& io;
        DCCType type;
        std::string my_nick;
        std::string his_nick;
        bool running;
        bool connected;
        bool will_be_killed;

        virtual void process_or_idle() { }
        virtual void begin_handler() { }
        virtual void end_handler() { }
        virtual void thread();
    };

    class DCCIn : public DCCIO {
    public:
        DCCIn();
        virtual ~DCCIn();

    protected:
        virtual void connect();

    private:
        bool listening;
    };

    class DCCOut : public DCCIO {
    public:
        DCCOut(unsigned long address, unsigned short port);
        virtual ~DCCOut();

    protected:
        virtual void connect();
    };

    class DCCChat : public DCC {
    public:
        DCCChat(Session *s, DCCManager& mgr, DCCIO& io, const std::string& nick);
        virtual ~DCCChat();

        void send(const std::string& msg);

    private:
        LineFetcher fetcher;

        virtual void process_or_idle();
        virtual void begin_handler();
    };

    class DCCXfer : public DCC {
    public:
        DCCXfer(Session *s, DCCManager& mgr, DCCIO& io, const std::string& nick, const std::string& filename, u32 filesize);
        virtual ~DCCXfer();

        const std::string& get_filename() const;
        void set_resume_position(u32 startpos);
        u32 get_total_acknownledged() const;
        u32 get_filesize() const;

    protected:
        static const int BufferLength = 512; //65536;
        std::string filename;
        u32 filesize;
        u32 startpos;
        XferState state;
        FILE *f;
        bool successful;

        char buffer[BufferLength];
        time_t last_time;
        u32 total_acknowledged;

        virtual void begin_handler();
        virtual void end_handler();
    };

    class DCCChatIn : public DCCChat, public DCCIn {
    public:
        DCCChatIn(Session *s, DCCManager& mgr, const std::string& nick);
        virtual ~DCCChatIn();
    };

    class DCCChatOut : public DCCChat, public DCCOut {
    public:
        DCCChatOut(Session *s, DCCManager& mgr, const std::string& nick, unsigned long address, unsigned short port);
        virtual ~DCCChatOut();
    };

    class DCCXferIn : public DCCXfer, public DCCIn {
    public:
        DCCXferIn(Session *s, DCCManager& mgr, const std::string& nick, const std::string& filename, u32 filesize);
        virtual ~DCCXferIn();

        u32 get_total_sent() const;

    private:
        u32 total_sent;
        size_t total_size_read;

        virtual void process_or_idle();
    };

    class DCCXferOut : public DCCXfer, public DCCOut {
    public:
        DCCXferOut(Session *s, DCCManager& mgr, const std::string& nick, const std::string& filename, u32 filesize, unsigned long address, unsigned short port);
        virtual ~DCCXferOut();

    private:
        std::string part_filename;
        u32 last_acknowledged;

        virtual void process_or_idle();
    };

    /**************************************************************************
     * DCCHandle
     * this handle is public
     **************************************************************************/
    class DCCHandleBase {
    public:
        DCCHandleBase(DCCManager& dcc_mgr, const DCC *dcc);
        DCCHandleBase(const DCC *dcc);
        virtual ~DCCHandleBase();

        DCC *get_handle();

    protected:
        DCCManager& dcc_mgr;
        const DCC *dcc;

        void check_handle() const;
    };

    class DCCHandle : protected DCCHandleBase {
    public:
        typedef std::vector<DCCHandle> List;

        DCCHandle(DCCManager& dcc_mgr, const DCC *dcc);
        //DCCHandle operator=(DCCHandle rhs);
        virtual ~DCCHandle();

        const std::string& get_his_nick() const;
        const std::string& get_my_nick() const;
        DCCType get_type() const;
        DCCDirection get_direction() const;
        bool is_running() const;
        bool is_connected() const;
    };

    class DCCChatHandle : public DCCHandle {
    public:
        DCCChatHandle(DCCManager& dcc_mgr, const DCC *dcc);
        virtual ~DCCChatHandle();
    };

    class DCCXferHandle : public DCCHandle {
    public:
        DCCXferHandle(DCCManager& dcc_mgr, const DCC *dcc);
        virtual ~DCCXferHandle();

        const std::string& get_filename() const;
        unsigned int get_transferred_bytes() const;
        unsigned int get_filesize() const;
    };

} /* namespace Circada */

#endif /* _CIRCADA_DCC_HPP_ */
