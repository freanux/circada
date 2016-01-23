/*
 *  StatusWidget.cpp
 *
 *  Created by freanux on Mar 9, 2015
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

#include "StatusWidget.hpp"
#include "Formatter.hpp"
#include "Utils.hpp"

#include <time.h>

StatusWidget::StatusWidget(ScreenWindow::List& windows)
    : windows(windows), configured(false), delimiter("│"), clock_position(0),
      following(true), following_indicator("+++"),
      window_type(Circada::WindowTypeApplication), nick_is_away(false), lag_in_s(0) { }

StatusWidget::~StatusWidget() {
    delete_ncurses_object();
}

void StatusWidget::configure(int posy, int width) {
    delete_ncurses_object();
    this->width = width;
    win_status = newwin(1, width, posy, 0);
    scrollok(win_status, FALSE);
}

void StatusWidget::set_window_nbr(int nbr) {
    char buffer[16];
    sprintf(buffer, "%d", nbr);
    window_nbr.assign(buffer);
}

void StatusWidget::set_nick(const std::string& nick) {
    this->nick = nick;
}

void StatusWidget::set_nick_mode(const std::string& nick_mode) {
    this->nick_mode = nick_mode;
}

void StatusWidget::set_nick_away(bool state) {
    nick_is_away = state;
}

void StatusWidget::set_connection(const std::string& connection) {
    this->connection = connection;
}

void StatusWidget::set_window_name(const std::string& window_name) {
    this->window_name = window_name;
}

void StatusWidget::set_channel_mode(const std::string& channel_mode) {
    this->channel_mode = channel_mode;
}

void StatusWidget::set_nick_count(int nbr) {
    if (nbr < 0) {
        nick_count.clear();
    } else {
        char buffer[16];
        sprintf(buffer, "%d", nbr);
        nick_count.assign(buffer);
    }
}

void StatusWidget::set_window_type(Circada::WindowType type) {
    window_type = type;
}

void StatusWidget::set_following(bool state) {
    following = state;
}

bool StatusWidget::get_following() const {
    return following;
}

void StatusWidget::set_lag(double lag_in_s) {
    this->lag_in_s = lag_in_s;
}

void StatusWidget::draw() {
    int y, x;

    /* draw window number */
    wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightCyan, FormatterColorDarkBlue), 0);
    mvwaddch(win_status, 0, 0, ' ');

    waddstr(win_status, window_nbr.c_str());
    wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);

    waddch(win_status, ' ');
    waddstr(win_status, delimiter.c_str());

    /* draw time */
    waddch(win_status, ' ');
    getyx(win_status, y, clock_position);
    draw_time(time(0));

    /* draw nick */
    if (nick.length()) {
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddch(win_status, ' ');
        waddstr(win_status, nick.c_str());
        if (nick_mode.length()) {
            waddch(win_status, ' ');
            waddch(win_status, '(');
            waddstr(win_status, nick_mode.c_str());
            waddch(win_status, ')');
        }
        if (nick_is_away) {
            wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightMagenta, FormatterColorDarkBlue), 0);
            waddstr(win_status, " zZzZ");
            wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
        }
    }

    /* connection */
    if (connection.length()) {
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddch(win_status, ' ');
        waddstr(win_status, connection.c_str());
    }

    /* window name */
    if (window_name.length()) {
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddch(win_status, ' ');
        switch (window_type) {
            case Circada::WindowTypeApplication:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
                break;

            case Circada::WindowTypeServer:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightYellow, FormatterColorDarkBlue), 0);
                break;

            case Circada::WindowTypeChannel:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlue), 0);
                break;

            case Circada::WindowTypePrivate:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
                break;

            case Circada::WindowTypeDCC:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightCyan, FormatterColorDarkBlue), 0);
                break;

            case Circada::WindowTypeAlerts:
                wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
                break;
        }
        waddstr(win_status, window_name.c_str());
        wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
        if (channel_mode.length()) {
            waddch(win_status, ' ');
            waddch(win_status, '(');
            waddstr(win_status, channel_mode.c_str());
            waddch(win_status, ')');
        }
    }

    /* nick count */
    if (nick_count.length()) {
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddch(win_status, ' ');
        waddstr(win_status, nick_count.c_str());
    }

    /* lag */
    if (lag_in_s >= 0.3) {
        char buffer[32];
        sprintf(buffer, "%.2f", lag_in_s);
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddstr(win_status, " Lag: ");
        waddstr(win_status, buffer);
    }

    /* draw activities */
    bool draw_activities = false;
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *w = *it;
        Circada::Window *cw = w->get_circada_window();
        if (cw && cw->get_action() != Circada::WindowActionNone) {
            draw_activities = true;
            break;
        }
    }
    if (draw_activities) {
        waddch(win_status, ' ');
        waddstr(win_status, delimiter.c_str());
        waddch(win_status, ' ');
        waddstr(win_status, "Act: ");
        int window_nbr = 0;
        for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
            window_nbr++;
            ScreenWindow *w = *it;
            switch (w->get_circada_window()->get_action()) {
                case Circada::WindowActionNoise:
                    wcolor_set(win_status, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlue), 0);
                    wprintw(win_status, "%d ", window_nbr);
                    break;

                case Circada::WindowActionChat:
                    wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlue), 0);
                    wattron(win_status, A_BOLD);
                    wprintw(win_status, "%d ", window_nbr);
                    wattroff(win_status, A_BOLD);
                    break;

                case Circada::WindowActionAlert:
                    wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightRed, FormatterColorDarkBlue), 0);
                    wattron(win_status, A_BOLD);
                    wprintw(win_status, "%d ", window_nbr);
                    wattroff(win_status, A_BOLD);
                    break;

                default:
                    break;
            }
        }
    }

    /* fill up */
    getyx(win_status, y, x);
    for (int i = x; i < width; i++) {
        mvwaddch(win_status, y, i, ' ');
    }

    /* draw following indicator */
    wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightRed, FormatterColorDarkBlue), 0);
    x = width - get_display_width_string(following_indicator) - 2;
    if (!following) {
        mvwaddch(win_status, 0, x++, ' ');
        mvwaddstr(win_status, 0, x, following_indicator.c_str());
    }

    /* refresh */
    wrefresh(win_status);
}

void StatusWidget::refresh() {
    wrefresh(win_status);
}

void StatusWidget::draw_time(time_t new_time) {
    if (clock_position) {
        char buf[16];
        char fmt[] = "%H:%M:%S";
        struct tm *tp;
        tp = localtime(&new_time);
        strftime(buf, sizeof(buf), fmt, tp);

        wcolor_set(win_status, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
        mvwaddstr(win_status, 0, clock_position, buf);

        int x, y;
        getyx(win_status, y, x);
        if (x >= width - 1) {
            mvwaddch(win_status, y, width - 1, ' ');
        }
    }
}

void StatusWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_status);
        configured = false;
    }
}
