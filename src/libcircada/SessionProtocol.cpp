/*
 *  SessionProtocol.cpp
 *
 *  Created by freanux on Feb 17, 2015
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
#include "Circada/Utils.hpp"
#include "Circada/Internals.hpp"
#include "Circada/Circada.hpp"

#include <sstream>
#include <cstdlib>
#include <cstring>

#include <iostream>

namespace Circada {

    Session::Command Session::commands[] = {
        /* commands */
        { CMD_PING, &Session::cmd_ping, Command::WindowNone, false },
        { CMD_PONG, &Session::cmd_pong, Command::WindowNone, false },
        { CMD_NICK, &Session::cmd_nick, Command::WindowNone, false },
        { CMD_JOIN, &Session::cmd_join, Command::WindowNone, false },
        { CMD_PART, &Session::cmd_part, Command::WindowNone, false },
        { CMD_KICK, &Session::cmd_kick, Command::WindowNone, false },
        { CMD_QUIT, &Session::cmd_quit, Command::WindowNone, false },
        { CMD_TOPIC, &Session::cmd_topic, Command::WindowNone, false },
        { CMD_PRIVMSG, &Session::cmd_privmsg, Command::WindowNone, false },
        { CMD_NOTICE, &Session::cmd_notice, Command::WindowNone, false },
        { CMD_MODE, &Session::cmd_mode, Command::WindowNone, false },
        { CMD_INVITE, &Session::cmd_invite, Command::WindowNone, false },

        /* replies */
        { RPL_WELCOME, &Session::rpl_welcome, Command::WindowNone, false },
        { RPL_PROTOCTL, &Session::rpl_protocol, Command::WindowNone, false },
        { RPL_TOPIC, &Session::rpl_topic, Command::WindowNone, false },
        { RPL_NAMREPLY, &Session::rpl_namreply, Command::WindowNone, false },
        { RPL_ENDOFNAMES, &Session::rpl_endofnames, Command::WindowNone, false },
        { RPL_CHANNELMODEIS, &Session::rpl_channelmodeis, Command::WindowNone, false },
        { RPL_NOWAWAY, &Session::rpl_nowaway, Command::WindowNone, false },
        { RPL_UNAWAY, &Session::rpl_unaway, Command::WindowNone, false },
        { RPL_TOPICWHOTIME, 0, 1, true },
        { RPL_WEBSITE, 0, 1, true },
        { RPL_INVITING, 0, 1, true },
        { RPL_CREATIONTIME, 0, 1, true },
        { RPL_AWAY, 0, 1, false },
        { RPL_BANLIST, 0, 1, true },
        { RPL_ENDOFBANLIST, 0, 1, true },
        { RPL_INVITELIST, 0, 1, true },
        { RPL_ENDOFINVITELIST, 0, 1, true },
        { RPL_EXCEPTLIST, 0, 1, true },
        { RPL_ENDOFEXCEPTLIST, 0, 1, true },

        /* errors */
        { ERR_NOSUCHNICK, 0, 1, false },
        { ERR_USERNOTINCHANNEL, 0, 1, false },
        { ERR_CHANOPRIVSNEEDED, 0, 1, false },
        { ERR_NICKNAMEINUSE, &Session::err_nicknameinuse, Command::WindowNone, false },

        /* internals */
        { INT_DAY_CHANGE, &Session::int_day_change, Command::WindowNone, false },

        /* eol */
        { 0, 0, 0, false }
    };

    void Session::cmd_ping(const Message& m) {
        sender->pump("PONG :" + m.params[0]);
    }

    void Session::cmd_pong(const Message& m) {
         if (m.pc > 1 && m.params[1].length() > 10 && m.params[1][0] == '\x1f') {
            struct timeval tim;
            double t_old = atof(m.params[1].substr(1).c_str());
            gettimeofday(&tim, 0);
            double t_new = tim.tv_sec + (tim.tv_usec / 1000000.0);
            last_tracked_lag = t_new - t_old;
            iss.lag_update(this, last_tracked_lag);
         } else {
            send_notification_with_noise(server_window, m);
         }
    }

    void Session::cmd_nick(const Message& m) {
        const std::string& new_nick = m.params[0];

        /* change nick in each window */
        if (m.nick.length()) {
            SessionWindow::List windows = iss.get_all_session_windows(this);
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                switch (w->get_window_type()) {
                    case WindowTypePrivate:
                        if (is_equal(w->get_name(), m.nick)) {
                            std::string old_nick = w->get_name();
                            w->set_name(new_nick);
                            iss.change_name(this, w, old_nick, new_nick);
                            std::string topic = new_nick + " (" + m.user_and_host + ")";
                            w->change_nick(m.nick, new_nick);
                            iss.change_nick(this, w, m.nick, new_nick);
                            if (w->set_topic(topic)) {
                                iss.change_topic(this, w, topic);
                            }
                            send_notification_with_noise(w, m);
                        } else if (m.nick == nick) {
                            w->change_nick(m.nick, new_nick);
                            iss.change_nick(this, w, m.nick, new_nick);
                            send_notification_with_noise(w, m);
                        }
                        break;

                    case WindowTypeDCC:
                        /* do nothing */
                        break;

                    default:
                        if (w->get_nick(m.nick)) {
                            w->change_nick(m.nick, new_nick);
                            iss.change_nick(this, w, m.nick, new_nick);
                            send_notification_with_noise(w, m);
                        }
                        break;
                }
            }
        }

        /* change my nick...?                                                 */
        /* if connection is not established in due to used nick by            */
        /* another user, change nick anyway.                                  */
        if (m.nick == nick || connection_state != ConnectionStateLoggedIn) {
            std::string old_nick = nick;
            nick = new_nick;
            iss.dcc_change_my_nick(this, nick);
            iss.change_my_nick(this, old_nick, new_nick);
        } else {
            iss.dcc_change_his_nick(this, m.nick, new_nick);
        }
    }

    void Session::cmd_join(const Message& m) {
        const std::string& channel = m.params[0];
        SessionWindow *w = create_window(WindowTypeChannel, channel);
        w->add_nick(m.nick, false);
        if (w->is_netsplit_over(m.nick)) {
            Message nsm;
            nsm.timestamp = m.timestamp;
            nsm.command = INT_NETSPLIT_OVER;
            nsm.pc = nsm.params.size();
            send_notification_with_noise(w, nsm);
        }
        iss.add_nick(this, w, m.nick);
        if (is_equal(m.nick, nick)) {
            /* request mode settings for this channel */
            sender->pump("MODE " + channel);
        }
        send_notification_with_noise(w, m);
    }

    void Session::cmd_part(const Message& m) {
        const std::string& channel = m.params[0];
        if (is_equal(m.nick, nick)) {
            destroy_window(get_window(channel));
        } else {
            SessionWindow *w = create_window(WindowTypeChannel, channel);
            w->remove_nick(m.nick);
            iss.remove_nick(this, w, m.nick);
            send_notification_with_noise(w, m);
        }
    }

    void Session::cmd_kick(const Message& m) {
        const std::string& channel = m.params[0];
        const std::string& kicked_nick = m.params[1];
        if (is_equal(kicked_nick, nick)) {
            destroy_window(get_window(channel));
            send_notification_with_noise(server_window, m);
        } else {
            SessionWindow *w = create_window(WindowTypeChannel, channel);
            w->remove_nick(kicked_nick);
            iss.remove_nick(this, w, kicked_nick);
            send_notification_with_noise(w, m);
        }
    }

    void Session::cmd_quit(const Message& m) {
        const std::string& quit_msg = m.params[0];
        Message nsm;
        struct timeval now;

        /* detect netsplit in quit message */
        bool ns = is_netsplit(quit_msg);
        if (ns) {
            gettimeofday(&now, NULL);
            nsm.timestamp = m.timestamp;
            nsm.command = INT_NETSPLIT;
            size_t npos = quit_msg.find(' ');
            if (npos == std::string::npos) {
                npos = 0;
            }
            nsm.params.push_back(quit_msg.substr(0, npos));     /* server left  */
            nsm.params.push_back(quit_msg.substr(npos + 1));    /* server right */
            nsm.pc = nsm.params.size();
        }

        /* walk thru all windows */
        SessionWindow::List windows = iss.get_all_session_windows(this);
        for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            SessionWindow *w = *it;
            switch (w->get_window_type()) {
                case WindowTypePrivate:
                    if (is_equal(w->get_name(), m.nick)) {
                        w->remove_nick(m.nick);
                        iss.remove_nick(this, w, m.nick);
                        send_notification_with_noise(w, m);
                    }
                    break;

                case WindowTypeDCC:
                    /* nothing */
                    break;

                default:
                    /* nick in channel window? */
                    if (w->get_nick(m.nick)) {
                        /* netsplit? */
                        if (ns) {
                            if (w->print_netsplit(quit_msg, now)) {
                                send_notification_with_noise(w, nsm);
                            }
                            w->add_netsplit_nick(quit_msg, m.user);
                        }
                        /* remove nick from list */
                        w->remove_nick(m.nick);
                        iss.remove_nick(this, w, m.nick);
                        send_notification_with_noise(w, m);
                    }
                    break;
            }
        }
    }

    void Session::cmd_topic(const Message& m) {
        const std::string& channel = m.params[0];
        const std::string& topic = m.params[1];
        SessionWindow *w = create_window(WindowTypeChannel, channel);
        w->set_topic(topic, true);
        iss.change_topic(this, w, topic);
        send_notification_with_noise(w, m);
    }

    void Session::cmd_privmsg(const Message& m) {
        const std::string& channel_or_nick = m.params[0];
        const std::string& text = m.params[1];

        if (is_channel(channel_or_nick)) {
            SessionWindow *w = get_window(channel_or_nick);
            if (!w) {
                /* that should never happen. */
                w = server_window;
            }
            add_nick_prefix(w, m);

            if (Nick::is_nick_in_text(nick, text)) {
                Message& unsecured_m = const_cast<Message&>(m);
                unsecured_m.to_me = true;
                if (w->set_action(WindowActionAlert)) {
                    iss.window_action(this, w);
                }
                send_to_alert_window(w, m);
            } else {
                if (w->set_action(WindowActionChat)) {
                    iss.window_action(this, w);
                }
            }
            if (!process_ctcp(m)) {
                send_privmsg_with_noise(w, m);
            }
        } else {
            if (!m.ctcp.length() || (m.ctcp.length() && m.ctcp == CTCP_ACTION)) {
                SessionWindow *w = 0;
                if (is_equal(nick, m.nick)) {
                    /* send to myself (dumb, but possible) */
                    w = create_window(WindowTypePrivate, channel_or_nick);
                    add_nick_prefix(w, m);
                    if (!w->get_topic().length()) {
                        std::string topic = channel_or_nick;
                        if (w->set_topic(topic, false)) {
                            iss.change_topic(this, w, topic);
                        }
                    }
                } else {
                    std::string topic = m.nick + " (" + m.user_and_host + ")";
                    w = create_window(WindowTypePrivate, m.nick);
                    add_nick_prefix(w, m);
                    if (w->set_topic(topic, false)) {
                        iss.change_topic(this, w, topic);
                    }
                }

                /* nick case change? */
                if (w && w->get_window_type() == WindowTypePrivate && !m.injected && w->get_name() != m.nick) {
                    std::string old_nick = w->get_name();
                    w->set_name(m.nick);
                    iss.change_name(this, w, old_nick, m.nick);
                }

                /* nick enters query? */
                if (w && !w->get_nick(m.nick)) {
                    w->add_nick(m.nick, false);
                    iss.add_nick(this, w, m.nick);
                }

                /* post */
                if (!process_ctcp(m)) {
                    send_privmsg_with_noise(w, m, WindowActionAlert);
                }
            } else {
                if (!process_ctcp(m)) {
                    send_privmsg_with_noise(server_window, m);
                }
            }
        }
    }

    void Session::cmd_notice(const Message& m) {
        Message& unsecured_m = const_cast<Message&>(m);
        std::string& channel = unsecured_m.params[0];
        if (remove_channel_wildcards(unsecured_m, channel)) {
            SessionWindow *w = get_window(channel);
            if (!w) {
                w = server_window;
            }
            send_notice_with_noise(w, unsecured_m, WindowActionAlert);
            send_to_alert_window(w, unsecured_m);
        } else {
            if (!unsecured_m.host.length()) {
                SessionWindow *w = get_window(channel);
                if (!w) w = server_window;
                send_notice_with_noise(w, unsecured_m);
            } else {
                SessionWindow *w = get_window(unsecured_m.nick);
                if (!w) w = server_window;
                if (w->get_window_type() == WindowTypePrivate) {
                    std::string topic = unsecured_m.nick + " (" + unsecured_m.user_and_host + ")";
                    if (w->set_topic(topic, false)) {
                        iss.change_topic(this, w, topic);
                    }
                    if (!unsecured_m.injected && w->get_name() != unsecured_m.nick) {
                        /* nick case change? */
                        std::string old_nick = w->get_name();
                        w->set_name(unsecured_m.nick);
                        iss.change_name(this, w, old_nick, unsecured_m.nick);
                    }
                }
                send_notice_with_noise(w, unsecured_m, WindowActionAlert);
                send_to_alert_window(w, unsecured_m);
            }
        }
    }

    void Session::cmd_mode(const Message& m) {
        const std::string& channel_or_nick = m.params[0];
        if (is_channel(channel_or_nick)) {
            std::string channel_modes_d;
            size_t parm_index = 2;
            bool add = true;
            bool is_last_channel_modes_d_set = false;
            bool last_channel_modes_d_flag = true;
            size_t sz = m.params[1].size();
            for (size_t i = 0; i < sz; i++) {
                char c = m.params[1][i];
                if (c == '+') {
                    add = true;
                } else if (c == '-') {
                    add = false;
                } else if (channel_modes_a.find(c) != std::string::npos) {
                    /* A: add/remove to list, has always a parameter */
                    const std::string& channel = m.params[0];
                    SessionWindow *w = create_window(WindowTypeChannel, channel);
                    std::string temp_flag = (add ? "+" : "-");
                    temp_flag += c;
                    Message tm = m;
                    tm.params.clear();
                    tm.command = (add ? INT_MODE_CHAN_ADD : INT_MODE_CHAN_REMOVE);
                    tm.params.push_back(channel);
                    tm.params.push_back(temp_flag);
                    if (parm_index < m.pc) tm.params.push_back(m.params[parm_index++]);
                    tm.pc = tm.params.size();
                    send_notification_with_noise(w, tm);
                } else if(channel_modes_b.find(c) != std::string::npos) {
                    /* B: change, has always a parameter */
                    const std::string& channel = m.params[0];
                    SessionWindow *w = create_window(WindowTypeChannel, channel);
                    std::string temp_flag = (add ? "+" : "-");
                    temp_flag += c;
                    Message tm = m;
                    tm.params.clear();
                    tm.command = INT_MODE_CHAN_CHANGE;
                    tm.params.push_back(channel);
                    tm.params.push_back(temp_flag);
                    if (parm_index < m.pc) tm.params.push_back(m.params[parm_index++]);
                    tm.pc = tm.params.size();
                    w->set_flags(temp_flag, false);
                    iss.change_channel_mode(this, w, temp_flag);
                    send_notification_with_noise(w, tm);
                } else if (channel_modes_c.find(c) != std::string::npos) {
                    /* C: change, has always a parameter if set */
                    const std::string& channel = m.params[0];
                    SessionWindow *w = create_window(WindowTypeChannel, channel);
                    std::string temp_flag = (add ? "+" : "-");
                    temp_flag += c;
                    Message tm = m;
                    tm.params.clear();
                    tm.command = (add ? INT_MODE_CHAN_SET : INT_MODE_CHAN_RESET);
                    tm.params.push_back(channel);
                    tm.params.push_back(temp_flag);
                    if (add && parm_index < m.pc) tm.params.push_back(m.params[parm_index++]);
                    tm.pc = tm.params.size();
                    send_notification_with_noise(w, tm);
                } else if (nick_prefixes_chars.find(c) != std::string::npos) {
                    /* change nick mode */
                    const std::string& channel = m.params[0];
                    SessionWindow *w = create_window(WindowTypeChannel, channel);
                    Nick *n = 0;
                    std::string temp_flag = (add ? "+" : "-");
                    temp_flag += c;
                    Message tm = m;
                    tm.params.clear();
                    tm.command = INT_MODE_NICK_CHANGE;
                    tm.params.push_back(m.params[0]);
                    tm.params.push_back(temp_flag);
                    if (parm_index < m.pc) {
                        const std::string& nick = m.params[parm_index++];
                        tm.params.push_back(nick);
                        n = w->get_nick(nick);
                    }
                    tm.pc = tm.params.size();
                    if (n) {
                        n->set_flags(temp_flag);
                        w->sort_nicks();
                        iss.change_nick_mode(this, w, n->get_nick(), temp_flag);

                    }
                    send_notification_with_noise(w, tm);
                } else {
                    /* D, all other flags: change, never has a parameter */
                    if (!is_last_channel_modes_d_set) {
                        channel_modes_d += (add ? "+" : "-");
                        is_last_channel_modes_d_set = true;
                        last_channel_modes_d_flag = add;
                    }

                    if (last_channel_modes_d_flag != add) {
                        channel_modes_d += (add ? "+" : "-");
                        last_channel_modes_d_flag = add;
                    }
                    channel_modes_d += c;
                }
            }

            /* aggregated channel mode changes (D, others) */
            if (channel_modes_d.length()) {
                const std::string& channel = m.params[0];
                SessionWindow *w = create_window(WindowTypeChannel, channel);
                Message tm = m;
                tm.params.clear();
                tm.command = INT_MODE_CHAN;
                tm.params.push_back(m.params[0]);
                tm.params.push_back(channel_modes_d);
                tm.pc = tm.params.size();
                w->set_flags(channel_modes_d, false);
                iss.change_channel_mode(this, w, channel_modes_d);
                send_notification_with_noise(w, tm);
            }
        } else {
            /* global user mode */
            const std::string& my_flags = m.params[1];
            flags.set_flags(my_flags);
            iss.change_my_mode(this, my_flags);
            send_notification_with_noise(server_window, m);
        }
    }

    void Session::cmd_invite(const Message& m) {
        std::string topic = m.nick + " (" + m.user_and_host + ")";
        SessionWindow *w = create_window(WindowTypePrivate, m.nick);
        if (w->set_topic(topic, false)) {
            iss.change_topic(this, w, topic);
        }
        send_notification_with_noise(w, m);
    }

    void Session::rpl_welcome(const Message& m) {
        old_time = time(0);
        lag_detector = true;
        connection_state = ConnectionStateLoggedIn;
        send_notification_with_noise(server_window, m);
    }

    void Session::rpl_protocol(const Message& m) {
        std::string temp;

        /* channel types/prefixes */
        set_capability(CAP_CHANTYPES, m, channel_prefixes);

        /* nick prefixes */
        if (set_capability(CAP_PREFIX, m, temp)) {
            size_t pos;
            if ((pos = temp.find(')')) != std::string::npos) {
                nick_prefixes_symbols = temp.substr(pos + 1);   /* eg: @%+ */
                nick_prefixes_chars = temp.substr(1, pos - 1);  /* eg: ohv */
            }
        }

        /* nick length */
        if (set_capability(CAP_NICKLEN, m, temp)) {
            nicklen = std::atoi(temp.c_str());
        }

        /* channel modes */
        if (set_capability(CAP_CHANMODES, m, temp)) {
            std::stringstream ss(temp);
            std::string item;
            if (std::getline(ss, item, ',')) channel_modes_a = item;
            if (std::getline(ss, item, ',')) channel_modes_b = item;
            if (std::getline(ss, item, ',')) channel_modes_c = item;
            if (std::getline(ss, item, ',')) channel_modes_d = item;
        }
        send_notification_with_noise(server_window, m);
    }

    void Session::rpl_topic(const Message& m) {
        const std::string& channel = m.params[1];
        const std::string& topic = m.params[2];

        SessionWindow *w = create_window(WindowTypeChannel, channel);
        w->set_topic(topic, true);
        iss.change_topic(this, w, topic);
    }

    void Session::rpl_namreply(const Message& m) {
        const std::string& channel = m.params[2];
        std::string nicks = m.params[3];
        SessionWindow *w = get_window(channel);

        if (w) {
            size_t p;
            std::string n;
            while (nicks.length()) {
                if ((p = nicks.find(' ')) != std::string::npos) {
                    n = nicks.substr(0, p);
                    nicks = nicks.substr(p + 1);
                } else {
                    n = nicks;
                    nicks = "";
                }
                w->add_nick(n, true);
            }
        }
    }

    void Session::rpl_endofnames(const Message& m) {
        const std::string& channel = m.params[1];

        if (channel == "*") {
            SessionWindow::List windows = iss.get_all_session_windows(this);
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                if (w->get_window_type() == WindowTypeChannel) {
                    w->sort_nicks();
                    iss.new_nicklist(this, w);
                }
            }
        } else {
            SessionWindow *w = get_window(channel);
            if (w) {
                w->sort_nicks();
                iss.new_nicklist(this, w);
            }
        }
    }

    void Session::rpl_channelmodeis(const Message& m) {
        const std::string& channel = m.params[1];
        const std::string& flags = m.params[2];
        SessionWindow *w = create_window(WindowTypeChannel, channel);
        w->set_flags(flags, true);
        iss.change_channel_mode(this, w, flags);
        send_notification_with_noise(w, m);
    }

    void Session::rpl_nowaway(const Message& m) {
        away = true;
        iss.away(this);
        send_notification_with_noise(server_window, m);
    }

    void Session::rpl_unaway(const Message& m) {
        away = false;
        iss.unaway(this);
        send_notification_with_noise(server_window, m);
    }

    void Session::err_nicknameinuse(const Message& m) {
        send_notification_with_noise(server_window, m);
        if (connection_state == ConnectionStateLogin) {
            connection_state = ConnectionStateNickFailed;
            send("NICK " + options.alternative_nick);
        }
    }

    void Session::int_day_change(const Message& m) {
        SessionWindow::List windows = iss.get_all_session_windows(this);
        for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            SessionWindow *w = *it;
            iss.noise(this, w, m); /* no action */
        }
    }

} /* namespace Circada */
