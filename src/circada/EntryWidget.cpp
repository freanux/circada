/*
 *  EntryWidget.cpp
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

#include "EntryWidget.hpp"
#include "UTF8.hpp"
#include "Formatter.hpp"
#include "Utils.hpp"

#include <cstring>
#include <cstdlib>

const int EntryTimeout = 500;

/* perhaps make these sequences configurable */
EntryWidget::EscapeKeyEvent EntryWidget::keyevents[] = {
    { "", &EntryWidget::keyevent_escape },
    { "[3~", &EntryWidget::keyevent_delete },
    { "\x7f", &EntryWidget::keyevent_delete_word },
    { "[A", &EntryWidget::keyevent_up },
    { "[B", &EntryWidget::keyevent_down },
    { "[C", &EntryWidget::keyevent_right },
    { "[D", &EntryWidget::keyevent_left },
    { "[H", &EntryWidget::keyevent_home },
    { "OH", &EntryWidget::keyevent_home },
    { "[1~", &EntryWidget::keyevent_home },
    { "[F", &EntryWidget::keyevent_end },
    { "OF", &EntryWidget::keyevent_end },
    { "[4~", &EntryWidget::keyevent_end },
    { "[5~", &EntryWidget::keyevent_pgup },
    { "[6~", &EntryWidget::keyevent_pgdn },
    { "[Z", &EntryWidget::keyevent_shift_tab },
    { "[1;5C", &EntryWidget::keyevent_ctrl_right },
    { "[1;5D", &EntryWidget::keyevent_ctrl_left },
    { "[5;5~", &EntryWidget::keyevent_ctrl_pgup },
    { "[6;5~", &EntryWidget::keyevent_ctrl_pgdn},
    { 0, 0 }
};

EntryWidget::EntryWidget(Circada::Mutex& draw_mtx)
    : draw_mtx(draw_mtx), configured(false), curx(0), ofsx(0),
      parser(0), last_mru_index(-1), max_mru(100), numbers_only(false),
      mode_key(0)
{
    timeout(EntryTimeout);
}

EntryWidget::~EntryWidget() {
    delete_ncurses_object();
}

void EntryWidget::configure(const std::string& label, int posx, int posy, int width, int maxlen) {
    delete_ncurses_object();

    if (label.length()) {
        this->label = label;
        this->label_length = get_utf8_length(label);
    }
    this->posx = posx;
    this->posy = posy;
    this->width = width;
    this->maxlen = maxlen;

    win_label = newwin(1, this->label_length + 1, posy, posx);
    win_input = newpad(1, (width > static_cast<int>(maxlen) ? width : maxlen) + 1);

    this->configured = true;
}

void EntryWidget::set_label(const std::string& label) {
    this->label = label;
    configure(this->label, posx, posy, width, maxlen);
}

const std::string& EntryWidget::get_content() {
    return content;
}

char EntryWidget::get_mode_key() {
    return mode_key;
}

EntryWidget::EntryWidgetEvent EntryWidget::input() {
    EntryWidgetEvent retval = EntryWidgetEventNone;
    timeout(EntryTimeout);
    int ch = getch();
    if (ch != -1) {
        if (ch == KEY_RESIZE) {
            retval = EntryWidgetEventResize;
        } else if (ch == 27) {
            /* catch escape sequence */
            std::string seq;
            timeout(0);
            while (true) {
                ch = getch();
                if (ch == -1) {
                    break;
                }
                seq += static_cast<char>(ch);
            }
            timeout(EntryTimeout);
            EscapeKeyEvent *evt = keyevents;
            while (evt->event) {
                if (!strcmp(evt->sequence, seq.c_str())) {
                    retval = (this->*evt->event)();
                    break;
                }
                evt++;
            }
        } else if (ch == 3) {
            reset();
        } else if (ch == 127) {
            reset_mru_tab();
            retval = keyevent_backspace();
        } else if (ch == 8) {
            reset_mru_tab();
            retval = keyevent_delete_word();
        } else if (ch == 9) {
            retval = keyevent_tab();
        } else if (ch == 10) {
            retval = keyevent_enter();
            reset_mru_tab();
        } else if (ch < 256 && ch & 0x80) {
            if (!numbers_only) {
                /* catch utf8 character sequence */
                std::string seq;
                seq += static_cast<char>(ch);
                timeout(0);
                while (true) {
                    ch = getch();
                    if (ch == -1) {
                        break;
                    }
                    seq += static_cast<char>(ch);
                }
                timeout(EntryTimeout);
                retval = keyevent_insert_sequence(seq);
            }
        } else if (ch > 31 && ch < 256) {
            if (!numbers_only) {
                std::string seq;
                seq += static_cast<char>(ch);
                retval = keyevent_insert_sequence(seq);
            } else if (ch >= 48 && ch <= 57) {
                std::string seq;
                seq += static_cast<char>(ch);
                retval = keyevent_insert_sequence(seq);
            } else {
                mode_key = static_cast<char>(ch);
                retval = EntryWidgetEventModeKey;
            }
        } else {
            reset_mru_tab();
        }
    }

    return retval;
}

void EntryWidget::reset() {
    content.clear();
    curx = ofsx = 0;

    last_text.clear();
    last_mru_index = -1;
    last_cursor_pos = 0;

    draw();
}

void EntryWidget::draw() {
    if (configured) {
        curs_set(0);

        Circada::ScopeMutex lock(&draw_mtx);
        int pos;

        /* draw input label */
        pos = 0;
        wattron(win_label, COLOR_PAIR(Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack)));
        for (UTF8Iterator it = label.begin(); it != label.end(); it++) {
            mvwaddstr(win_label, 0, pos, it.get_sequence().c_str());
            pos += get_display_width(it.get_sequence());
        }
        mvwaddstr(win_label, 0, pos++, " ");

        /* draw input box */
        pos = 0;
        wattron(win_input, COLOR_PAIR(Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack)));
        for (UTF8Iterator it = content.begin(); it != content.end(); it++) {
            mvwaddstr(win_input, 0, pos, it.get_sequence().c_str());
            pos += get_display_width(it.get_sequence());
        }
        for (int i = pos; i < maxlen; i++) {
            mvwaddch(win_input, 0, i, ' ');
        }

        /* draw info bar */
        if (info_left.length() + info_selected.length() + info_right.length()) {
            waddch(win_input, ' ');
            int draw_from = 0;
            int sz = get_utf8_length(info_left);
            int w = width / 2 - (2 * maxlen);
            if (sz > w) {
                draw_from = sz - w - 1;
                if (draw_from > sz) draw_from = sz - 1;
                if (draw_from < 0) draw_from = 0;
            }
            UTF8Iterator utf8_it(info_left.begin(), draw_from);
            int draw_from_utf8 = utf8_it.get_string_iterator() - info_left.begin();
            wattron(win_input, COLOR_PAIR(Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorBrightBlack)));
            waddstr(win_input, info_left.substr(draw_from_utf8).c_str());
            wattron(win_input, COLOR_PAIR(Formatter::get_color_code(FormatterColorDarkBlack, FormatterColorBrightWhite)));
            waddstr(win_input, info_selected.c_str());
            wattron(win_input, COLOR_PAIR(Formatter::get_color_code(FormatterColorDarkWhite, FormatterColorBrightBlack)));
            waddstr(win_input, info_right.c_str());
            int ty, tx;
            getyx(win_input, ty, tx);
            for (int i = tx; i < width; i++) {
                mvwaddch(win_input, ty, i, ' ');
            }
            wattron(win_input, COLOR_PAIR(Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack)));
        }

        /* refreshing label and input box */
        wrefresh(win_label);
        set_cursor();
        curs_set(1);
    }
}

void EntryWidget::set_cursor() {
    int sz = label_length + 1;
    int diff;
    int real_curx = 0;

    int i = 0;
    UTF8Iterator it(content.begin());
    while (i < curx) {
        real_curx += get_display_width(it.get_sequence());
        it++;
        i++;
    }

    diff = real_curx - ofsx - (width - sz - 1);
    if (diff > 0) ofsx += diff;

    diff = real_curx - ofsx;
    if (diff < 0) ofsx += diff;

    wmove(win_input, 0, real_curx);
    prefresh(win_input, 0, ofsx, posy, posx + label_length + 1, posy + 1, width - 1);
}

void EntryWidget::set_parser(Circada::Parser *parser) {
    this->parser = parser;
}

int EntryWidget::get_max_mru() const {
    return max_mru;
}

void EntryWidget::set_max_mru(int max) {
    max_mru = (max < 3 ? 3 : max);
}

void EntryWidget::complete_tab() {
    if (!numbers_only && parser) {
        std::string line = content;
        UTF8Iterator it(content.begin(), curx);
        int pos = it.get_string_iterator() - content.begin();
        parser->complete(&line, &pos);
        content = line;
        curx = 0;
        int i = 0;
        it = content.begin();
        while (i < pos) {
            curx++;
            i += it.get_sequence().length();
        }
        draw();
    }
}

void EntryWidget::mru_up() {
    if (!numbers_only) {
        int sz = static_cast<int>(mru.size());
        if (sz && last_mru_index < sz - 1) {
            parser->reset_tab_completion();
            last_mru_index++;
            content = mru[last_mru_index];
            curx = get_utf8_length(content);
            draw();
        }
    }
}

void EntryWidget::mru_down() {
    if (!numbers_only) {
        if (last_mru_index > 0) {
            parser->reset_tab_completion();
            last_mru_index--;
            content = mru[last_mru_index];
            curx = get_utf8_length(content);
        } else if (!last_mru_index) {
            last_mru_index = -1;
            content = last_text;
            curx = last_cursor_pos;
        }
        draw();
    }
}

void EntryWidget::set_numbers_only(bool state) {
    numbers_only = state;
    if (numbers_only) {
        content.clear();
        last_text.clear();
        mru.clear();
        last_mru_index = -1;
        last_cursor_pos = 0;
    }
}

void EntryWidget::set_info_bar(const std::string& info_left, const std::string& info_selected, const std::string& info_right, bool redraw) {
    this->info_left = info_left;
    this->info_selected = info_selected;
    this->info_right = info_right;
    if (redraw) {
        draw();
    }
}

void EntryWidget::reset_mru_tab() {
    if (!numbers_only) {
        parser->reset_tab_completion();
        last_text = content;
        last_mru_index = -1;
        last_cursor_pos = curx;
    }
}

void EntryWidget::delete_ncurses_object() {
    if (configured) {
        delwin(win_label);
        delwin(win_input);
        configured = false;
    }
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_insert_sequence(const std::string& sequence) {
    size_t advance = get_utf8_length(sequence);
    size_t sz = get_utf8_length(content);
    if (static_cast<int>(sz + advance) <= maxlen) {
        UTF8Iterator it(content.begin(), curx);
        std::string::size_type pos = it.get_string_iterator() - content.begin();
        content.insert(pos, sequence);
        curx += advance;
        reset_mru_tab();
        draw();
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_delete_word() {
    bool in_word = false;
    if (curx) {
        while (curx) {
            curx--;
            UTF8Iterator it(content.begin(), curx);
            if (in_word) {
                if (it.get_sequence() == " ") {
                    curx++;
                    break;
                }
            } else {
                in_word = (it.get_sequence() != " ");
            }
            std::string::size_type pos = it.get_string_iterator() - content.begin();
            content.erase(pos, it.get_sequence().length());
        }
        reset_mru_tab();
        draw();
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_delete() {
    UTF8Iterator it(content.begin(), curx);
    if (it != content.end()) {
        std::string::size_type pos = it.get_string_iterator() - content.begin();
        content.erase(pos, it.get_sequence().length());
        reset_mru_tab();
        draw();
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_enter() {
    last_text.clear();
    if (!numbers_only) {
        parser->reset_tab_completion();
        if (content.length()) {
            if (!mru.size() || mru[0] != content) {
                mru.push_front(content);
                last_mru_index = -1;
                while (static_cast<int>(mru.size()) > max_mru) {
                    mru.pop_back();
                }
            }
        }
    }

    return EntryWidgetEventEnter;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_backspace() {
    if (curx) {
        curx--;
        UTF8Iterator it(content.begin(), curx);
        if (it != content.end()) {
            std::string::size_type pos = it.get_string_iterator() - content.begin();
            content.erase(pos, it.get_sequence().length());
            reset_mru_tab();
            draw();
        }
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_escape() {
    return EntryWidgetEventEscape;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_up() {
    return EntryWidgetEventUp;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_down() {
    return EntryWidgetEventDown;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_right() {
    if (numbers_only) {
        return EntryWidgetEventRight;
    }

    size_t sz = get_utf8_length(content);
    if (curx < static_cast<int>(sz)) {
        curx++;
        reset_mru_tab();
        set_cursor();
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_left() {
    if (numbers_only) {
        return EntryWidgetEventLeft;
    }

    if (curx) {
        curx--;
        reset_mru_tab();
        set_cursor();
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_home() {
    if (!numbers_only) {
        if(curx > 0) {
            curx = 0;
            reset_mru_tab();
            set_cursor();
        }
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_end() {
    if (!numbers_only) {
        size_t sz = get_utf8_length(content);
        if (curx < static_cast<int>(sz)) {
            curx = sz;
            reset_mru_tab();
            set_cursor();
        }
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_pgup() {
    return EntryWidgetEventPageUp;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_pgdn() {
    return EntryWidgetEventPageDown;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_tab() {
    return EntryWidgetEventTab;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_shift_tab() {
    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_ctrl_left() {
    if (!numbers_only) {
        if (curx) {
            UTF8Iterator it(content.begin(), curx);
            bool in_word = false;
            while (it != content.begin()) {
                curx--;
                it--;
                if (in_word) {
                    if (it.get_sequence() == " ") {
                        curx++;
                        break;
                    }
                } else {
                    in_word = (it.get_sequence() != " ");
                }
            }
            reset_mru_tab();
            set_cursor();
        }
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_ctrl_right() {
    if (!numbers_only) {
        size_t sz = get_utf8_length(content);
        if (curx < static_cast<int>(sz)) {
            UTF8Iterator it(content.begin(), curx);
            bool in_word = false;
            while (it != content.end()) {
                if (in_word) {
                    if (it.get_sequence() == " ") {
                        break;
                    }
                } else {
                    in_word = (it.get_sequence() != " ");
                }
                it++;
                curx++;
            }
            reset_mru_tab();
            set_cursor();
        }
    }

    return EntryWidgetEventNone;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_ctrl_pgup() {
    return EntryWidgetEventPreviousWindow;
}

EntryWidget::EntryWidgetEvent EntryWidget::keyevent_ctrl_pgdn() {
    return EntryWidgetEventNextWindow;
}
