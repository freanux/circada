/*
 *  TextWidget.hpp
 *
 *  Created by freanux on Mar 10, 2015
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

#ifndef _TEXTWIDGET_HPP_
#define _TEXTWIDGET_HPP_

#include "Terminal.hpp"
#include "TopicWidget.hpp"
#include "StatusWidget.hpp"
#include "ScreenWindow.hpp"
#include "Formatter.hpp"

#include <string>

class TextWidget {
public:
    TextWidget(StatusWidget& status_widget);
    virtual ~TextWidget();

    void configure(int height, int width, int posx);
    void draw();
    void scroll_up();
    void scroll_down();
    void draw_line(ScreenWindow *w, const std::string& line);
    void select_window(ScreenWindow *w);
    ScreenWindow *get_selected_window();
    void refresh(ScreenWindow *w);

private:
    bool configured;
    ScreenWindow *selected_window;
    int startx; /* leftmost position */
    int orig_width;
    int orig_height;
    int orig_posx;
    int width;
    int height;
    int curx;
    int cury;
    bool first_line;
    WINDOW *win_text;

    StatusWidget& status_widget;

    void delete_ncurses_object();
    void top_down_draw(int max_lines);
    bool draw_clipping(int from_index, int upmost_skip_rows, int how_many_rows);
    void set_formats(const char *p);
    int draw_line(const std::string& line, bool test_only = false, int from_line = -1, int to_line = -1);
    void draw_word(int leftmost, std::string& word, int& line_height, bool test_only, int from_line, int to_line);
    void increment_line(int new_posx, int& line_height, bool test_only, int from_line, int to_line);
    void set_height(int line, int height);
    int get_height(int line);
};

#endif // _TEXTWIDGET_HPP_
