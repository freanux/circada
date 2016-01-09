/*
 *  NicklistWidget.cpp
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

#include "NicklistWidget.hpp"
#include "Formatter.hpp"

#include <time.h>

NicklistWidget::NicklistWidget()
    : configured(false), current_window(0), nicklist_delimiter("│") { }

NicklistWidget::~NicklistWidget() {
    delete_ncurses_object();
}

void NicklistWidget::configure(int posy, int posx, int width, int height) {
    delete_ncurses_object();
    this->width = width;
    this->height = height;
    delete_ncurses_object();
    win_nicklist = newwin(height, width, posy, posx);
    scrollok(win_nicklist, FALSE);
}

void NicklistWidget::draw(ScreenWindow *w) {
    if (w == current_window) {
        Circada::Nick::List *list = 0;
        int curpos = 0;
        int sz = 0;
        if (w) {
            if (w->get_circada_window()) {
                list = &w->get_circada_window()->get_nicks();
                curpos = w->nicklist_top;
                sz = list->size();
                if (curpos + height > sz) {
                    curpos = sz - height;
                    if (curpos < 0) {
                        curpos = 0;
                    }
                }
            }
            w->nicklist_top = curpos;
        }

        for (int y = 0; y < height; y++) {
            wcolor_set(win_nicklist, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlack), 0);
            mvwaddstr(win_nicklist, y, 0, nicklist_delimiter.c_str());
            if (w && list && curpos < sz) {
                Circada::Nick& nick = (*list)[curpos];
                std::string nick_with_symbol;
                nick_with_symbol += nick.get_flag();
                nick_with_symbol += nick.get_nick();
                if (nick.get_flag() != ' ') {
                    wcolor_set(win_nicklist, Formatter::get_color_code(FormatterColorBrightYellow, FormatterColorDarkBlack), 0);
                } else {
                    wcolor_set(win_nicklist, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack), 0);
                }
                waddstr(win_nicklist, nick_with_symbol.c_str());
                curpos++;
            }
            wclrtoeol(win_nicklist);
        }
        wrefresh(win_nicklist);
    }
}

void NicklistWidget::scroll_up() {
    if (current_window) {
        int new_top = current_window->nicklist_top - height;
        if (new_top < 0) {
            new_top = 0;
        }
        if (new_top != current_window->nicklist_top) {
            current_window->nicklist_top = new_top;
            draw(current_window);
        }
    }
}


void NicklistWidget::scroll_down() {
    if (current_window) {
        Circada::Window *cw = current_window->get_circada_window();
        if (cw) {
            int sz = cw->get_nicks().size();
            if (sz) {
                int new_top = current_window->nicklist_top + height;
                if (new_top > sz - 1) {
                    new_top = sz - 1;
                }
                if (new_top != current_window->nicklist_top) {
                    current_window->nicklist_top = new_top;
                    draw(current_window);
                }
            }
        }
    }
}

const std::string& NicklistWidget::get_nicklist_delimiter() const {
    return nicklist_delimiter;
}

void NicklistWidget::set_nicklist_delimiter(const std::string& delimiter) {
    nicklist_delimiter = delimiter;
}

void NicklistWidget::select_window(ScreenWindow *w) {
    current_window = w;
}

void NicklistWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_nicklist);
        configured = false;
    }
}

