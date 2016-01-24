/*
 *  TextWidget.cpp
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

#include "TextWidget.hpp"
#include "UTF8.hpp"
#include "Utils.hpp"

#include <cwchar>
#include <cstdlib>

/* NOTICES:
 * this chunk of code is a quick-and-dirty hack. it works, but it isn't
 * clearly legible. sorry for that :)
 */

TextWidget::TextWidget(StatusWidget& status_widget)
    : configured(false), selected_window(0), startx(0),
      orig_width(0), orig_height(0), orig_posx(0), width(0), height(0),
      curx(0), cury(0), first_line(true), status_widget(status_widget) { }

TextWidget::~TextWidget() {
    void delete_ncurses_object();
}

void TextWidget::configure(int height, int width, int posx) {
    this->orig_height = height;
    this->orig_width = width;
    this->orig_posx = posx;

    this->height = height - 1;
    this->width = width;
    delete_ncurses_object();
    win_text = newwin(this->height, this->width, 1, posx);
    scrollok(win_text, FALSE);
    configured = true;
}

void TextWidget::draw() {
    if (selected_window) {
        if (selected_window->following) {
            ScreenWindow::Lines& lines = selected_window->get_lines();
            int index = lines.size();
            if (index) {
                index--;
                selected_window->line_at_bottom = index;
                int save_curx = curx;
                int save_cury = cury;
                selected_window->rows_in_last_line = draw_line(lines[index], true);
                curx = save_curx;
                cury = save_cury;
            } else {
                selected_window->line_at_bottom = 0;
                selected_window->rows_in_last_line = 0;
            }
        }
        top_down_draw(height);
    }
}

void TextWidget::scroll_up() {
    top_down_draw(2 * height - 1);
}

void TextWidget::scroll_down() {
    ScreenWindow::Lines& lines = selected_window->get_lines();
    int sz = lines.size();
    if (selected_window->line_at_bottom == sz - 1) {
        top_down_draw(height);
    } else {
        bool bailed_out = draw_clipping(selected_window->line_at_bottom, 0, height + selected_window->rows_in_last_line - 1);
        if (bailed_out) {
            wrefresh(win_text);
        } else {
            top_down_draw(height);
        }
    }
}

void TextWidget::top_down_draw(int max_lines) {
    ScreenWindow::Lines& lines = selected_window->get_lines();

    int counted_lines = selected_window->rows_in_last_line;
    int current_index = selected_window->line_at_bottom;
    int skip_rows = 0;

    if (lines.size()) {
        while (current_index) {
            current_index--;

            /* test to get number of lines of current line */
            curx = cury = 0;
            int lines_to_draw = draw_line(lines[current_index], true);
            counted_lines += lines_to_draw;
            if (counted_lines >= max_lines) {
                skip_rows = counted_lines - max_lines;
                break;
            }
        }
    }

    /* draw */
    draw_clipping(current_index, skip_rows, height);
    wrefresh(win_text);
}

bool TextWidget::draw_clipping(int from_index, int upmost_skip_rows, int how_many_rows) {
    ScreenWindow::Lines& lines = selected_window->get_lines();
    int sz = lines.size();

    bool bailed_out = false;

    int save_curx, save_cury;
    curx = cury = 0;
    for (int x = 0; x < width; x++) {
        mvwaddch(win_text, cury, x, ' ');
    }
    first_line = true;
    if (from_index < sz) {
        int lines_drawn = draw_line(lines[from_index++], false, upmost_skip_rows + 1) - upmost_skip_rows;
        while (from_index < sz && lines_drawn < how_many_rows) {
            /* test */
            save_curx = curx;
            save_cury = cury;
            curx = cury = 0;
            int lines_to_be_drawn = draw_line(lines[from_index], true);
            curx = save_curx;
            cury = save_cury;

            /* draw */
            selected_window->line_at_bottom = from_index;
            if (lines_drawn + lines_to_be_drawn > how_many_rows) {
                /* draw upper part of line */
                int lines_to_draw = how_many_rows - lines_drawn;
                selected_window->rows_in_last_line = lines_to_draw;
                draw_line(lines[from_index], false, -1, lines_to_draw + 1);
                bailed_out = true;
                break;
            } else {
                selected_window->rows_in_last_line = draw_line(lines[from_index]);
            }
            lines_drawn += lines_to_be_drawn;
            from_index++;
        }
    }

    for (int y = cury + 1; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mvwaddch(win_text, y, x, ' ');
        }
    }

    /* is there more text to show -> set following to false */
    if (sz) {
        selected_window->following = false;
        from_index = sz - 1;
        if (selected_window->line_at_bottom == from_index) {
            save_curx = curx;
            save_cury = cury;
            curx = cury = 0;
            int lines_to_be_drawn = draw_line(lines[from_index], true);
            curx = save_curx;
            cury = save_cury;
            if (selected_window->rows_in_last_line == lines_to_be_drawn) {
                selected_window->following = true;
            }
        }
    } else {
        selected_window->following = true;
    }

    /* update +++ in statusbar */
    if (selected_window->following != status_widget.get_following()) {
        status_widget.set_following(selected_window->following);
        status_widget.draw();
        status_widget.refresh();
    }

    return bailed_out;
}

void TextWidget::draw_line(ScreenWindow *w, const std::string& line) {
    if (w == selected_window) {
        if (w->following) {
            selected_window->rows_in_last_line = draw_line(line, false);
            selected_window->line_at_bottom = w->get_lines().size() - 1;
            if (w->line_at_bottom < 0) {
                w->line_at_bottom = 0;
            }
        }
    }
}

int TextWidget::draw_line(const std::string& line, bool test_only, int from_line, int to_line) {
    int line_height = 0;

    unsigned char c;
    UTF8Iterator utf8_it(line.begin());
    std::string word;
    int leftmost = startx;
    bool new_word = true;

    /* new line? */
    if (first_line) {
        first_line = false;
    } else {
        increment_line(leftmost, line_height, test_only, from_line, to_line);
    }
    line_height = 1;

    while (utf8_it != line.end()) {
        /* check word wrap */
        if (new_word && leftmost) {
            new_word = false;
            int character_count = 0;
            UTF8Iterator temp_utf8_it = utf8_it;
            while (temp_utf8_it != line.end()) {
                c = *temp_utf8_it.get_string_iterator();
                if (c == ' ') {
                    break;
                }

                if (c == Formatter::AttributeSwitch) {
                    std::string::const_iterator str_it = temp_utf8_it.get_string_iterator();
                    str_it++;
                    c = *str_it;
                    switch (c) {
                        case Formatter::AttributeFormat:
                            str_it += sizeof(Formatter::Attribute) + 1;
                            break;

                        case Formatter::AttributeBreakMarker:
                        case Formatter::AttributeRepeater:
                            str_it++;
                            break;
                    }
                    temp_utf8_it = UTF8Iterator(str_it);
                } else {
                    character_count += get_display_width(temp_utf8_it.get_sequence());
                    //character_count++;
                    temp_utf8_it++;
                }
            }

            if (character_count) {
                if (curx + character_count >= width && leftmost + character_count <= width) {
                    if (!test_only && to_line > -1 && line_height == to_line - 1) {
                        break;
                    }
                    increment_line(leftmost, line_height, test_only, from_line, to_line);
                    if (leftmost) {
                        curx--;
                    }
                }
            }
        }

        /* regular drawing */
        c = *utf8_it.get_string_iterator();
        if (c == Formatter::AttributeSwitch) {
            std::string::const_iterator str_it = utf8_it.get_string_iterator();
            str_it++;
            c = *str_it;
            if (c == Formatter::AttributeFormat) {
                draw_word(leftmost, word, line_height, test_only, from_line, to_line);
                if (!test_only && to_line > -1 && line_height == to_line) break;
                str_it++;
                int diff = str_it - line.begin();
                if (!test_only) {
                    set_formats(&line.c_str()[diff]);
                }
                str_it += sizeof(Formatter::Attribute);
            } else if (c == Formatter::AttributeBreakMarker) {
                leftmost = curx + 1;
                str_it++;
            } else if (c == Formatter::AttributeRepeater) {
                if (!test_only) {
                    while (curx < width - 1) {
                        mvwaddch(win_text, cury, curx++, '-');
                    }
                }
                str_it++;
            }
            utf8_it = UTF8Iterator(str_it);
        } else {
            if (c == ' ') {
                new_word = true;
                draw_word(leftmost, word, line_height, test_only, from_line, to_line);
                if (!test_only && to_line > -1 && line_height == to_line) break;
            }
            word += utf8_it.get_sequence();
            utf8_it++;
        }
    }
    if (word.length()) {
        draw_word(leftmost, word, line_height, test_only, from_line, to_line);
    }

    /* fill up line */
    wattroff(win_text, A_BOLD);
    wattroff(win_text, A_UNDERLINE);
#ifdef A_ITALIC
    wattroff(win_text, A_ITALIC);
#endif
    if (!test_only) {
        for (int i = curx; i < width; i++) {
            mvwaddch(win_text, cury, i, ' ');
        }
    }

    return line_height;
}

void TextWidget::refresh(ScreenWindow *w) {
    if (selected_window == w) {
        wrefresh(win_text);
    }
}

void TextWidget::select_window(ScreenWindow *w) {
    selected_window = w;
    configure(orig_height, orig_width, orig_posx);
}

ScreenWindow *TextWidget::get_selected_window() {
    return selected_window;
}

void TextWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_text);
        configured = false;
    }
}

void TextWidget::set_formats(const char *p) {
    const Formatter::Attribute *a = reinterpret_cast<const Formatter::Attribute *>(p);
    wcolor_set(win_text, a->color, 0);
    if (a->switches & Formatter::AttributeBold) {
        wattron(win_text, A_BOLD);
    } else {
        wattroff(win_text, A_BOLD);
    }

    if (a->switches & Formatter::AttributeUnderline) {
        wattron(win_text, A_UNDERLINE);
    } else {
        wattroff(win_text, A_UNDERLINE);
    }

    /* on many systems, A_ITALIC is not defined */
#ifdef A_ITALIC
    if (a->switches & Formatter::AttributeItalic) {
        wattron(win_text, A_ITALIC);
    } else {
        wattroff(win_text, A_ITALIC);
    }
#endif
}

void TextWidget::draw_word(int leftmost, std::string& word, int& line_height, bool test_only, int from_line, int to_line) {
    int len = get_utf8_length(word);
    if (len) {
        UTF8Iterator it = word.begin();
        int pos;
        while (*it) {
            pos = curx + get_display_width(it.get_sequence());
            if (pos > width) {
                if (!test_only && to_line > -1 && line_height == to_line - 1) return;
                increment_line(leftmost, line_height, test_only, from_line, to_line);
            }
            if (!test_only && (from_line == -1 || line_height >= from_line)) {
                mvwaddstr(win_text, cury, curx, it.get_sequence().c_str());
            }
            curx += get_display_width(it.get_sequence());

            /*
            if (curx > width - 1) {
                if (!test_only && to_line > -1 && line_height == to_line - 1) return;
                increment_line(leftmost, line_height, test_only, from_line, to_line);
            }
            if (!test_only && (from_line == -1 || line_height >= from_line)) {
                mvwaddstr(win_text, cury, curx, it.get_sequence().c_str());
            }
            curx += get_display_width(it.get_sequence());
            */

            it++;
        }
        word.clear();
    }
}

void TextWidget::increment_line(int new_posx, int& line_height, bool test_only, int from_line, int to_line) {
    /* fill up line */
    if (!test_only && (from_line == -1 || line_height >= from_line)) {
        for (int i = curx; i < width; i++) {
            mvwaddch(win_text, cury, i, ' ');
        }
    }

    /* next line */
    if (cury < height - 1) {
        if  (from_line == -1 || line_height >= from_line) {
            cury++;
        }
    } else {
        if (!test_only) {
            if  (from_line == -1 || line_height >= from_line) {
                scrollok(win_text, TRUE);
                wscrl(win_text, 1);
                scrollok(win_text, FALSE);
            }
        }
    }
    curx = new_posx;

    /* fill up line */
    if (!test_only && (from_line == -1 || line_height >= from_line)) {
        for (int i = 0; i < curx; i++) {
            mvwaddch(win_text, cury, i, ' ');
        }
    }

    /* increment used rows in this text line */
    line_height++;
}
