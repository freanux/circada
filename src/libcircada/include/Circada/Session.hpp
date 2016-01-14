/*
 *  Session.hpp
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

#ifndef _CIRCADA_SESSION_HPP_
#define _CIRCADA_SESSION_HPP_

#include "Circada/Exception.hpp"
#include "Circada/Configuration.hpp"
#include "Circada/Thread.hpp"
#include "Circada/Nick.hpp"
#include "Circada/IOSync.hpp"
#include "Circada/Socket.hpp"
#include "Circada/Mutex.hpp"
#include "Circada/Window.hpp"
#include "Circada/Message.hpp"
#include "Circada/Flags.hpp"
#include "Circada/Recoder.hpp"
#include "Circada/SessionOptions.hpp"
#include "Circada/DCC.hpp"

#include <vector>
#include <string>
#include <queue>

namespace Circada {

    class SessionException : public Exception {
    public:
        SessionException(const char *msg) : Exception(msg) { }
        SessionException(const std::string& msg) : Exception(msg) { }
    };

    class SenderThread : private Thread, private IOSync {
    private:
        SenderThread(const SenderThread& rhs) throw (SessionException);
        SenderThread& operator=(const SenderThread& rhs);

    public:
        SenderThread(Socket *socket) throw (SessionException);
        virtual ~SenderThread();

        void pump(const std::string& data);

    private:
        typedef std::queue<std::string> Queue;

        Socket *socket;
        bool running;
        Mutex mtx;
        Queue queue;

        void send(const std::string& data) throw (SessionException);
        virtual void thread();
    };

    class Joinable {
    public:
        Joinable();
        virtual ~Joinable();

        virtual void join() = 0;
    };

    class Suicidal {
    public:
        Suicidal();
        virtual ~Suicidal();

        virtual void suicide() = 0;
    };

    class IrcServerSide;

    class Session : private Thread, public ServerNickPrefix, private Joinable, public Suicidal {
    private:
        Session(const Session& rhs);
        Session& operator=(const Session& rhs);

    public:
        typedef std::vector<Session *> List;

        Session(Configuration& config, IrcServerSide *iss, const SessionOptions& options) throw (SessionException);
        virtual ~Session();

        /* use these functions to connect and disconnect                          */
        void connect() throw (SessionException);
        void disconnect() throw();

        /* runtime functions                                                      */
        /* use these to get some status, during an irc session                    */
        void send(const std::string& data) throw (SessionException);
        bool is_that_me(const std::string& nick);
        bool is_channel(const std::string& name);
        std::string get_flags();
        const std::string& get_nick() const;
        const std::string& get_server() const;
        bool am_i_away() const;
        int get_nicklen() const;
        Window *get_server_window() const;
        double get_lag() const;

        /* managing dcc requests                                                  */
        DCCChatHandle dcc_chat_offer(const std::string& nick) throw (SessionException);
        DCCXferHandle dcc_file_offer(const std::string& nick, const std::string& filename) throw (SessionException);
        DCCHandle::List get_dcc_list();

    protected:
        /* ServerNickPrefix */
        virtual const std::string& get_nick_chars() const;
        virtual const std::string& get_nick_symbols() const;

        /* JoiningSession */
        virtual void join();

        /* Suicidal */
        virtual void suicide();

    private:
        typedef std::queue<Message> Queue;
        typedef void (Session::*CommandFunction)(const Message &m);

        struct Command {
            static const int WindowNone = -2;
            static const int ServerWindow = -1;
            const char *command;
            CommandFunction function;
            int window_name;        /* specifies the params[] index, where the */
                                    /* channel name is, -1 for server window   */
            bool create_if_not_exists;
        };

        enum ConnectionState {
            ConnectionStateLogin = 0,
            ConnectionStateNickFailed,
            ConnectionStateLoggedIn
        };

        static Command commands[];

        /* session management */
        Configuration& config;
        IrcServerSide *iss;
        bool running;
        SenderThread *sender;
        SessionWindow *server_window;
        Recoder recoder;
        bool lag_detector;
        double last_tracked_lag;
        time_t old_time; /* for lag detector */
        SessionOptions options;
        ConnectionState connection_state;
        bool suiciding;

        /* server caps */
        std::string channel_prefixes;
        std::string nick_prefixes_chars;
        std::string nick_prefixes_symbols;
        int nicklen;
        std::string channel_modes_a;
        std::string channel_modes_b;
        std::string channel_modes_c;
        std::string channel_modes_d;

        /* session runtime */
        Queue queue;
        Mutex mtx;
        Mutex injection_mtx;
        Socket socket;

        /* nick management */
        std::string nick;
        Flags flags;
        bool away;

        virtual void thread();
        void execute_injected() throw (SessionException, SocketException);
        void execute(const Message& m) throw (SessionException, SocketException);
        void inject(Message& m);

        bool set_capability(const std::string& cap, const Message& m, std::string& to_cap);
        bool process_ctcp(const Message& m);
        void process_dcc(const Message& m);

        SessionWindow *create_window(WindowType type, const std::string& name);
        void destroy_window(SessionWindow *w);
        SessionWindow *get_window(const std::string& name);

        bool remove_channel_wildcards(Message& m, std::string& name);
        void add_nick_prefix(Window *w, const Message& m);

        void send_privmsg_with_noise(SessionWindow *w, const Message& m, WindowAction action);
        void send_privmsg_with_noise(SessionWindow *w, const Message& m);
        void send_notice_with_noise(SessionWindow *w, const Message& m, WindowAction action);
        void send_notice_with_noise(SessionWindow *w, const Message& m);
        void send_notification_with_noise(SessionWindow *w, const Message& m, WindowAction action);
        void send_notification_with_noise(SessionWindow *w, const Message& m);
        void send_ctcp_with_alert(const Message& m);
        void send_unhandled_ctcp_with_alert(const Message& m);
        void window_action_and_notify(SessionWindow *w, WindowAction action);

        /* protocol */
        void cmd_ping(const Message& m) throw (SessionException, SocketException);
        void cmd_pong(const Message& m) throw (SessionException, SocketException);
        void cmd_nick(const Message& m) throw (SessionException, SocketException);
        void cmd_join(const Message& m) throw (SessionException, SocketException);
        void cmd_part(const Message& m) throw (SessionException, SocketException);
        void cmd_kick(const Message& m) throw (SessionException, SocketException);
        void cmd_quit(const Message& m) throw (SessionException, SocketException);
        void cmd_topic(const Message& m) throw (SessionException, SocketException);
        void cmd_privmsg(const Message& m) throw (SessionException, SocketException);
        void cmd_notice(const Message& m) throw (SessionException, SocketException);
        void cmd_mode(const Message& m) throw (SessionException, SocketException);
        void cmd_invite(const Message& m) throw (SessionException, SocketException);

        void rpl_welcome(const Message& m) throw (SessionException, SocketException);
        void rpl_protocol(const Message& m) throw (SessionException, SocketException);
        void rpl_topic(const Message& m) throw (SessionException, SocketException);
        void rpl_namreply(const Message& m) throw (SessionException, SocketException);
        void rpl_endofnames(const Message& m) throw (SessionException, SocketException);
        void rpl_channelmodeis(const Message& m) throw (SessionException, SocketException);
        void rpl_nowaway(const Message& m) throw (SessionException, SocketException);
        void rpl_unaway(const Message& m) throw (SessionException, SocketException);

        void err_nicknameinuse(const Message& m) throw (SessionException, SocketException);

        void int_day_change(const Message& m) throw (SessionException, SocketException);
    };

} /* namespace Circada */

#endif /* _CIRCADA_SESSION_HPP_ */
