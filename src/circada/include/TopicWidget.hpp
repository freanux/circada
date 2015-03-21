/*
 *  TopicWidget.hpp
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

#ifndef _TOPICWIDGET_HPP_
#define _TOPICWIDGET_HPP_

#include "Terminal.hpp"

#include <string>

class TopicWidget {
public:
    TopicWidget();
    virtual ~TopicWidget();

    void configure(int width);
    void set_topic(const std::string& topic);
    void draw();

private:
    bool configured;
    int width;
    WINDOW *win_topic;
    std::string topic;

    void delete_ncurses_object();
};

#endif // _TOPICWIDGET_HPP_
