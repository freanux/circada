/*
 *  Circada.cpp
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

#include <Circada/Circada.hpp>
#include <Circada/Session.hpp>
#include <Circada/Global.hpp>

namespace Circada {

IrcClient::IrcClient(Configuration& config) : IrcServerSide(config), config(config) { }

IrcClient::~IrcClient() {
    ScopeMutex lock(&mtx);
    while (sessions.size()) {
        destroy_session_nolock(sessions[0]);
    }
}

Window *IrcClient::create_application_window(const std::string& name, const std::string& topic) {
    IrcServerSide *iss = static_cast<IrcServerSide *>(this);
    return iss->create_application_window(this, name, topic);
}
Window *IrcClient::get_application_window() {
    IrcServerSide *iss = static_cast<IrcServerSide *>(this);
    return iss->get_application_window();
}

Session *IrcClient::create_session(const SessionOptions& options) throw (CircadaException) {
    Session *s = 0;

    try {
        s = new Session(config, this, options);
        ScopeMutex lock(&mtx);
        sessions.push_back(s);
    } catch (const std::exception& e) {
        if (s) delete s;
        throw CircadaException(e.what());
    }

    return s;
}

void IrcClient::destroy_session(Session *s) throw (CircadaException) {
    ScopeMutex lock(&mtx);
    destroy_session_nolock(s);
}

size_t IrcClient::get_session_count() {
    ScopeMutex lock(&mtx);

    return sessions.size();
}

DCCHandle::List IrcClient::get_dcc_list() {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    return dcc_mgr->get_all_handles(0);
}

void IrcClient::dcc_accept(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    dcc_mgr->accept_dcc_handle(dcc, false);
}

void IrcClient::dcc_force(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    dcc_mgr->accept_dcc_handle(dcc, true);
}

void IrcClient::dcc_decline(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    dcc_mgr->decline_dcc_handle(dcc);
}

void IrcClient::dcc_abort(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    dcc_mgr->abort_dcc_handle(dcc);
}

void IrcClient::dcc_send_msg(DCCHandle dcc, const std::string& msg) throw (DCCInvalidHandleException, DCCOperationNotPermittedException, SocketException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    dcc_mgr->send_dcc_msg(dcc, msg);
}

DCCHandle IrcClient::get_dcc_handle_from_window(Window *w) throw (DCCOperationNotPermittedException) {
    SessionWindow *sw = static_cast<SessionWindow *>(w);
    if (!sw->is_dcc_window()) {
        throw DCCOperationNotPermittedException();
    }
    const DCC *dcc = sw->get_dcc();
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    return DCCHandle(*dcc_mgr, dcc);
}

Window *IrcClient::get_window_from_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCManager *dcc_mgr = static_cast<DCCManager *>(this);
    return dcc_mgr->get_window_from_dcc_handle(dcc);
}

DCCChatHandle IrcClient::get_chat_handle(DCCHandle dcc) throw (DCCOperationNotPermittedException) {
    if (dcc.get_type() != DCCTypeChat) {
        throw DCCOperationNotPermittedException();
    }
    return *static_cast<DCCChatHandle *>(&dcc);
}

DCCXferHandle IrcClient::get_xfer_handle(DCCHandle dcc) throw (DCCOperationNotPermittedException) {
    if (dcc.get_type() != DCCTypeXfer) {
        throw DCCOperationNotPermittedException();
    }
    return *static_cast<DCCXferHandle *>(&dcc);
}

void IrcClient::destroy_session_nolock(Session *s) throw (CircadaException) {
    for (Session::List::iterator it = sessions.begin(); it != sessions.end(); it++) {
        if ((*it) == s) {
            sessions.erase(it);
            s->disconnect();
            delete s;
            break;
        }
    }
}

void IrcClient::suicide(Session *s) {
    ScopeMutex lock(&mtx);
    for (Session::List::iterator it = sessions.begin(); it != sessions.end(); it++) {
        if ((*it) == s) {
            sessions.erase(it);
            Suicidal *suicial = reinterpret_cast<Suicidal *>(s);
            suicial->suicide();
            break;
        }
    }
}

Window *IrcClient::query(Session *s, const std::string& nick) throw (CircadaException) {
    SessionWindow *w = 0;
    if (s) {
        if (!s->is_channel(nick) && !s->is_that_me(nick)) {
            w = create_window(this, s, s, WindowTypePrivate, s->get_nick(), nick);
            w->set_topic(nick);
            change_topic(s, w, nick);
        }
    } else {
        throw CircadaException("No valid session specified.");
    }
    return w;
}

void IrcClient::unquery(Window *w) throw (CircadaException) {
    if (w) {
        SessionWindow *sw = static_cast<SessionWindow *>(w);
        if (sw->is_dcc_window()) {
            const DCC *dcc = sw->get_dcc();
            if (dcc) {
                destroy_dcc(dcc);
            }
            destroy_window(this, sw);
        } else if (sw->get_window_type() == WindowTypePrivate) {
            destroy_window(this, sw);
        } else {
            throw CircadaException("This window cannot be closed this way.");
        }
    } else {
        throw CircadaException("No valid window specified.");
    }
}

} /* namespace Circada */
