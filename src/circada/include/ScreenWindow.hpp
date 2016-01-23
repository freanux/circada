/*
 *  ScreenWindow.hpp
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

#ifndef _SCREENWINDOW_HPP_
#define _SCREENWINDOW_HPP_

#include "Formatter.hpp"

#include <Circada/Circada.hpp>

#include <vector>

class ScreenWindow {
public:
    typedef std::vector<Circada::DCCHandle> DDCHandles;
    typedef std::vector<ScreenWindow *> List;
    typedef std::vector<std::string> Lines;

    ScreenWindow(Circada::Configuration& config, int sequence, Circada::Session *s, Circada::Window *w);
    virtual ~ScreenWindow();

    Circada::Session *get_circada_session();
    Circada::Window *get_circada_window();
    const std::string& add_line(Formatter& fmt, const Circada::Message& m, const char *from = 0);
    void add_formatted_line(const std::string& line);
    Lines& get_lines();
    int get_sequence();

    /* direct accessible */
    int line_at_bottom;
    int rows_in_last_line;
    bool following;
    int nicklist_top;
    DDCHandles dcc_handles;

private:
    int sequence;
    Circada::Configuration& config;
    Circada::Session *session;
    Circada::Window *window;

    Lines lines;

    void cleanup();
};

struct ScreenWindowComparer {
    bool operator()(ScreenWindow* const& lhs, ScreenWindow* const& rhs);
};

#endif // _SCREENWINDOW_HPP_
