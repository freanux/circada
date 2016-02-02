/*
 *  ScreenWindow.cpp
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

#include "ScreenWindow.hpp"
#include "Utils.hpp"

#include <cstring>
#include <algorithm>

template<class T> static bool erase_last_viewed(const T& elem) {
    return (elem.type == ScreenWindow::Line::TypeLastViewed);
}

ScreenWindow::ScreenWindow(Circada::Configuration& config, int sequence, Circada::Session *s, Circada::Window *w)
    : line_at_bottom(0), rows_in_last_line(0), following(true), nicklist_top(0),
      sequence(sequence), config(config), session(s), window(w) { }

ScreenWindow::~ScreenWindow() { }

Circada::Session *ScreenWindow::get_circada_session() {
    return session;
}

Circada::Window *ScreenWindow::get_circada_window() {
    return window;
}

const std::string& ScreenWindow::add_line(Formatter& fmt, const Circada::Message& m, const char *from) {
    std::string line;
    fmt.parse(m, line, from);
    lines.push_back(Line(line));
    cleanup();
    return lines[lines.size() - 1].text;
}

void ScreenWindow::add_formatted_line(const std::string& line) {
    lines.push_back(line);
    cleanup();
}

void ScreenWindow::set_last_viewed(Formatter& fmt) {
    lines.erase(std::remove_if(lines.begin(), lines.end(), erase_last_viewed<Line>), lines.end());
    Circada::Message m;
    m.command = INT_LAST_VIEWED;
    m.injected = false;
    m.its_me = false;
    m.pc = 0;
    m.session = 0;
    m.to_me = false;
    std::string line;
    fmt.parse(m, line, 0);
    lines.push_back(Line(Line::TypeLastViewed, line));
    cleanup();
}

ScreenWindow::Lines& ScreenWindow::get_lines() {
    return lines;
}

int ScreenWindow::get_sequence() {
    return sequence;
}

bool ScreenWindowComparer::operator()(ScreenWindow* const& lhs, ScreenWindow* const& rhs) {
    if (lhs->get_circada_session() == rhs->get_circada_session()) {
        Circada::Window *lhs_w = lhs->get_circada_window();
        Circada::Window *rhs_w = rhs->get_circada_window();
        if (lhs_w->get_window_type() == rhs_w->get_window_type()) {
            return lhs_w->get_name() < rhs_w->get_name();
        }
        return lhs_w->get_window_type() < rhs_w->get_window_type();
    }

    return lhs->get_circada_session() < rhs->get_circada_session();
}

void ScreenWindow::cleanup() {
    int max_messages = atoi(config.get_value("", "window_max_entries", "10000").c_str());
    if (max_messages) {
        while (static_cast<int>(lines.size()) > max_messages) {
            lines.erase(lines.begin());
        }
    }
}
