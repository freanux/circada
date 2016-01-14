/*
 *  Events.hpp
 *
 *  Created by freanux on Mar 4, 2015
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

#ifndef _CIRCADA_EVENTS_HPP_
#define _CIRCADA_EVENTS_HPP_

#include "Circada/Session.hpp"
#include "Circada/Window.hpp"
#include "Circada/Message.hpp"

#include <string>

namespace Circada {

    class Events {
    public:
        Events() { }
        virtual ~Events() { }

        /* IMPLEMENTATION NOTICES:                                            */
        /*                                                                    */
        /* 1) if you implement these virtuals, bear in mind, that the         */
        /*    calls will come from different threads! you have to             */
        /*    synchronize it via a host message loop system.                  */
        /*                                                                    */
        /* 2) you shall not call s->query() in these virtuals.                */
        /*    you will run into a deadlock situation!                         */
        /*                                                                    */

        /* irc events */
        virtual void message(Session *s, Window *w, const Message& m) throw () { }
        virtual void notice(Session *s, Window *w, const Message& m) throw () { }
        virtual void noise(Session *s, Window *w, const Message& m) throw () { }
        virtual void ctcp_request(Session *s, Window *w, const Message& m) throw () { }
        virtual void ctcp_unhandled_request(Session *s, Window *w, const Message& m) throw () { }
        virtual void change_my_mode(Session *s, const std::string& mode) throw () { }
        virtual void open_window(Session *s, Window *w) throw () { }
        virtual void close_window(Session *s, Window *w) throw () { }
        virtual void window_action(Session *s, Window *w) throw () { }
        virtual void change_topic(Session *s, Window *w, const std::string& topic) throw () { }
        virtual void change_name(Session *s, Window *w, const std::string& name) throw () { }
        virtual void change_channel_mode(Session *s, Window *w, const std::string& mode) throw () { }
        virtual void new_nicklist(Session *s, Window *w) throw () { }
        virtual void add_nick(Session *s, Window *w, const std::string& nick) throw () { }
        virtual void remove_nick(Session *s, Window *w, const std::string& nick) throw () { }
        virtual void change_nick(Session *s, Window *w, const std::string& old_nick, const std::string& new_nick) throw () { }
        virtual void change_my_nick(Session *s, const std::string& old_nick, const std::string& new_nick) throw () { }
        virtual void change_nick_mode(Session *s, Window *w, const std::string& nick, const std::string& mode) throw () { }
        virtual void away(Session *s) throw () { }
        virtual void unaway(Session *s) throw () { }
        virtual void lag_update(Session *s, double lag_in_s) throw () { }
        virtual void connection_lost(Session *s, const std::string& reason) throw () { }

        /* dcc events, during running irc connection */
        virtual void dcc_offered_chat_timedout(Session *s, Window *w, const DCCChatHandle dcc, const std::string& reason) throw () { }
        virtual void dcc_incoming_chat_request(Session *s, Window *w, const DCCChatHandle dcc) throw () { }
        virtual void dcc_offered_xfer_timedout(Session *s, Window *w, const DCCXferHandle dcc, const std::string& reason) throw () { }
        virtual void dcc_incoming_xfer_request(Session *s, Window *w, const DCCXferHandle dcc) throw () { }
        virtual void dcc_unhandled_request(Session *s, Window *w, const std::string& dcc_request, const Message& m) throw () { }
        virtual void dcc_unhandled_chat_request(Session *s, Window *w, const std::string& nick, const std::string& chat_request, const Message& m) throw () { }

        /* events of detached dccs */
        virtual void dcc_chat_begins(Window *w, const DCCChatHandle dcc) throw () { }
        virtual void dcc_chat_ended(Window *w, const DCCChatHandle dcc, const std::string& reason) throw () { }
        virtual void dcc_chat_failed(Window *w, const DCCChatHandle dcc, const std::string& reason) throw () { }
        virtual void dcc_message(Window *w, const DCCChatHandle dcc, const std::string& ctcp, const std::string& msg) throw () { }

        virtual void dcc_xfer_begins(Window *w, const DCCXferHandle dcc) throw () { }
        virtual void dcc_xfer_ended(Window *w, const DCCXferHandle dcc) throw () { }
        virtual void dcc_xfer_failed(Window *w, const DCCXferHandle dcc, const std::string& reason) throw () { }
        virtual void dcc_send_progress(Window *w, const DCCXferHandle dcc) throw () { }
        virtual void dcc_receive_progress(Window *w, const DCCXferHandle dcc) throw () { }
    };

} /* namespace Circada */

#endif /* _CIRCADA_EVENTS_HPP_ */
