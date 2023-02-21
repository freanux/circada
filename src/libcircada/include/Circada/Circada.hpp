/*
 *  Circada.hpp
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

#ifndef _CIRCADA_CIRCADA_HPP_
#define _CIRCADA_CIRCADA_HPP_

#include "Circada/CircadaException.hpp"
#include "Circada/Configuration.hpp"
#include "Circada/IrcServerSide.hpp"
#include "Circada/IrcClientSide.hpp"
#include "Circada/Utils.hpp"
#include "Circada/Internals.hpp"
#include "Circada/Environment.hpp"
#include "Circada/Parser.hpp"

#include <vector>
#include <string>

namespace Circada {

    class IrcClient : private IrcServerSide, public IrcClientSide {
    private:
        IrcClient(const IrcClient& rhs);
        IrcClient& operator=(const IrcClient& rhs);

    public:
        IrcClient(Configuration& config);
        virtual ~IrcClient();

        Window *create_application_window(const std::string& name, const std::string& topic);
        Window *get_application_window();

        /* create and destroy sessions with these functions. */
        Session *create_session(const SessionOptions& options);
        void destroy_session(Session *s);
        size_t get_session_count();

        /* managing all dcc requests  */
        DCCHandle::List get_dcc_list();
        void dcc_accept(DCCHandle dcc);
        void dcc_force(DCCHandle dcc);
        void dcc_decline(DCCHandle dcc);
        void dcc_abort(DCCHandle dcc);
        void dcc_send_msg(DCCHandle dcc, const std::string& msg);
        DCCHandle get_dcc_handle_from_window(Window *w);
        Window *get_window_from_dcc_handle(DCCHandle dcc);
        DCCChatHandle get_chat_handle(DCCHandle dcc);
        DCCXferHandle get_xfer_handle(DCCHandle dcc);

        /* use suicide, to cut a connection. finally, the object will     */
        /* kill itself. this function acts like a fire and forget signal. */
        void suicide(Session *s);

        /* use this to query a nick/user */
        Window *query(Session *s, const std::string& nick);

        /* use this to unquery a chat or close a dcc session */
        void unquery(Window *w);

    protected:
        Mutex mtx;

    private:
        Configuration& config;
        Session::List sessions;

        void destroy_session_nolock(Session *s);
    };

} /* namespace Circada */

#endif /* _CIRCADA_CIRCADA_HPP_ */
