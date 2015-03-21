/*
 *  StatusWidget.hpp
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

#ifndef _STATUSWIDGET_HPP_
#define _STATUSWIDGET_HPP_

#include "EntryWidget.hpp"
#include "ScreenWindow.hpp"

#include <Circada/Circada.hpp>
#include <string>

class StatusWidget {
public:
    StatusWidget(ScreenWindow::List& windows);
    virtual ~StatusWidget();

    void configure(int posy, int width);
    void set_delimiter(const std::string& delimiter);
    void set_window_nbr(int nbr);
    void set_nick(const std::string& nick);
    void set_nick_mode(const std::string& nick_mode);
    void set_nick_away(bool state);
    void set_connection(const std::string& connection);
    void set_window_name(const std::string& window_name);
    void set_channel_mode(const std::string& channel_mode);
    void set_nick_count(int nbr);   /* -1 to set off */
    void set_window_type(Circada::WindowType type);
    void set_following(bool state);
    bool get_following() const;
    void set_lag(double lag_in_s);
    void draw();
    void refresh();
    void draw_time(time_t new_time);

private:
    ScreenWindow::List& windows;
    bool configured;
    std::string delimiter;
    int clock_position;
    bool following;
    std::string following_indicator;
    Circada::WindowType window_type;
    bool nick_is_away;
    double lag_in_s;

    int width;
    WINDOW *win_status;

    std::string window_nbr;
    std::string nick;
    std::string nick_mode;
    std::string connection;
    std::string window_name;
    std::string channel_mode;
    std::string nick_count;

    void delete_ncurses_object();
};

#endif // _STATUSWIDGET_HPP_
