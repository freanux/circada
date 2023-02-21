/*
 *  Session.cpp
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

#include "Circada/Session.hpp"
#include "Circada/Circada.hpp"
#include "Circada/RFC2812.hpp"
#include "Circada/Internals.hpp"
#include "Circada/Utils.hpp"
#include "Circada/LineFetcher.hpp"
#include "Circada/Environment.hpp"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Circada {

    class SuicideThread : public Thread {
    private:
        SuicideThread(const SenderThread& rhs);
        SuicideThread& operator=(const SenderThread& rhs);

    public:
        SuicideThread(IrcServerSide& iss, Session *session, Socket& socket);
        virtual ~SuicideThread();

    private:
        IrcServerSide& iss;
        Session *session;
        Socket& socket;

        virtual void thread();
    };

    SenderThread::SenderThread(Socket *socket)
        : socket(socket), running(false)
    {
        running = true;
        if (!thread_start()) {
            running = false;
            throw SessionException("Starting thread failed.");
        }
    }

    SenderThread::~SenderThread() {
        running = false;
        io_sync_signal_event();
        thread_join();
    }

    void SenderThread::pump(const std::string& data) {
        ScopeMutex lock(&mtx);
        queue.push(data);
        io_sync_signal_event();
    }

    void SenderThread::send(const std::string& data) {
        try {
            socket->send(data + "\r\n");
        } catch (const SocketException& e) {
            throw SessionException(e.what());
        }
    }

    void SenderThread::thread() {
        while (running) {
            /* outgoing data */
            while (io_sync_wait_for_event()) {
                /* if outgoing queue has to be throttled,                     */
                /* that should happen here.                                   */
                if (!running) break;
                bool queue_empty = true;
                do {
                    /* critical section: pop request */
                    std::string data;
                    {
                        ScopeMutex lock(&mtx);
                        if (queue.empty()) break;
                        data = queue.front();
                        queue.pop();
                        queue_empty = queue.empty();
                    }
                    try {
                        send(data);
                    } catch (const SessionException& e) {
                        Queue empty;
                        std::swap(queue, empty);
                        break;
                    }
                } while (!queue_empty);
            }
        }

        Queue empty;
        std::swap(queue, empty);
    }

    Joinable::Joinable() { }

    Joinable::~Joinable() { }

    Suicidal::Suicidal() { }

    Suicidal::~Suicidal() { }

    Session::Session(Configuration& config, IrcServerSide& iss, const SessionOptions& options)
        : config(config), iss(iss), running(false), sender(0), server_window(0),
          recoder(iss.get_encodings()), lag_detector(false), last_tracked_lag(0), old_time(0),
          options(options), connection_state(ConnectionStateLogin), suiciding(false)
    {
        /* checks */
        if (!options.server.length()) throw SessionException("No server specified.");
        if (!options.port) throw SessionException("No port specified.");
        if (!options.nick.length()) throw SessionException("No nick specified.");
        if (!options.alternative_nick.length()) throw SessionException("No alternative nick specified.");
        if (!options.user.length()) throw SessionException("No user specified.");
        if (!options.real_name.length()) throw SessionException("No real name specified.");

        /* go */
        sender = new SenderThread(&socket);
    }

    Session::~Session() {
        iss.destroy_all_dccs_in_session(this);
        iss.destroy_all_windows_in_session(&iss, this);
        delete sender;
    }

    void Session::connect() {
        if (!running) {
            running = true;
            if (!thread_start()) {
                running = false;
                throw SessionException("Starting thread failed.");
            }
        } else {
            throw SessionException("Thread already started.");
        }
    }

    void Session::disconnect() {
        if (running) {
            struct timespec wait, remaining;
            wait.tv_sec = 0;
            int counter = 0;

            if (socket.is_connected()) {
                do {
                    try {
                        if (socket.get_error()) break;
                        send("QUIT :" + iss.get_quit_message());
                        /* wait for 5 seconds */
                        while (counter < 50) {
                            if (socket.get_error()) break;
                            counter++;
                            wait.tv_nsec = 100000000;
                            nanosleep(&wait, &remaining);
                        }
                    } catch (...) {
                        /* chomp */
                    }
                } while (false);
            }

            try {
                socket.close();
            } catch (...) {
                /* chomp */
            }

            running = false;
            thread_join();
        }
    }

    void Session::send(const std::string& data) {
        if (!running || !sender || socket.get_error()) {
            throw SessionException("Connection not established.");
        }

        /* if our pumped message is a privmsg or a notice, execute it,        */
        /* because the server will not send us our own msg.                   */
        Message m;
        m.parse(this, data, &recoder);
        m.nick = nick;
        m.its_me = true;
        if (iss.get_injection()) {
            if (is_equal(m.command, CMD_PRIVMSG) || is_equal(m.command, CMD_NOTICE)) {
                inject(m);
            }
        }

        /* special case: our nick is used by another user, now we try to change */
        /* our nick to the alternative one. if successfully chosen,             */
        /* the login can go on.                                                 */
        if (is_equal(m.command, CMD_NICK) && connection_state != ConnectionStateLoggedIn) {
            inject(m);
        }

        /* i think, we do not have to check the socket health status.         */
        /* the receive loop will be informed, if there is a socket error.     */
        sender->pump(data);
    }

    bool Session::is_that_me(const std::string& nick) {
        return is_equal(this->nick, nick);
    }

    DCCChatHandle Session::dcc_chat_offer(const std::string& nick) {
        /* try to find an earlier requested chat */
        DCCHandle::List handles = iss.get_all_handles(this);
        for (DCCHandle::List::iterator it = handles.begin(); it != handles.end(); it++) {
            DCCHandle& h = *it;
            if (is_equal(h.get_his_nick(), nick) && !h.is_running()) {
                iss.accept_dcc_handle(h, false);
                DCCChatHandle chat_handle = *static_cast<DCCChatHandle *>(&h);
                return chat_handle;
            }
        }


        /* create new chat request */
        DCC *dcc = 0;
        try {
            dcc = iss.create_chat_in(this, nick);
            char buffer[32];
            std::string req;
            req = "PRIVMSG " + nick + " :\01";
            req += "DCC CHAT chat ";
            sprintf(buffer, "%lu", static_cast<unsigned long>(ntohl(socket.get_address())));
            req += buffer;
            req += " ";
            sprintf(buffer, "%hu", dcc->get_dccio().get_port());
            req += buffer;
            req += "\01";
            send(req);
        } catch (const DCCManagerException& e) {
            throw SessionException(e.what());
        }

        return DCCChatHandle(iss, dcc);
    }

    DCCXferHandle Session::dcc_file_offer(const std::string& nick, const std::string& filename) {
        DCC *dcc = 0;
        try {
            std::string converted_filename;
            u32 filesize = 0;
            char buffer[32];
            dcc = iss.create_xfer_in(this, nick, filename, converted_filename, filesize);
            std::string req;
            req = "PRIVMSG " + nick + " :\01";
            req += "DCC SEND " + converted_filename + " ";
            sprintf(buffer, "%lu", static_cast<unsigned long>(ntohl(socket.get_address())));
            req += buffer;
            req += " ";
            sprintf(buffer, "%hu", dcc->get_dccio().get_port());
            req += buffer;
            req += " ";
            sprintf(buffer, "%u", filesize);
            req += buffer;
            req += "\01";
            send(req);
        } catch (const DCCManagerException& e) {
            throw SessionException(e.what());
        }

        return DCCXferHandle(iss, dcc);
    }

    /**************************************************************************
     * ServerNickPrefix
     **************************************************************************/
    const std::string& Session::get_nick_chars() const {
        return nick_prefixes_chars;
    }

    const std::string& Session::get_nick_symbols() const {
        return nick_prefixes_symbols;
    }

    /**************************************************************************
     * Joinable
     **************************************************************************/
    void Session::join() {
        thread_join();
    }

    /**************************************************************************
     * Suicidal
     **************************************************************************/
    void Session::suicide() {
        if (!suiciding) {
            suiciding = true;
            new SuicideThread(iss, this, socket);
        }
    }

    /**************************************************************************
     * Thread
     **************************************************************************/
    void Session::thread() {
        time_t new_time;
        int day_old, day_new;
        struct tm *tp;
        char time_buf[128];

        /* clear queue */
        Queue empty;
        std::swap(queue, empty);

        /* standard values */
        nicklen = DEFAULT_NICKLEN;
        nick_prefixes_chars = DEFAULT_NICK_CHARS;
        nick_prefixes_symbols = DEFAULT_NICK_SYMBOLS;
        channel_prefixes = DEFAULT_CHANNEL_PREFIXES;
        channel_modes_a = DEFAULT_CHANNEL_MODES_A;
        channel_modes_b = DEFAULT_CHANNEL_MODES_B;
        channel_modes_c = DEFAULT_CHANNEL_MODES_C;
        channel_modes_d = DEFAULT_CHANNEL_MODES_D;

        nick = options.nick;
        away = false;
        flags.clear();

        day_old = day_new = 0;
        new_time = 0;

        /* go */
        server_window = create_window(WindowTypeServer, (options.name.length() ? options.name : options.server));
        server_window->set_topic(get_server());
        iss.change_topic(this, server_window, server_window->get_topic());

        server_window->add_nick(nick, false);
        iss.add_nick(this, server_window, nick);
        iss.new_nicklist(this, server_window);

        try {
            /* socket and login */
            if (options.ca_file.length()) {
                socket.set_tls(options.ca_file);
            } else {
                socket.reset_tls();
            }
            socket.connect(options.server.c_str(), options.port);
            int flags = 0;
            flags += (options.user_invisible ? 8 : 0);
            flags += (options.receive_wallops ? 4 : 0);
            char flag_buf[16];
            sprintf(flag_buf, "%d", flags);
            connection_state = ConnectionStateLogin;
            sender->pump("USER " + options.user + " " + std::string(flag_buf) + " * :" + options.real_name);
            sender->pump("NICK " + nick);

            /* receive loop */
            LineFetcher fetcher("\r\n");
            while (running) {
                LineFetcher::Lines lines;

                /* incoming data */
                if (socket.activity(0, 100000)) {
                    fetcher.fetch(socket, lines);
                }

                /* grab current timestamp for new day and lag detector */
                new_time = time(0);

                /* signal new day? */
                tp = localtime(&new_time);
                strftime(time_buf, sizeof(time_buf), "%d", tp);
                day_new = std::atoi(time_buf);
                if (day_old == 0) {
                    day_old = day_new;
                }
                if (day_new != day_old) {
                    day_old = day_new;
                    Message m;
                    m.command = INT_DAY_CHANGE;
                    strftime(time_buf, sizeof(time_buf), "%A %B %d %G", tp);
                    m.params.push_back(time_buf);
                    m.make_midnight();
                    execute(m);
                }

                /* lag detector? */
                if (lag_detector) {
                    if (new_time - old_time >= 60) {
                        old_time = new_time;
                        struct timeval tim;
                        gettimeofday(&tim, 0);
                        double t = tim.tv_sec + (tim.tv_usec / 1000000.0);
                        sprintf(time_buf, "%f", t);
                        sender->pump("PING :\x1f" + std::string(time_buf));
                    }
                }

                /* execute incoming messages */
                execute_injected();
                for (LineFetcher::Lines::iterator it = lines.begin(); it != lines.end(); it++) {
                    Message m;
                    m.parse(this, *it, &recoder);
                    execute(m);
                }
            }
        } catch (const Exception& e) {
            if (!suiciding) {
                iss.connection_lost(this, e.what());
            }
        }

        /* delete all pending dccs, which are attached to the session */
        iss.destroy_all_dccs_in_session(this);

        /* detach current thread */
        thread_detach();
    }

    /**************************************************************************
     * private functions
     **************************************************************************/
    void Session::execute_injected() {
        while (true) {
            Message m;
            {
                ScopeMutex lock(&injection_mtx);
                if (queue.empty()) break;
                m = queue.front();
                queue.pop();
            }
            execute(m);
        }
    }

    void Session::execute(const Message& m) {
        ScopeMutex lock(&mtx);

        Command *cmd = commands;
        bool found = false;
        while (cmd->command) {
            if (is_equal(m.command.c_str(), cmd->command)) {
                found = true;
                if (cmd->function) {
                    (this->*cmd->function)(m);
                }
                if (cmd->window_name != Command::WindowNone) {
                    SessionWindow *w = 0;
                    if (cmd->window_name == Command::ServerWindow) {
                        w = server_window;
                    } else {
                        const std::string& channel_or_nick = m.params[cmd->window_name];
                        w = get_window(channel_or_nick);
                        if (!w) {
                            if (cmd->create_if_not_exists) {
                                w = create_window((is_channel(channel_or_nick) ? WindowTypeChannel : WindowTypePrivate), channel_or_nick);
                            } else {
                                w = server_window;
                            }
                        }
                    }
                    send_notification_with_noise(w, m);
                }
                break;
            }
            cmd++;
        }

        /* default -> send message to gui in server window */
        if (!found) {
            send_notification_with_noise(server_window, m);
        }
    }

    void Session::inject(Message& m) {
        ScopeMutex lock(&injection_mtx);
        m.injected = true;
        queue.push(m);
    }

    bool Session::set_capability(const std::string& cap, const Message& m, std::string& to_cap) {
        for (size_t i = 0; i < m.params.size() - 1; i++) {
            //if (m.params[i].find(cap + "=") != std::string::npos) {
            if (m.params[i].find(cap + "=") == 0) {
                to_cap = m.params[i].substr(cap.length() + 1);
                return true;
            }
        }

        return false;
    }

    bool Session::is_channel(const std::string& name) {
        if (name.length()) {
            return (channel_prefixes.find(name[0]) != std::string::npos);
        }

        return false;
    }

    std::string Session::get_flags() {
        return flags.get_flags();
    }

    const std::string& Session::get_nick() const {
        return nick;
    }

    const std::string& Session::get_server() const {
        return options.server;
    }

    bool Session::am_i_away() const {
        return away;
    }

    int Session::get_nicklen() const {
        return nicklen;
    }

    Window *Session::get_server_window() const {
        return server_window;
    }

    double Session::get_lag() const {
        return last_tracked_lag;
    }

    DCCHandle::List Session::get_dcc_list() {
        return iss.get_all_handles(this);
    }

    bool Session::process_ctcp(const Message& m) {
        /* hard coded ctcp requests:                                          */
        /* in near future, ctcp requests should be                            */
        /* implemented easily from caller class.                              */
        if (m.ctcp.length() && (!is_equal(m.nick, nick) || m.host.length())) {
            if (is_equal(m.ctcp, "ACTION")) {
                return false;
            } else if (is_equal(m.ctcp.c_str(), "VERSION")) {
                sender->pump("NOTICE " + m.nick + " :\x01" + "VERSION " + iss.get_project_name() + " " + iss.get_project_version() + " running on " + Environment::get_uname() + "\x01");
                send_ctcp_with_alert(m);
                return true;
            } else if (is_equal(m.ctcp, "PING")) {
                sender->pump("NOTICE " + m.nick + " :\x01" + "PONG " + m.params[1] + "\x01");
                send_ctcp_with_alert(m);
                return true;
            } else if (is_equal(m.ctcp.c_str(), "TIME")) {
                sender->pump("NOTICE " + m.nick + " :\x01" + "TIME " + get_current_time() + "\x01");
                send_ctcp_with_alert(m);
                return true;
            } else if (is_equal(m.ctcp.c_str(), "CLIENTINFO")) {
                sender->pump("NOTICE " + m.nick + " :\x01" + "CLIENTINFO ACTION CLIENTINFO DCC PING TIME VERSION" + "\x01");
                send_ctcp_with_alert(m);
                return true;
            } else if (is_equal(m.ctcp.c_str(), "DCC")) {
                process_dcc(m);
                return true;
            } else {
                Message& unsecured_m = const_cast<Message&>(m);
                unsecured_m.unhandled_ctcp_dcc = true;
                send_unhandled_ctcp_with_alert(m);
                return true;
            }
        }
        return false;
    }

    void Session::process_dcc(const Message& m) {
        std::string dcc_request = "???";

        if (m.pc >= 2) {
            const std::string& who = m.nick;
            std::string str = m.params[1];
            std::vector<std::string> params;
            size_t pos, pc;
            while (str.length()) {
                if ((pos = str.find(" ", 0)) != std::string::npos) {
                    params.push_back(str.substr(0, pos));
                    str = str.substr(pos + 1);
                } else {
                    params.push_back(str);
                    break;
                }
            }
            pc = params.size();
            if (pc) {
                dcc_request = params[0];
                if (is_equal(dcc_request.c_str(), "CHAT") && pc > 3) {
                    const std::string& chat_request = params[1];
                    if (is_equal(chat_request, "chat")) {
                        unsigned int addr = htonl(atoi(params[2].c_str()));
                        unsigned long port = atol(params[3].c_str());
                        DCC *dcc = 0;
                        try {
                            dcc = iss.create_chat_out(this, who, addr, port);
                            iss.dcc_incoming_chat_request(this, server_window, DCCChatHandle(iss, dcc));
                        } catch (const DCCException& e) {
                            iss.dcc_chat_failed(server_window, DCCChatHandle(iss, dcc), e.what());
                        }
                    } else {
                        iss.dcc_unhandled_chat_request(this, server_window, who, chat_request, m);
                    }
                    window_action_and_notify(server_window, WindowActionAlert);
                    return;
                } else if (is_equal(dcc_request.c_str(), "SEND") && pc > 3) {
                    const std::string& filename = params[1];
                    unsigned long addr = htonl(atol(params[2].c_str()));
                    unsigned short port = atoi(params[3].c_str());
                    unsigned long fsz = (pc > 4 ? atol(params[4].c_str()) : 0);
                    DCC *dcc = 0;
                    try {
                        dcc = iss.create_xfer_out(this, who, filename, fsz, addr, port);
                        iss.dcc_incoming_xfer_request(this, server_window, DCCXferHandle(iss, dcc));
                    } catch (const DCCException& e) {
                        iss.dcc_xfer_failed(server_window, DCCXferHandle(iss, dcc), e.what());
                    }
                    window_action_and_notify(server_window, WindowActionAlert);
                    return;
                } else if (is_equal(dcc_request.c_str(), "RESUME") && pc > 3) {
                    const std::string& filename = params[1];
                    unsigned short port = atoi(params[2].c_str());
                    u32 startpos = atoi(params[3].c_str());
                    DCC *dcc = 0;
                    try {
                        if (iss.set_resume_position(this, port, startpos, dcc)) {
                            char startpos_str[32];
                            sprintf(startpos_str, "%du", startpos);
                            std::string reply("PRIVMSG " + m.nick + " :\x01");
                            reply += "DCC ACCEPT " + filename + " " + params[2] + " ";
                            reply += startpos_str;
                            reply += "\x01";
                            sender->pump(reply);
                        }
                    } catch (const DCCManagerException& e) {
                        iss.dcc_mgr_failed(dcc, e.what());
                    }
                    return;
                } else if (is_equal(dcc_request.c_str(), "ACCEPT") && pc > 3) {
                    unsigned short port = atoi(params[2].c_str());
                    u32 startpos = atoi(params[3].c_str());
                    DCC *dcc = 0;
                    try {
                        if (iss.set_resume_position(this, port, startpos, dcc)) {
                            dcc->start();
                        }
                    } catch (const DCCManagerException& e) {
                        iss.dcc_mgr_failed(dcc, e.what());
                    }
                    return;
                }
            }
        }

        Message& unsecured_m = const_cast<Message&>(m);
        unsecured_m.unhandled_ctcp_dcc = true;
        iss.dcc_unhandled_request(this, server_window, dcc_request, m);
        window_action_and_notify(server_window, WindowActionAlert);
    }

    SessionWindow *Session::create_window(WindowType type, const std::string& name) {
        return iss.create_window(&iss, this, this, type, nick, name);
    }

    SessionWindow *Session::create_alert_window() {
        SessionWindow *w = create_window(WindowTypeAlerts, "notifications");
        w->set_topic("Notifications on " + get_server());
        return w;
    }

    void Session::destroy_window(SessionWindow *w) {
        iss.destroy_window(&iss, w);
    }

    SessionWindow *Session::get_window(const std::string& name) {
        return iss.get_window(this, name);
    }

    bool Session::remove_channel_wildcards(Message& m, std::string& name) {
        size_t sz = name.length();
        if (sz) {
            for (size_t i = 0; i < sz; i++) {
                if (nick_prefixes_symbols.find(name[i]) == std::string::npos) {
                    if (channel_prefixes.find(name[i]) != std::string::npos) {
                        m.op_notices = name.substr(0, i);
                        name = name.substr(i);
                        return true;
                    }
                    return false;
                }
            }
        }

        return false;
    }

    void Session::add_nick_prefix(Window *w, const Message& m) {
        Message& unsecured_m = const_cast<Message&>(m);
        unsecured_m.nick_with_prefix.clear();
        unsecured_m.nick_with_prefix += w->get_nick_flag(unsecured_m.nick);
        unsecured_m.nick_with_prefix += unsecured_m.nick;
    }

    void Session::send_privmsg_with_noise(SessionWindow *w, const Message& m, WindowAction action) {
        iss.message(this, w, m);
        window_action_and_notify(w, action);
    }

    void Session::send_privmsg_with_noise(SessionWindow *w, const Message& m) {
        send_privmsg_with_noise(w, m, WindowActionNoise);
    }

    void Session::send_notice_with_noise(SessionWindow *w, const Message& m, WindowAction action) {
        iss.notice(this, w, m);
        window_action_and_notify(w, action);
    }

    void Session::send_notice_with_noise(SessionWindow *w, const Message& m) {
        send_notice_with_noise(w, m, WindowActionNoise);
    }

    void Session::send_notification_with_noise(SessionWindow *w, const Message& m, WindowAction action) {
        iss.noise(this, w, m);
        window_action_and_notify(w, action);
    }

    void Session::send_notification_with_noise(SessionWindow *w, const Message& m) {
        send_notification_with_noise(w, m, WindowActionNoise);
    }

    void Session::send_ctcp_with_alert(const Message& m) {
        iss.ctcp_request(this, server_window, m);
        window_action_and_notify(server_window, WindowActionAlert);
    }

    void Session::send_unhandled_ctcp_with_alert(const Message& m) {
        iss.ctcp_unhandled_request(this, server_window, m);
        window_action_and_notify(server_window, WindowActionAlert);
    }

    void Session::window_action_and_notify(SessionWindow *w, WindowAction action) {
        if (w) {
            if (w->set_action(action)) {
                iss.window_action(this, w);
            }
        }
    }

    void Session::send_to_alert_window(SessionWindow *w, const Message& m) {
        if (w && w->get_window_type() == WindowTypeChannel) {
            SessionWindow *aw = create_alert_window();
            iss.alert(this, aw, w, m);
            window_action_and_notify(w, WindowActionAlert);
            window_action_and_notify(aw, WindowActionAlert);
        }
    }

    /**************************************************************************
     * SuicideThread
     **************************************************************************/
    SuicideThread::SuicideThread(IrcServerSide& iss, Session *session, Socket& socket)
        : iss(iss), session(session), socket(socket)
    {
        thread_start();
    }

    SuicideThread::~SuicideThread() { }

    void SuicideThread::thread() {
        thread_detach();

        struct timespec wait, remaining;
        wait.tv_sec = 0;
        int counter = 0;

        do {
            try {
                if (socket.get_error()) break;
                socket.send("QUIT :" + iss.get_quit_message() + "\r\n");
                /* wait for 5 seconds */
                while (counter < 50) {
                    if (socket.get_error()) break;
                    counter++;
                    wait.tv_nsec = 100000000;
                    nanosleep(&wait, &remaining);
                }
            } catch (...) {
                /* chomp */
            }
        } while (false);

        try {
            socket.close();
        } catch (...) {
            /* chomp */
        }
        session->join();
        delete session;
        delete this;
    }

} /* namespace Circada */
