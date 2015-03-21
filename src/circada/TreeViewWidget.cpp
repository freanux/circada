/*
 *  TreeViewWidget.cpp
 *
 *  Created by freanux on Mar 18, 2015
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

#include "TreeViewWidget.hpp"
#include "Utils.hpp"

TreeViewEntry::TreeViewEntry(ScreenWindow *win, bool coloring, const std::string& sep, const std::string& entry)
    : win(win), coloring(coloring), sep(sep), entry(entry) { }

TreeViewWidget::TreeViewWidget() : configured(false), separator("│") { }

TreeViewWidget::~TreeViewWidget() {
    void delete_ncurses_object();
}

void TreeViewWidget::configure(int height, int width, int posy, int posx) {
    delete_ncurses_object();
    this->height = height;
    this->width = width;
    win_treeview = newwin(height, width, posy, posx);
    scrollok(win_treeview, FALSE);
    configured = true;
}

int TreeViewWidget::get_estimated_width() {
    int max_width = 0;
    for (TreeView::iterator it = treeview.begin(); it != treeview.end(); it++) {
        TreeViewEntry& e = *it;
        int len = e.entry.length() + get_display_width_string(e.sep);
        if (len > max_width) {
            max_width = len;
        }
    }
    max_width += get_display_width_string(separator);
    return max_width;
}

TreeViewWidget::TreeView& TreeViewWidget::get_treeview() {
    return treeview;
}

void TreeViewWidget::draw(ScreenWindow *selected_window) {
    if (configured) {
        int sep_width = get_display_width_string(separator);
        int border_pos = width - sep_width;
        size_t index = 0;
        size_t sz = treeview.size();

        /* calculate top */
        for (size_t i = 0; i < sz; i++) {
            const TreeViewEntry& e = treeview[i];
            if (e.coloring && e.win == selected_window) {
                index = i;
                break;
            }
        }
        if (index + height > sz - 1) {
            int new_index = sz - height;
            if (new_index < 0) {
                new_index = 0;
            }
            index = new_index;
        }

        /* draw */
        for (int y = 0; y < height; y++) {
            if (index < sz) {
                const TreeViewEntry& e = treeview[index++];
                wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlack), 0);
                mvwaddstr(win_treeview, y, 0, e.sep.c_str());
                if (e.coloring) {
                    if (e.win == selected_window) {
                        wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorBrightBlack), 0);
                    } else {
                        switch (e.win->get_circada_window()->get_action()) {
                            case Circada::WindowActionNone:
                                wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlack), 0);
                                break;

                            case Circada::WindowActionNoise:
                                wattron(win_treeview, A_BOLD);
                                wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack), 0);
                                break;

                            case Circada::WindowActionChat:
                                wattron(win_treeview, A_BOLD);
                                wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack), 0);
                                break;

                            case Circada::WindowActionAlert:
                                wattron(win_treeview, A_BOLD);
                                wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack), 0);
                                break;
                        }
                    }
                }
                waddstr(win_treeview, e.entry.c_str());
                wattroff(win_treeview, A_BOLD);
            }
            wclrtoeol(win_treeview);
            wcolor_set(win_treeview, Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlack), 0);
            mvwaddstr(win_treeview, y, border_pos, separator.c_str());
        }
        wrefresh(win_treeview);
    }
}

void TreeViewWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_treeview);
        configured = false;
    }
}
