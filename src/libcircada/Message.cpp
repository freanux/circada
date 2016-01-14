/*
 *  Message.cpp
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

#include "Circada/Message.hpp"
#include "Circada/Session.hpp"
#include "Circada/Utils.hpp"

#include <ctime>

namespace Circada {

    void Message::parse(Session *s, const std::string& message, const Recoder *recoder) {
        size_t pos;
        std::string str = message;
        int text_index = 0;

        /* initial setup */
        timestamp = get_now();
        session = s;
        unhandled_ctcp_dcc = false;

        /* copy line */
        line = message;

        /* grab prefix */
        if (str[0] == ':') {
            /* nick!user@server */
            if ((pos = str.find(" ", 0)) != std::string::npos) {
                user = str.substr(1, pos - 1);
                if (user[0] == ':') user = user.substr(1); /* is that necessary? */
                str = str.substr(pos + 1);
                if ((pos = user.find("!", 0)) != std::string::npos) {
                    host = user.substr(pos + 1);
                    nick = user.substr(0, pos);
                    nick_with_prefix = " " + nick;
                } else {
                    nick = user;
                    user = "";
                    host = "";
                }
            }
            user_and_host = host;
            if ((pos = host.find('@')) != std::string::npos) {
                user = host.substr(0, pos);
                host = host.substr(pos + 1);
            }
        }

        /* grab command */
        if ((pos = str.find(" ", 0)) != std::string::npos) {
            command = str.substr(0, pos);
            str = str.substr(pos + 1);
        }

        /* get parameters */
        while (str.length()) {
            if (str[0] == ':') {
                text_index = params.size();
                params.push_back(str.substr(1));
                break;
            }
            if ((pos = str.find(" ", 0)) != std::string::npos) {
                params.push_back(str.substr(0, pos));
                str = str.substr(pos + 1);
            } else {
                params.push_back(str);
                break;
            }
        }
        pc = params.size();

        /* ctcp */
        if (pc && params[pc - 1][0] == '\x01') {
            std::string& p = params[pc - 1];
            p = p.substr(1);
            if ((pos = p.find(' ')) != std::string::npos) {
                ctcp = p.substr(0, pos);
                p = p.substr(pos + 1);
                p = p.substr(0, p.length() - 1);
            } else {
                ctcp = p.substr(0, p.length() - 1);
                p = "";
            }
        } else {
            ctcp = "";
        }

        /* sent privmsg and notice are marked as injected */
        injected = false;

        /* it's a message from me? */
        its_me = (s ? s->is_that_me(nick) : false);

        /* this message is a PRIVMSG and has text for me */
        to_me = false;

        /* recode text */
        if (text_index && recoder) {
            recoder->recode(params[text_index]);
        }
    }

    void Message::make_midnight() {
        timestamp = "00:00:00";
    }

} /* namespace Circada */
