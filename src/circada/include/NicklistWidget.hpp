/*
 *  NicklistWidget.hpp
 *
 *  Created by freanux on Mar 14, 2015
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

#ifndef _NICKLISTWIDGET_HPP_
#define _NICKLISTWIDGET_HPP_

#include "Terminal.hpp"
#include "ScreenWindow.hpp"

#include <string>

class NicklistWidget {
public:
    NicklistWidget();
    virtual ~NicklistWidget();

    void configure(int posy, int posx, int width, int height);
    void draw(ScreenWindow *w);
    void select_window(ScreenWindow *w);
    void scroll_up();
    void scroll_down();
    const std::string& get_nicklist_delimiter() const;
    void set_nicklist_delimiter(const std::string& delimiter);

private:
    bool configured;
    ScreenWindow *current_window;
    std::string nicklist_delimiter;
    int width;
    int height;
    WINDOW *win_nicklist;

    void delete_ncurses_object();
};

#endif // _NICKLISTWIDGET_HPP_
