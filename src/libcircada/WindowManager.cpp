/*
 *  WindowManager.cpp
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

#include "Circada/WindowManager.hpp"
#include "Circada/Utils.hpp"

namespace Circada {

    WindowManager::WindowManager() { }

    WindowManager::~WindowManager() {
        destroy_all_windows();
    }

    SessionWindow::List WindowManager::get_all_session_windows(Session *s) {
        SessionWindow::List session_windows;

        ScopeMutex lock(&mtx);
        if (s) {
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                if (w->get_session() == s) {
                    session_windows.push_back(w);
                }
            }
        }

        return session_windows;
    }

    SessionWindow *WindowManager::get_application_window() {
        ScopeMutex lock(&mtx);

        SessionWindow *w = 0;
        for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            SessionWindow *sw = *it;
            if (!sw->get_session()) {
                w = sw;
                break;
            }
        }

        return w;
    }

    SessionWindow *WindowManager::get_window(Session *s, const std::string& name) {
        ScopeMutex lock(&mtx);

        if (s) {
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                if (w->get_session() == s && is_equal(w->get_plain_name(), name) && !w->get_dcc()) {
                    return w;
                }
            }
        }

        return 0;
    }

    SessionWindow *WindowManager::get_window(const DCC *dcc, const std::string& nick) {
        ScopeMutex lock(&mtx);

        if (dcc) {
            SessionWindow::List::iterator it;

            /* find corresponding active dcc window */
            for (it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                if (w->get_dcc() == dcc) {
                    return w;
                }
            }

            /* then look for an old dcc window with this nick */
            if (nick.length()) {
                for (it = windows.begin(); it != windows.end(); it++) {
                    SessionWindow *w = *it;
                    if (w->is_dcc_window() && is_equal(w->get_plain_name(), nick) && w->get_ddc_type() == dcc->get_type()) {
                        w->set_dcc(dcc);
                        return w;
                    }
                }
            }
        }

        return 0;
    }

    SessionWindow *WindowManager::create_application_window(Events *evt, const std::string& name, const std::string& topic) {
        SessionWindow *w = get_application_window();

        if (!w) {
            ScopeMutex lock(&mtx);
            w = new SessionWindow(name, topic);
            windows.push_back(w);
            evt->open_window(0, w);
            evt->window_action(0, w);
        }

        return 0;
    }

    SessionWindow *WindowManager::create_window(Events *evt, Session *s, ServerNickPrefix *snp, WindowType type, const std::string& my_nick, const std::string& name) {
        SessionWindow *w = 0;

        if (s) {
            w = get_window(s, name);
            if (!w) {
                ScopeMutex lock(&mtx);

                w = new SessionWindow(s, type, name, snp);
                windows.push_back(w);
                evt->open_window(s, w);
                evt->window_action(s, w);
                if (type == WindowTypePrivate) {
                    w->add_nick(my_nick, false);
                    w->add_nick(name, false);
                    evt->new_nicklist(s, w);
                }
            }
        }

        return w;
    }

    SessionWindow *WindowManager::create_window(Events *evt, const DCC *dcc, const std::string& my_nick, const std::string& his_nick) {
        SessionWindow *w = get_window(dcc, his_nick);

        if (!w) {
            ScopeMutex lock(&mtx);
            w = new SessionWindow(0, dcc, his_nick, 0);
            windows.push_back(w);
            evt->open_window(0, w);
            evt->window_action(0, w);
            w->add_nick(my_nick, false);
            w->add_nick(his_nick, false);
            evt->new_nicklist(0, w);
        }

        return w;
    }

    void WindowManager::detach_window(const DCC *dcc) {
        ScopeMutex lock(&mtx);

        if (dcc) {
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                SessionWindow *w = *it;
                if (w->get_dcc() == dcc) {
                    w->reset_dcc_handler();
                    break;
                }
            }
        }
    }

    void WindowManager::destroy_window(Events *evt, SessionWindow *w) {
        ScopeMutex lock(&mtx);
        destroy_window_nolock(evt, w);
    }

    void WindowManager::destroy_window_nolock(Events *evt, SessionWindow *w) {
        if (w) {
            for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                if (*it == w) {
                    if (evt) {
                        evt->close_window(w->get_session(), w);
                    }
                    windows.erase(it);
                    delete w;
                    break;
                }
            }
        }
    }

    void WindowManager::destroy_all_windows_in_session(Events *evt, Session *s) {
        if (s) {
            ScopeMutex lock(&mtx);
            bool found;
            do {
                found = false;
                for (SessionWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
                    SessionWindow *w = *it;
                    if (w->get_session() == s) {
                        found = true;
                        destroy_window_nolock(evt, w);
                        break;
                    }
                }
            } while (found);
        }
    }

    void WindowManager::destroy_all_windows() {
        ScopeMutex lock(&mtx);
        while (windows.size()) {
            destroy_window_nolock(0, windows[0]);
        }
    }

} /* namespace Circada */
