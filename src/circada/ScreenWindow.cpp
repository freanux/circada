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

const std::string& ScreenWindow::add_line(Formatter& fmt, const Circada::Message& m) {
    std::string line;
    fmt.parse(m, line);
    lines.push_back(line);
    cleanup();
    return lines[lines.size() - 1];
}

void ScreenWindow::add_formatted_line(const std::string& line) {
    lines.push_back(line);
    cleanup();
}

ScreenWindow::Lines& ScreenWindow::get_lines() {
    return lines;
}

int ScreenWindow::get_sequence() {
    return sequence;
}

bool ScreenWindowComparer::operator()(ScreenWindow* const& lhs, ScreenWindow* const& rhs) {
    //if (!lhs->get_circada_session()) {
    //    return true;
    //}

    if (lhs->get_circada_session() == rhs->get_circada_session()) {
        Circada::Window *lhs_w = lhs->get_circada_window();
        Circada::Window *rhs_w = rhs->get_circada_window();
        if (lhs_w->get_window_type() == rhs_w->get_window_type()) {
            return lhs->get_sequence() < rhs->get_sequence();
            /*
            const std::string& lhs_name = lhs_w->get_name();
            const std::string& rhs_name = rhs_w->get_name();
            return strcasecmp(lhs_name.c_str(), rhs_name.c_str()) < 0;
            */
        }
        return (lhs_w->get_window_type() < rhs_w->get_window_type());
    }

    return (lhs->get_sequence() < rhs->get_sequence());
}

void ScreenWindow::cleanup() {
    int max_messages = atoi(config.get_value("", "window_max_entries", "10000").c_str());
	if (max_messages) {
		while (static_cast<int>(lines.size()) > max_messages) {
			lines.erase(lines.begin());
		}
	}
}
