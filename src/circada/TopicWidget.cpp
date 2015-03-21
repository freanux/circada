/*
 *  TopicWidget.cpp
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

#include "TopicWidget.hpp"
#include "Formatter.hpp"

TopicWidget::TopicWidget() : configured(false) { }

TopicWidget::~TopicWidget() {
    void delete_ncurses_object();
}

void TopicWidget::configure(int width) {
    delete_ncurses_object();
    this->width = width;
    win_topic = newwin(1, width, 0, 0);
    scrollok(win_topic, FALSE);
}

void TopicWidget::set_topic(const std::string& topic) {
    this->topic = topic;
}

void TopicWidget::draw() {
    wcolor_set(win_topic, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlue), 0);
    mvwaddch(win_topic, 0, 0, ' ');
    waddstr(win_topic, topic.c_str());
    int y, x;
    getyx(win_topic, y, x);
    for (int i = x; i < width; i++) {
        mvwaddch(win_topic, y, i, ' ');
    }
    wrefresh(win_topic);
}

void TopicWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_topic);
        configured = false;
    }
}
