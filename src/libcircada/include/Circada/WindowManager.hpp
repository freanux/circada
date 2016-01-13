/*
 *  WindowManager.hpp
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

#ifndef _CIRCADA_WINDOWMANAGER_HPP_
#define _CIRCADA_WINDOWMANAGER_HPP_

//#include <Circada/CircadaException.hpp>
#include <Circada/Session.hpp>
#include <Circada/Window.hpp>
#include <Circada/Mutex.hpp>
#include <Circada/Events.hpp>

namespace Circada {

    class WindowManager {
    public:
        WindowManager();
        virtual ~WindowManager();

        SessionWindow::List get_all_session_windows(Session *s);
        SessionWindow *get_application_window();
        SessionWindow *get_window(Session *s, const std::string& name);
        SessionWindow *get_window(const DCC *dcc, const std::string& nick);
        SessionWindow *create_application_window(Events *evt, const std::string& name, const std::string& topic);
        SessionWindow *create_window(Events *evt, Session *s, ServerNickPrefix *snp, WindowType type, const std::string& my_nick, const std::string& name);
        SessionWindow *create_window(Events *evt, const DCC *dcc, const std::string& my_nick, const std::string& his_nick);
        void detach_window(const DCC *dcc);
        void destroy_window(Events *evt, SessionWindow *w);
        void destroy_window_nolock(Events *evt, SessionWindow *w);
        void destroy_all_windows_in_session(Events *evt, Session *s);
        void destroy_all_windows();

    private:
        SessionWindow::List windows;
        Mutex mtx;

        void destroy_window_nolock(SessionWindow *w);
    };

} /* namespace Circada */

#endif /* _CIRCADA_WINDOWMANAGER_HPP_ */
