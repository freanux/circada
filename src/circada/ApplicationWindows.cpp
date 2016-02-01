/*
 *  ApplicationWindows.cpp
 *
 *  Created by freanux on Mar 7, 2015
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

#include "Application.hpp"

ScreenWindow *Application::create_window(Session *s, Window *w) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = new ScreenWindow(config, window_sequence++, s, w);
    windows.push_back(sw);
    //std::sort(windows.begin(), windows.end(), ScreenWindowComparer());
    update_input_infobar();

    return sw;
}

ScreenWindow *Application::get_server_window(Session *s) {
    ScopeMutex lock(&draw_mtx);
    return get_server_window_nolock(s);
}

ScreenWindow *Application::get_server_window_nolock(Session *s) {
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        if (sw->get_circada_session() == s && sw->get_circada_window()->get_window_type() == WindowTypeServer) {
            return sw;
        }
    }

    return 0;
}

ScreenWindow *Application::get_window(Window *w) {
    ScopeMutex lock(&draw_mtx);
    return get_window_nolock(w);
}

ScreenWindow *Application::get_window_nolock(Window *w) {
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        if (sw->get_circada_window() == w) {
            return sw;
        }
    }

    return 0;
}

void Application::destroy_window(Window *w) {
    ScopeMutex lock(&draw_mtx);
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        if (sw->get_circada_window() == w) {
            windows.erase(it);
            delete sw;
            update_input_infobar();
            break;
        }
    }
}

void Application::destroy_window(ScreenWindow *w) {
    ScopeMutex lock(&draw_mtx);
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        if (sw == w) {
            windows.erase(it);
            delete sw;
            update_input_infobar();
            break;
        }
    }
}

void Application::set_topic(Window *w, const std::string& topic) {
    ScopeMutex lock(&draw_mtx);
    if (w && selected_window->get_circada_window() == w) {
        topic_widget.set_topic(w->get_topic());
        topic_widget.draw();
        set_cursor();
    }
}

void Application::set_topic(ScreenWindow *w, const std::string& topic) {
    ScopeMutex lock(&draw_mtx);
    if (w && selected_window == w) {
        topic_widget.set_topic(w->get_circada_window()->get_topic());
        topic_widget.draw();
        set_cursor();
    }
}

ScreenWindow *Application::set_name(Window *w, const std::string& name) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (sw && selected_window->get_circada_window() == w) {
        //std::sort(windows.begin(), windows.end(), ScreenWindowComparer());
        status_widget.set_window_name(w->get_name());
        update_input_infobar();
        status_widget.draw();
        set_cursor();
    }

    return sw;
}

ScreenWindow *Application::set_name(ScreenWindow *w, const std::string& name) {
    ScopeMutex lock(&draw_mtx);
    if (w && selected_window == w) {
        //std::sort(windows.begin(), windows.end(), ScreenWindowComparer());
        status_widget.set_window_name(w->get_circada_window()->get_name());
        update_input_infobar();
        status_widget.draw();
        set_cursor();
    }

    return w;
}

ScreenWindow *Application::set_channel_mode(Window *w, const std::string& mode) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (sw && selected_window->get_circada_window() == w) {
        status_widget.set_channel_mode(w->get_flags());
        status_widget.draw();
        set_cursor();
    }

    return sw;
}

void Application::set_lag(ScreenWindow *w, double lag_in_s) {
    ScopeMutex lock(&draw_mtx);
    if (w == selected_window) {
        status_widget.set_lag(lag_in_s);
        status_widget.draw();
        set_cursor();
    }
}

void Application::changes_in_nicklist(Window *w) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (sw && sw == selected_window) {
        if (w->get_window_type() == WindowTypeChannel) {
            status_widget.set_nick_count(w->get_nicks().size());
            status_widget.draw();
            nicklist_widget.draw(sw);
            set_cursor();
        }
    }
}

void Application::select_window(ScreenWindow *w) {
    if (selected_window != w) {
        {
            ScopeMutex lock(&draw_mtx);

            selected_window = w;
            set_nicklist(&w->get_circada_window()->get_nicks());

            /* reset action in selected window */
            if (selected_window->get_circada_window()) {
                selected_window->get_circada_window()->reset_action();
            }

            /* select window and redraw */
            topic_widget.set_topic(w->get_circada_window()->get_topic());
            text_widget.select_window(w);
            nicklist_widget.select_window(w);

            /* fill status & entry widget */
            Session *s = w->get_circada_session();
            if (s) {
                Window *cw = w->get_circada_window();
                status_widget.set_connection(s->get_server());
                status_widget.set_nick(s->get_nick());
                status_widget.set_nick_mode(s->get_flags());
                status_widget.set_nick_away(s->am_i_away());
                if (cw->get_window_type() == WindowTypeServer) {
                    status_widget.set_window_name("status");
                } else {
                    status_widget.set_window_name(cw->get_name());
                }
                if (cw->get_window_type() == WindowTypeChannel) {
                    status_widget.set_nick_count(cw->get_nicks().size());
                    status_widget.set_channel_mode(cw->get_flags());
                } else {
                    status_widget.set_nick_count(-1);
                    status_widget.set_channel_mode("");
                }
                status_widget.set_lag(s->get_lag());

                if (cw->get_window_type() == WindowTypeServer) {
                    entry_widget.set_label("status:");
                } else {
                    entry_widget.set_label(cw->get_name() + ":");
                }
            } else {
                status_widget.set_window_name(w->get_circada_window()->get_name());
                status_widget.set_connection("");
                status_widget.set_nick("");
                status_widget.set_nick_mode("");
                status_widget.set_nick_away(false);
                status_widget.set_channel_mode("");
                status_widget.set_nick_count(-1);
                status_widget.set_lag(0);
                if (w->get_circada_window()->get_window_type() == WindowTypeApplication) {
                    entry_widget.set_label("#");
                } else {
                    entry_widget.set_label(w->get_circada_window()->get_name() + ":");
                }
            }
            status_widget.set_window_type(w->get_circada_window()->get_window_type());
            status_widget.set_window_nbr(get_window_nbr(w));
            status_widget.set_following(w->following);
        }

        /* update widgets */
        update_input_infobar();
        configure();
    }
}

void Application::select_next_window() {
    ScreenWindow *w = text_widget.get_selected_window();
    if (w) {
        for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            ScreenWindow *tmp_w = *it;
            if (tmp_w == w) {
                it++;
                if (it != windows.end()) {
                    select_window(*it);
                }
                break;
            }
        }
    }
}

void Application::select_prev_window() {
    ScreenWindow *w = text_widget.get_selected_window();
    if (w) {
        for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            ScreenWindow *tmp_w = *it;
            if (tmp_w == w) {
                if (it != windows.begin()) {
                    it--;
                    select_window(*it);
                    break;
                }
            }
        }
    }
}

int Application::get_window_nbr(ScreenWindow *w) {
    int cnt = 0;
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        cnt++;
        ScreenWindow *tmp_w = *it;
        if (tmp_w == w) {
            break;
        }
    }

    return cnt;
}
