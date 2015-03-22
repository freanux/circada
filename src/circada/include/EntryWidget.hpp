/*
 *  EntryWidget.hpp
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

#ifndef _ENTRYWIDGET_HPP_
#define _ENTRYWIDGET_HPP_

#include "Terminal.hpp"

#include <Circada/Circada.hpp>
#include <deque>
#include <string>

class EntryWidget {
public:
    enum EntryWidgetEvent {
        EntryWidgetEventNone = 0,
        EntryWidgetEventResize,
        EntryWidgetEventEnter,
        EntryWidgetEventPageUp,
        EntryWidgetEventPageDown,
        EntryWidgetEventNextWindow,
        EntryWidgetEventPreviousWindow,
        EntryWidgetEventTab,
        EntryWidgetEventUp,
        EntryWidgetEventDown,
        EntryWidgetEventLeft,
        EntryWidgetEventRight,
        EntryWidgetEventEscape,
        EntryWidgetEventModeKey
    };

    EntryWidget(Circada::Mutex& draw_mtx);
    virtual ~EntryWidget();

    void configure(const std::string& label, int posx, int posy, int width, int maxlen);
    void set_label(const std::string& label);
    const std::string& get_content();
    char get_mode_key();
    EntryWidgetEvent input();
    void reset();
    void draw();
    void set_parser(Circada::Parser *parser);
    int get_max_mru() const;
    void set_max_mru(int max);
    void set_cursor();
    void complete_tab();
    void mru_up();
    void mru_down();
    void set_numbers_only(bool state);
    void set_info_bar(const std::string& info_left, const std::string& info_selected, const std::string& info_right, bool redraw);

private:
    typedef std::deque<std::string> MRU;
    typedef EntryWidgetEvent (EntryWidget::*KeyEvent)();
    struct EscapeKeyEvent {
        const char *sequence;
        KeyEvent event;
    };

    static EscapeKeyEvent keyevents[];

    Circada::Mutex& draw_mtx;
    bool configured;
    int curx;
    int ofsx;
    Circada::Parser *parser;
    int last_mru_index;
    int max_mru;
    bool numbers_only;
    char mode_key;
    std::string last_text;
    int last_cursor_pos;
    MRU mru;

    WINDOW *win_label;
    WINDOW *win_input;
    std::string label;
    std::string content;
    int label_length;
    int posx, posy;
    int width;
    int maxlen;
    std::string info_left;
    std::string info_selected;
    std::string info_right;

    void reset_mru_tab();
    void delete_ncurses_object();

    /* escape key events */
    EntryWidgetEvent keyevent_insert_sequence(const std::string& sequence);
    EntryWidgetEvent keyevent_delete_word();
    EntryWidgetEvent keyevent_delete();
    EntryWidgetEvent keyevent_enter();
    EntryWidgetEvent keyevent_backspace();
    EntryWidgetEvent keyevent_escape();
    EntryWidgetEvent keyevent_up();
    EntryWidgetEvent keyevent_down();
    EntryWidgetEvent keyevent_right();
    EntryWidgetEvent keyevent_left();
    EntryWidgetEvent keyevent_home();
    EntryWidgetEvent keyevent_end();
    EntryWidgetEvent keyevent_pgup();
    EntryWidgetEvent keyevent_pgdn();
    EntryWidgetEvent keyevent_tab();
    EntryWidgetEvent keyevent_shift_tab();
    EntryWidgetEvent keyevent_ctrl_left();
    EntryWidgetEvent keyevent_ctrl_right();
    EntryWidgetEvent keyevent_ctrl_pgup();
    EntryWidgetEvent keyevent_ctrl_pgdn();
};

#endif // _ENTRYWIDGET_HPP_
