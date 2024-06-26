/*
 *  Application.cpp
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

#include "Application.hpp"
#include "UTF8.hpp"

#include <unistd.h>
#include <cstdlib>
#include <algorithm>

namespace {

    void fill_opt(const Configuration& config, const std::string& category, const std::string& key, std::string& into) {
        const std::string& value = config.get_value(category, key);
        if (value.length()) {
            into = value;
        } else {
            const std::string& value = config.get_value("", key);
            if (value.length()) {
                into = value;
            }
        }
    }

}

Application::Application(Configuration& config)
    : IrcClient(config), config(config), nicklist_width(0), selected_window(0),
      entry_widget(draw_mtx), number_widget(draw_mtx), status_widget(windows),
      text_widget(status_widget), window_sequence(0), input_numbers(false),
      number_input_sign("%"), windowbar_separator("│"), nicklist_visible(true),
      treeview_visible(true), highlightwindow_visible(false)
{
    /* set to system default locale. ensure,       */
    /* that you have UTF-8 as globallocale set up. */
    setlocale(LC_ALL, "");

    /* setup ncurses */
    win_main = initscr();
    start_color();
    use_default_colors();
    raw();
    noecho();
    set_escdelay(0);

    /* setup standard colors */
    for (int i = 0; i < 256; i++) {
        int fg = i & 0x0f;
        int bg = i >> 4;
        if (!bg) bg = -1;
        init_pair(i, fg, bg);
    }
    /* setup standard incoming encoding rules. outgoing is always UTF-8 */
    encodings.push_back("UTF-8");
    encodings.push_back("CP1252");
    encodings.push_back("ISO-8859-1");

    /* setup entry widgets */
    entry_widget.set_parser(this);
    number_widget.set_numbers_only(true);

    /* resize all widgets */
    configure();

    /* lua */
    lua_setup();

    /* create application window */
    create_application_window(get_project_name(), get_project_name());

}

Application::~Application() {
    endwin();
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        delete *it;
    }
}

void Application::run() {
    /* startup script lua */
    try {
        std::string lua_file = config.get_value("", "script");
        if (lua_file.length()) {
            lua.safe_script_file(lua_file);
        }
    } catch (const sol::error& e) {
        lua_error(e);
    }

    /* loop */
    time_t old_time = 0;
    running = true;
    while (running) {
        time_t new_time = time(0);
        if (new_time != old_time) {
            old_time = new_time;
            ScopeMutex lock(&draw_mtx);
            status_widget.draw_time(new_time);
            status_widget.refresh();
            set_cursor();
        }
        if (input_numbers) {
            /* navigation mode */
            switch (number_widget.input()) {
                case EntryWidget::EntryWidgetEventEscape:
                    number_widget.reset();
                    input_numbers = false;
                    entry_widget.draw();
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventResize:
                    configure();
                    break;

                case EntryWidget::EntryWidgetEventEnter:
                {
                    ScreenWindow *sw = 0;
                    {
                        ScopeMutex lock(&draw_mtx);
                        input_numbers = false;
                        int nbr = atoi(number_widget.get_content().c_str()) - 1;
                        int sz = static_cast<int>(windows.size());
                        if (nbr >= 0 && nbr < sz) {
                            sw = windows[nbr];
                        }
                    }
                    number_widget.reset();
                    input_numbers = false;
                    if (sw) {
                        select_window(sw);
                    }
                    entry_widget.draw();
                    set_cursor();
                    break;
                }

                case EntryWidget::EntryWidgetEventPageUp:
                    {
                        ScopeMutex lock(&draw_mtx);
                        text_widget.scroll_up();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventPageDown:
                    {
                        ScopeMutex lock(&draw_mtx);
                        text_widget.scroll_down();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventLeft:
                    select_prev_window();
                    break;

                case EntryWidget::EntryWidgetEventRight:
                    select_next_window();
                    break;

                case EntryWidget::EntryWidgetEventUp:
                    {
                        ScopeMutex lock(&draw_mtx);
                        nicklist_widget.scroll_up();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventDown:
                    {
                        ScopeMutex lock(&draw_mtx);
                        nicklist_widget.scroll_down();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventPreviousWindow:
                    select_prev_window();
                    break;

                case EntryWidget::EntryWidgetEventNextWindow:
                    select_next_window();
                    break;

                case EntryWidget::EntryWidgetEventModeKey:
                    switch (number_widget.get_mode_key()) {
                        case 'n':
                        case 'N':
                            nicklist_visible = !nicklist_visible;
                            configure();
                            break;

                        case 'c':
                        case 'C':
                            treeview_visible = !treeview_visible;
                            configure();
                            break;

                        default:
                            /* nothing */
                            break;
                    }
                    break;

                default:
                    /* nothing */
                    break;
            }
        } else {
            /* normal input mode */
            switch (entry_widget.input()) {
                case EntryWidget::EntryWidgetEventEscape:
                    input_numbers = true;
                    number_widget.draw();
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventResize:
                    configure();
                    break;

                case EntryWidget::EntryWidgetEventEnter:
                    parse_entry();
                    break;

                case EntryWidget::EntryWidgetEventPageUp:
                    {
                        ScopeMutex lock(&draw_mtx);
                        text_widget.scroll_up();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventPageDown:
                    {
                        ScopeMutex lock(&draw_mtx);
                        text_widget.scroll_down();
                    }
                    set_cursor();
                    break;

                case EntryWidget::EntryWidgetEventPreviousWindow:
                    select_prev_window();
                    break;

                case EntryWidget::EntryWidgetEventNextWindow:
                    select_next_window();
                    break;

                case EntryWidget::EntryWidgetEventTab:
                    entry_widget.complete_tab();
                    break;

                case EntryWidget::EntryWidgetEventUp:
                    entry_widget.mru_up();
                    break;

                case EntryWidget::EntryWidgetEventDown:
                    entry_widget.mru_down();
                    break;

                default:
                    /* nothing */
                    break;
            }
        }
    }
}

void Application::configure() {
    /* get current terminal window size */
    getmaxyx(win_main, height, width);

    /* setup widgets */
    {
        ScopeMutex lock(&draw_mtx);

        /* determine nicklist width */
        nicklist_width = 0;
        if (selected_window && nicklist_visible) {
            Circada::Session *cs = selected_window->get_circada_session();
            if (cs && selected_window->get_circada_window()->get_window_type() == WindowTypeChannel) {
                nicklist_width = cs->get_nicklen() + 1 + get_utf8_length(nicklist_widget.get_nicklist_delimiter());
                nicklist_widget.configure(1, width - nicklist_width, nicklist_width, height - 3);
            }
        }

        /* determine treeview width */
        int treeview_width = 0;
        if (treeview_visible) {
            treeview_width = treeview_widget.get_estimated_width();
            treeview_widget.configure(height - 3, treeview_width, 1, 0);
        }
        topic_widget.configure(width);
        text_widget.configure(height - 2, width - nicklist_width - treeview_width, treeview_width);
        status_widget.configure(height - 2, width);
        entry_widget.configure("", 0, height - 1, width, 1024);
        number_widget.configure(number_input_sign, 0, height - 1, width, 4);
    }

    /* refresh and draw */
    refresh();
    draw();
}

void Application::draw() {
    {
        ScopeMutex lock(&draw_mtx);
        curs_set(0);
        topic_widget.draw();
        text_widget.draw();
        if (nicklist_width) {
            nicklist_widget.draw(selected_window);
        }
        if (treeview_visible) {
            treeview_widget.draw(selected_window);
        }
        status_widget.draw();
        curs_set(1);
    }
    if (input_numbers) {
        number_widget.draw();
    } else {
        entry_widget.draw();
    }
    set_cursor();
}

void Application::parse_entry() {
    std::string line = entry_widget.get_content();
    entry_widget.reset();
    ScreenWindow *sw = text_widget.get_selected_window();
    Session *s = sw->get_circada_session();
    Window *w = sw->get_circada_window();
    try {
        bool external;
        std::string ctcp;
        std::string msg;
        std::string output = entry_parser.parse(s, w, line, external);
        if (external) {
            execute(output);
        } else if (s) {
            if (w->get_window_type() == WindowTypeAlerts) {
                print(sw, "You cannot send message into this window.");
            } else {
                s->send(output);
            }
        } else if (w->get_window_type() == WindowTypeDCC) {
            DCCHandle dcc = get_dcc_handle_from_window(w);
            dcc_send_msg(dcc, output);
            std::string ctcp;
            std::string msg;
            if (output.length() && output[0] == '\x01') {
                msg = output.substr(1);
                size_t pos = msg.find(' ');
                if (pos != std::string::npos) {
                    ctcp = msg.substr(0, pos);
                    msg = msg.substr(pos + 1);
                    if (msg.length()) {
                        msg = msg.substr(0, msg.length() - 1);
                    }
                }
            } else {
                msg = output;
            }

            ScopeMutex lock(&draw_mtx);
            std::string line;
            fmt.append_dcc_msg(get_now(), dcc.get_my_nick(), true, ctcp, msg, line);
            sw->add_formatted_line(line);
            text_widget.draw_line(sw, line);
            text_widget.refresh(sw);
            set_cursor();
        } else {
            print(sw, "Change into running connection.");
        }
    } catch (const Exception& e) {
        print(sw, e.what());
    }
}

void Application::print(ScreenWindow *w, const std::string& what) {
    ScopeMutex lock(&draw_mtx);
    std::string line;
    std::string temp;
    std::string timestamp = get_now();

    temp.clear();
    fmt.append_format(what, fmt.fmt_print, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);
    text_widget.refresh(w);
    set_cursor();
}

void Application::set_cursor() {
    if (input_numbers) {
        number_widget.set_cursor();
    } else {
        entry_widget.set_cursor();
    }
}

void Application::update_input_infobar() {
    /* the caller has to lock mutex */
    std::string l, s, r;
    if (selected_window) {
        size_t sz = windows.size();
        char buf[16];
        bool left = true;
        for (size_t i = 0; i < sz; i++) {
            ScreenWindow *sw = windows[i];
            const std::string *name = &sw->get_circada_window()->get_name();
            //sprintf(buf, "%lu", i + 1);
            sprintf(buf, "%u", static_cast<int>(i + 1));
            if (selected_window == sw) {
                s = " ";
                s += buf;
                s += " " + *name + " ";
                left = false;
            } else if (left) {
                l += " ";
                l += buf;
                l += " " + *name + " " + windowbar_separator;
            } else {
                r += windowbar_separator + " ";
                r += buf;
                r += " " + *name + " ";
            }
        }
        number_widget.set_info_bar(l, s, r, input_numbers);
    }
    build_window_tree();
}

void Application::append_welcome_message(ScreenWindow *w) {
    char buffer[128];

    ScopeMutex lock(&draw_mtx);
    std::string line;
    std::string temp;
    std::string timestamp = get_now();

    temp.clear();
    fmt.append_format("          welcome to", fmt.fmt_logo_text, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.append_format("      _                 __", fmt.fmt_logo, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.append_format(" ____(_)__________ ____/ /__ _", fmt.fmt_logo, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.append_format("/ __/ / __/ __/ _ `/ _  / _ `/", fmt.fmt_logo, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.append_format("\\__/_/_/  \\__/\\_,_/\\_,_/\\_,_/", fmt.fmt_logo, temp);
    fmt.append_format(" (v" + get_project_version() + ")", fmt.fmt_logo_text, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    sprintf(buffer, "compiled on %s %s", __DATE__, __TIME__);
    fmt.append_format(buffer, fmt.fmt_logo_addition, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    sprintf(buffer, "%d colors supported", COLOR_PAIRS - 1);
    fmt.append_format(buffer, fmt.fmt_logo_text, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    temp.clear();
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);

    text_widget.refresh(w);

    set_cursor();
}

void Application::execute(const std::string& line) {
    /* quick and dirty implementation of internal commands */
    std::string command;
    std::string params;
    size_t pos = line.find(' ');
    if (pos == std::string::npos) {
        pos = line.length();
    } else {
        params = line.substr(pos + 1);
    }

    command = line.substr(0, pos);
    if (is_equal(command.c_str(), "connect")) {
        execute_connect(params);
    } else if (is_equal(command.c_str(), "clear")) {
        execute_clear(params);
    } else if (is_equal(command.c_str(), "close")) {
        execute_close(params);
    } else if (is_equal(command.c_str(), "dcc")) {
        execute_dcc(params);
    } else if (is_equal(command.c_str(), "disconnect")) {
        execute_disconnect(params);
    } else if (is_equal(command.c_str(), "query")) {
        execute_query(params);
    } else if (is_equal(command.c_str(), "quit")) {
        execute_quit(params);
    } else if (is_equal(command.c_str(), "save")) {
        execute_save(params);
    } else if (is_equal(command.c_str(), "set")) {
        execute_set(params);
    } else if (is_equal(command.c_str(), "get")) {
        execute_get(params);
    } else if (is_equal(command.c_str(), "sort")) {
        execute_sort(params);
    } else if (is_equal(command.c_str(), "netsplits")) {
        execute_netsplits(params);
    } else if (is_equal(command.c_str(), "lua")) {
        execute_lua(params);
    }
}

void Application::split(const std::string& from, Params& into, int max_params) {
    std::string str = from;
    size_t pos = 0;
    int parmcount = 0;
    while (str.length()) {
        parmcount++;
        if ((pos = str.find(" ", 0)) != std::string::npos && (!max_params || parmcount < max_params)) {
            into.push_back(str.substr(0, pos));
            str = str.substr(pos + 1);
        } else {
            into.push_back(str);
            break;
        }
    }
}

void Application::execute_connect(const std::string& params) {
    Params p;
    split(params, p, 1);

    size_t sz = p.size();
    if (sz) {
        try {
            SessionOptions opts;

            const std::string& name = config.get_value(p[0], "name");
            if (name.length()) opts.name = name;

            opts.server = config.get_value(p[0], "server", p[0]);
            opts.port = atoi(config.get_value(p[0], "port", "6667").c_str());

            ::fill_opt(config, p[0], "nick", opts.nick);
            ::fill_opt(config, p[0], "alternative_nick", opts.alternative_nick);
            ::fill_opt(config, p[0], "user", opts.user);
            ::fill_opt(config, p[0], "real_name", opts.real_name);
            ::fill_opt(config, p[0], "ca_file", opts.ca_file);
            ::fill_opt(config, p[0], "cert_file", opts.cert_file);
            ::fill_opt(config, p[0], "key_file", opts.key_file);
            ::fill_opt(config, p[0], "tls_priority", opts.tls_priority);

            Session *s = create_session(opts);
            s->connect();
        } catch (const Exception& e) {
            ScopeMutex lock(&draw_mtx);
            ScreenWindow *sw = get_window_nolock(get_application_window());
            print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
            text_widget.refresh(sw);
            set_cursor();
        }
    } else {
        ScopeMutex lock(&draw_mtx);
        ScreenWindow *sw = get_window_nolock(get_application_window());
        print_line(sw, get_now(), "Insufficient parameters.", fmt.fmt_info_normal);
        text_widget.refresh(sw);
        set_cursor();
    }
}

void Application::execute_clear(const std::string& params) {
    ScopeMutex lock(&draw_mtx);
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        sw->get_circada_window()->reset_action();
    }
    status_widget.draw();
    if (treeview_visible) {
        treeview_widget.draw(selected_window);
    }
    set_cursor();
}

void Application::execute_close(const std::string& params) {
    Window *w = selected_window->get_circada_window();
    try {
        unquery(w);
    } catch (const Exception& e) {
        ScopeMutex lock(&draw_mtx);
        ScreenWindow *sw = get_window_nolock(w);
        print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
        text_widget.refresh(sw);
        set_cursor();
    }
}

void Application::execute_disconnect(const std::string& params) {
    Session *s = selected_window->get_circada_session();
    if (s) {
        destroy_session(s);
    } else {
        ScopeMutex lock(&draw_mtx);
        ScreenWindow *sw = get_window_nolock(get_application_window());
        print_line(sw, get_now(), "Change into a running connection.", fmt.fmt_info_normal);
        text_widget.refresh(sw);
        set_cursor();
    }
}

void Application::execute_dcc(const std::string& params) {
    Params p;
    split(params, p);
    char buffer[128];
    std::string timestamp = get_now();
    ScreenWindow *into = 0;
    Session *s = selected_window->get_circada_session();

    if (s) {
        into = get_server_window(s);
    } else {
        into = get_window(get_application_window());
        s = 0;
    }

    if (!p.size()) {
        /* list all dccs */
        ScopeMutex lock(&draw_mtx);
        into->dcc_handles.clear();
        if (s) {
            into->dcc_handles = s->get_dcc_list();
            Window *w = s->get_server_window();
            into = get_window_nolock(w);
            print_line(into, timestamp, "List of all DCCs in this session:", fmt.fmt_text_bold);
        } else {
            into->dcc_handles = get_dcc_list();
            print_line(into, timestamp, "List of all DCCs:", fmt.fmt_text_bold);
        }
        size_t sz = into->dcc_handles.size();
        for (size_t i = 0; i < sz; i++) {
            std::string line;
            const DCCHandle& handle = into->dcc_handles[i];
            sprintf(buffer, "%d", static_cast<int>(i) + 1);

            fmt.append_format(" ", fmt.fmt_dcc_info, line);
            fmt.append_format(buffer, fmt.fmt_dcc_bold, line);
            fmt.append_format(": ", fmt.fmt_dcc_info, line);

            if (handle.get_type() == DCCTypeChat) {
                fmt.append_format("CHAT", fmt.fmt_dcc_bold, line);
                fmt.append_format(" with ", fmt.fmt_dcc_info, line);
                fmt.append_format(handle.get_his_nick(), fmt.fmt_dcc_bold, line);
                if (handle.get_direction() == DCCDirectionIncoming)  {
                    fmt.append_format(", offered", fmt.fmt_dcc_info, line);
                }
            } else {
                DCCXferHandle xfer = get_xfer_handle(handle);

                fmt.append_format("XFER", fmt.fmt_dcc_bold, line);
                if (xfer.get_direction() == DCCDirectionIncoming)  {
                    fmt.append_format(" to ", fmt.fmt_dcc_info, line);
                } else {
                    fmt.append_format(" from ", fmt.fmt_dcc_info, line);
                }
                fmt.append_format(handle.get_his_nick(), fmt.fmt_dcc_bold, line);

                if (handle.get_direction() == DCCDirectionIncoming)  {
                    fmt.append_format(", offered", fmt.fmt_dcc_info, line);
                }
                fmt.append_format(" (", fmt.fmt_dcc_info, line);
                fmt.append_format(xfer.get_filename(), fmt.fmt_dcc_bold, line);

                sprintf(buffer, "%u bytes", xfer.get_filesize());
                fmt.append_format(", ", fmt.fmt_dcc_info, line);
                fmt.append_format(buffer, fmt.fmt_dcc_info, line);

                if (handle.is_running()) {
                    sprintf(buffer, "%u bytes transferred", xfer.get_transferred_bytes());
                    fmt.append_format(", ", fmt.fmt_dcc_info, line);
                    fmt.append_format(buffer, fmt.fmt_dcc_info, line);
                }

                fmt.append_format(")", fmt.fmt_dcc_info, line);
            }
            fmt.append_format(" - ", fmt.fmt_dcc_info, line);
            fmt.append_format((handle.is_running() ? (handle.is_connected() ? "RUNNING" : "WAIT") : "HELD"), fmt.fmt_dcc_bold, line);
            print_line(into, timestamp, line);
        }
        print_line(into, timestamp, "End of list", fmt.fmt_text_normal);
        text_widget.refresh(into);
        set_cursor();
    } else if (is_equal(p[0], "accept") && p.size() == 2) {
        int index = atoi(p[1].c_str()) - 1;
        if (index < 0 || index >= static_cast<int>(into->dcc_handles.size())) {
            print_line(into, timestamp, "Invalid DCC selected.", fmt.fmt_text_normal);
        } else {
            DCCHandle dcc = into->dcc_handles[index];
            try {
                dcc_accept(dcc);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
        }
        text_widget.refresh(into);
        set_cursor();
    } else if (is_equal(p[0], "force") && p.size() == 2) {
        int index = atoi(p[1].c_str()) - 1;
        if (index < 0 || index >= static_cast<int>(into->dcc_handles.size())) {
            print_line(into, timestamp, "Invalid DCC selected.", fmt.fmt_text_normal);
        } else {
            DCCHandle dcc = into->dcc_handles[index];
            try {
                dcc_force(dcc);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
        }
        text_widget.refresh(into);
        set_cursor();
    } else if (is_equal(p[0], "decline") && p.size() == 2) {
        int index = atoi(p[1].c_str()) - 1;
        if (index < 0 || index >= static_cast<int>(into->dcc_handles.size())) {
            print_line(into, timestamp, "Invalid DCC selected.", fmt.fmt_text_normal);
        } else {
            DCCHandle dcc = into->dcc_handles[index];
            try {

                dcc_decline(dcc);
                print_line(into, timestamp, "DCC request declined.", fmt.fmt_dcc_info);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
        }
        text_widget.refresh(into);
        set_cursor();
    } else if (is_equal(p[0], "abort") && p.size() == 2) {
        int index = atoi(p[1].c_str()) - 1;
        if (index < 0 || index >= static_cast<int>(into->dcc_handles.size())) {
            print_line(into, timestamp, "Invalid DCC selected.", fmt.fmt_text_normal);
        } else {
            DCCHandle dcc = into->dcc_handles[index];
            try {
                Window *w = get_window_from_dcc_handle(dcc);
                if (w) {
                    send_dcc_abort(w, dcc, "Operation aborted.");
                }
                dcc_abort(dcc);
                print_line(into, timestamp, "DCC request aborted.", fmt.fmt_dcc_info);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
        }
        text_widget.refresh(into);
        set_cursor();
    } else if (is_equal(p[0], "chat") && p.size() == 2) {
        if (!s) {
            print(into, "Change into running connection.");
        } else {
            ScopeMutex lock(&draw_mtx);
            try {
                DCCChatHandle dcc = s->dcc_chat_offer(p[1]);
                std::string info;
                fmt.append_format("You offered a ", fmt.fmt_dcc, info);
                fmt.append_format("DCC CHAT", fmt.fmt_dcc_bold, info);
                fmt.append_format(" request to ", fmt.fmt_dcc, info);
                fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
                print_line(into, get_now(), info);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
            text_widget.refresh(into);
            set_cursor();
        }
    } else if (is_equal(p[0], "send") && p.size() == 3) {
        if (!s) {
            print(into, "Change into running connection.");
        } else {
            ScopeMutex lock(&draw_mtx);
            try {
                DCCXferHandle dcc = s->dcc_file_offer(p[1], p[2]);
                std::string info;
                fmt.append_format("You offered a ", fmt.fmt_dcc, info);
                fmt.append_format("DCC XFER", fmt.fmt_dcc_bold, info);
                fmt.append_format(" request to ", fmt.fmt_dcc, info);
                fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
                fmt.append_format(" (", fmt.fmt_dcc, info);
                fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_bold, info);
                sprintf(buffer, "%u bytes", dcc.get_filesize());
                fmt.append_format(", ", fmt.fmt_dcc_info, info);
                fmt.append_format(buffer, fmt.fmt_dcc_info, info);
                fmt.append_format(")", fmt.fmt_dcc, info);
                print_line(into, get_now(), info);
            } catch (const Exception& e) {
                print_line(into, timestamp, e.what(), fmt.fmt_dcc_fail);
            }
            text_widget.refresh(into);
            set_cursor();
        }
    } else {
        print_line(into, timestamp, "Insufficient parameters.", fmt.fmt_dcc_fail);
    }
}

void Application::execute_query(const std::string& params) {
    Params p;
    split(params, p);

    size_t sz = p.size();
    if (!sz) {
        execute_close(params);
    } else if (sz == 1) {
        Window *w = selected_window->get_circada_window();
        Session *s = selected_window->get_circada_session();
        try {
            Window *qw = query(s, p[0]);
            select_window(get_window(qw));
        } catch (const Exception& e) {
            ScopeMutex lock(&draw_mtx);
            ScreenWindow *sw = get_window_nolock(w);
            print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
            text_widget.refresh(sw);
            set_cursor();
        }
    }
}

void Application::execute_quit(const std::string& params) {
    running = false;
}

void Application::execute_save(const std::string& params) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(get_application_window());
    try {
        config.save();
        print_line(sw, get_now(), "Configuration saved.", fmt.fmt_info_normal);
    } catch (const Exception& e) {
        print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
    }
    text_widget.refresh(sw);
    set_cursor();
}

void Application::execute_set(const std::string& params) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(get_application_window());
    Params p;
    split(params, p, 2);

    size_t sz = p.size();
    if (sz) {
        try {
            std::string category;
            std::string key = p[0];
            std::string value;
            size_t pos = key.find('.');
            if (pos != std::string::npos) {
                category = key.substr(0, pos);
                key = key.substr(pos + 1);
            }

            if (sz == 2) {
                value = p[1];
            }

            config.set_value(category, key, value);
            print_line(sw, get_now(), p[0] + "="  + value, fmt.fmt_info_normal);
        } catch (const Exception& e) {
            print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
        }
    } else {
        print_line(sw, get_now(), "Insufficient parameters.", fmt.fmt_info_normal);
    }
    text_widget.refresh(sw);
    set_cursor();
}

void Application::execute_get(const std::string& params) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(get_application_window());
    Params p;
    split(params, p, 1);

    size_t sz = p.size();
    if (sz) {
        try {
            std::string category;
            std::string key = p[0];
            size_t pos = key.find('.');
            if (pos != std::string::npos) {
                category = key.substr(0, pos);
                key = key.substr(pos + 1);
            }
            print_line(sw, get_now(), p[0] + "=" + config.get_value(category, key), fmt.fmt_info_normal);
        } catch (const Exception& e) {
            print_line(sw, get_now(), e.what(), fmt.fmt_info_normal);
        }
    } else {
        /* list all entries */
        const Configuration::Entries& entries = config.get_entries();
        std::string timestamp(get_now());
        print_line(sw, timestamp, "--- start of list ---", fmt.fmt_info_normal);
        for (Configuration::Entries::const_iterator it = entries.begin(), end = entries.end(); it != end; ++it) {
            print_line(sw, timestamp, it->first + "=" + it->second, fmt.fmt_info_normal);
        }
        print_line(sw, timestamp, "--- end of list ---", fmt.fmt_info_normal);
    }
    text_widget.refresh(sw);
    set_cursor();
}

void Application::execute_sort(const std::string& params) {
    {
        ScopeMutex lock(&draw_mtx);
        std::sort(windows.begin(), windows.end(), ScreenWindowComparer());
        update_input_infobar();
    }
    configure();
    set_cursor();
}

void Application::execute_netsplits(const std::string& params) {
    ScopeMutex lock(&draw_mtx);
    if (selected_window) {
        char buffer[128];
        const Netsplits& netsplits = selected_window->get_circada_window()->get_netsplits();
        std::string timestamp(get_now());
        for (Netsplits::const_iterator it = netsplits.begin(); it != netsplits.end(); it++) {
            const Netsplit& ns = it->second;
            sprintf(buffer, "%s: %d nicks", it->first.c_str(), static_cast<int>(ns.nicks.size()));
            print_line(selected_window, timestamp, buffer, fmt.fmt_info_normal);
        }
        print_line(selected_window, timestamp, "--- end of netsplit list ---", fmt.fmt_info_normal);
        text_widget.refresh(selected_window);
    }
    set_cursor();
}

void Application::execute_lua(const std::string& params) {
    try {
        lua.script(params);
    } catch (const sol::error& e) {
        lua_error(e);
    }
    set_cursor();
}

void Application::print_line(ScreenWindow *w, const std::string& timestamp, const std::string& what, Format& format) {
    std::string line;
    std::string temp;
    fmt.append_format(what, format, temp);
    fmt.plaintext(timestamp, temp, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);
    if (w != selected_window) {
        if (w->get_circada_window()->set_action(WindowActionAlert)) {
            beep();
            if (treeview_visible) {
                treeview_widget.draw(selected_window);
            }
            status_widget.draw();
            set_cursor();
        }
    }
}

void Application::print_line(ScreenWindow *w, const std::string& timestamp, const std::string& what) {
    std::string line;
    std::string temp;
    fmt.plaintext(timestamp, what, line);
    w->add_formatted_line(line);
    text_widget.draw_line(w, line);
    if (w != selected_window) {
        if (w->get_circada_window()->set_action(WindowActionAlert)) {
            beep();
            if (treeview_visible) {
                treeview_widget.draw(selected_window);
            }
            status_widget.draw();
            set_cursor();
        }
    }
}

void Application::send_dcc_abort(Window *w, DCCHandle dcc, const std::string& reason) {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    std::string info;

    if (dcc.get_type() == DCCTypeChat) {
        fmt.append_format("DCC CHAT with ", fmt.fmt_dcc, info);
        fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
        fmt.append_format(" ended.", fmt.fmt_dcc, info);
    } else {
        fmt.append_format("DCC XFER with ", fmt.fmt_dcc, info);
        fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
        fmt.append_format(" aborted.", fmt.fmt_dcc_fail, info);
    }

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::build_window_tree() {
    /* calculate width */
    size_t maxlen = 0;
    size_t sz = windows.size();
    size_t len;
    for (size_t i = 0; i < sz; i++) {
        ScreenWindow *sw = windows[i];
        if (sw->get_circada_window()->get_window_type() != WindowTypeServer) {
            len = sw->get_circada_window()->get_name().length() + 4;
        } else {
            len = sw->get_circada_session()->get_server().length() + 2;
        }
        if (len > maxlen) {
            maxlen = len;
        }
    }

    /* build tree */
    TreeViewWidget::TreeView& tv = treeview_widget.get_treeview();
    tv.clear();
    std::string sep;

    /* application windows first */
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        Window *w = sw->get_circada_window();
        if (w->get_window_type() == WindowTypeApplication) {
            tv.push_back(TreeViewEntry(sw, true, "+-", make_tree_nr(it) + w->get_name()));
        }
    }

    /* running dccs */
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        Window *w = sw->get_circada_window();
        if (w->get_window_type() == WindowTypeDCC) {
            tv.push_back(TreeViewEntry(sw, true, "+-", make_tree_nr(it) + w->get_name()));
        }
    }

    /* now connections */
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        ScreenWindow *sw = *it;
        Window *w = sw->get_circada_window();
        if (w->get_window_type() == WindowTypeServer) {
            tv.push_back(TreeViewEntry(sw, false, "+-", w->get_name()));

            bool last_server = true;
            for (ScreenWindow::List::iterator lit = it + 1; lit != windows.end(); lit++) {
                ScreenWindow *sw = *lit;
                if (sw->get_circada_window()->get_window_type() == WindowTypeServer) {
                    last_server = false;
                    break;
                }
            }

            Session *s = sw->get_circada_session();
            for (ScreenWindow::List::iterator sit = windows.begin(); sit != windows.end(); sit++) {
                ScreenWindow *sw = *sit;
                Window *w = sw->get_circada_window();
                if (sw->get_circada_session() == s) {
                    sep = (last_server ? "  +-" : "| +-");
                    if (w->get_window_type() == WindowTypeServer) {
                        tv.push_back(TreeViewEntry(sw, true, sep, make_tree_nr(sit) + "status"));
                    } else {
                        tv.push_back(TreeViewEntry(sw, true, sep, make_tree_nr(sit) + w->get_name()));
                    }
                }
            }
        }
    }
}

std::string Application::make_tree_nr(ScreenWindow::List::iterator& it) {
    char buffer[16];
    std::string num;
    int index = static_cast<int>((it - windows.begin()) + 1);
    sprintf(buffer, "%d", index);
    num = buffer;
    num += ":";

    return num;
}

/* ---------------------------------------------------------------------------------- */

Session *Application::lua_find_session_throw(Session *s) {
    if (!find_session(s)) {
        throw sol::error("Session not found");
    }

    return s;
}

void Application::lua_cmd_raw(Session *s, const std::string& params) {
    lua_find_session_throw(s)->send(params);
}

void Application::lua_cmd_join(Session *s, const std::string& params) {
    lua_cmd_raw(s, "JOIN " + params);
}

void Application::lua_cmd_part(Session *s, const std::string& params) {
    lua_cmd_raw(s, "PART " + params);
}

void Application::lua_cmd_privmsg(Session *s, const std::string& dest, const std::string& msg) {
    lua_cmd_raw(s, "PRIVMSG " + dest + " :" + msg);
}

void Application::lua_cmd_notice(Session *s, const std::string& dest, const std::string& msg) {
    lua_cmd_raw(s, "NOTICE " + dest + " :" + msg);
}

void Application::lua_cmd_me(Session *s, const std::string& dest, const std::string& msg) {
    lua_cmd_raw(s, "PRIVMSG " + dest + " :\x01" + "ACTION " + msg + "\x01");
}

void Application::lua_cmd_mode(Session *s, const std::string& dest, const std::string& params) {
    lua_cmd_raw(s, "MODE " + dest + " " + params);
}

/* ---------------------------------------------------------------------------------- */

sol::table Application::make_table_from_message(const Message& msg) {
    sol::table t = lua.create_table("Message");

    t["timestamp"] = msg.timestamp;
    t["line"] = msg.line;
    t["nick_with_prefix"] = msg.nick_with_prefix;
    t["user_and_host"] = msg.user_and_host;
    t["user"] = msg.user;
    t["host"] = msg.host;
    t["command"] = msg.command;
    t["ctcp"] = msg.ctcp;
    t["op_notices"] = msg.op_notices;

    return t;
}

void Application::lua_on_connection_lost(Session *s, const std::string& reason) {
    try {
        lua["on_connection_lost"](s, reason);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_message_arrived(Session *s, Window *w, const Message& msg) {
    try {
        lua["on_message_arrived"](s, w, msg);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_my_mode_changed(Session *s, const std::string& mode) {
    try {
        lua["on_my_mode_changed"](s, mode);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_window_opened(Session *s, Window *w) {
    try {
        lua["on_window_opened"](s, w);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_window_closing(Session *s, Window *w) {
    try {
        lua["on_window_closing"](s, w);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_topic_changed(Session *s, Window *w, const std::string& topic) {
    try {
        lua["on_topic_changed"](s, w, topic);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_name_changed(Session *s, Window *w, const std::string& old_name, const std::string& new_name) {
    try {
        lua["on_name_changed"](s, w, old_name, new_name);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_channel_mode_changed(Session *s, Window *w, const std::string& mode) {
    try {
        lua["on_channel_mode_changed"](s, w, mode);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_new_nicklist(Session *s, Window *w) {
    try {
        lua["on_new_nicklist"](s, w);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_nick_added(Session *s, Window *w, const std::string& nick) {
    try {
        lua["on_nick_added"](s, w, nick);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_nick_removed(Session *s, Window *w, const std::string& nick) {
    try {
        lua["on_nick_removed"](s, w, nick);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

void Application::lua_on_nick_changed(Session *s, Window *w, const std::string& old_nick, const std::string& new_nick) {
    try {
        lua["on_nick_changed"](s, w, old_nick, new_nick);
    } catch (const sol::error& e)  {
        lua_error(e);
    }
}

/* ---------------------------------------------------------------------------------- */

void Application::lua_setup() {
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::utf8, sol::lib::table, sol::lib::math);

    // setup functions
    lua["__prn"] = [&](const std::string& s) {
        Window *w = get_application_window();
        w->set_action(Circada::WindowActionChat);
        print(get_window(w), s);
        window_action(0, w);
    };

    lua["raw"]          = [&](Session *s, const std::string& a) { lua_cmd_raw(s, a); };
    lua["join"]         = [&](Session *s, const std::string& a) { lua_cmd_join(s, a); };
    lua["part"]         = [&](Session *s, const std::string& a) { lua_cmd_part(s, a); };
    lua["msg"]          = [&](Session *s, const std::string& a, const std::string& b) { lua_cmd_privmsg(s, a, b); };
    lua["notice"]       = [&](Session *s, const std::string& a, const std::string& b) { lua_cmd_notice(s, a, b); };
    lua["me"]           = [&](Session *s, const std::string& a, const std::string& b) { lua_cmd_me(s, a, b); };
    lua["mode"]         = [&](Session *s, const std::string& a, const std::string& b) { lua_cmd_mode(s, a, b); };

    // override print(...)
    lua.script(R"(
        function print(...)
            local arg = {...}
            local s = ""
            for a, b in ipairs(arg) do
                s = s .. tostring(b)
            end
            __prn(s)
        end
    )");

    // register types
    lua.new_usertype<Message>("Message", sol::no_constructor,
        "timestamp",        sol::readonly(&Message::timestamp),
        "line",             sol::readonly(&Message::line),
        "nick_with_prefix", sol::readonly(&Message::nick_with_prefix),
        "user_and_host",    sol::readonly(&Message::user_and_host),
        "user",             sol::readonly(&Message::user),
        "host",             sol::readonly(&Message::host),
        "command",          sol::readonly(&Message::command),
        "ctcp",             sol::readonly(&Message::ctcp),
        "op_notices",       sol::readonly(&Message::op_notices)
    );

    lua.new_usertype<Session>("Session", sol::no_constructor,
        "is_that_me",       &Session::is_that_me,
        "is_channel",       &Session::is_channel,
        "get_flags",        &Session::get_flags,
        "get_nick",         &Session::get_nick,
        "get_server",       &Session::get_server,
        "am_i_away",        &Session::am_i_away,
        "get_nicklen",      &Session::get_nicklen,
        "get_lag",          &Session::get_lag
    );

    lua.new_enum("WindowType", "APPLICATION", 0, "SERVER", 1, "CHANNEL", 2, "PRIVATE", 3, "DCC", 4, "ALERTS", 5);
    lua.new_enum("WindowAction", "NONE", 0, "NOISE", 1, "CHAT", 2, "ALERT", 3);

    lua.new_usertype<Window>("Window", sol::no_constructor,
        "get_window_type",  &Window::get_window_type,
        "get_name",         &Window::get_name,
        "get_topic",        &Window::get_topic,
        "get_flags",        &Window::get_flags,
        "get_action",       &Window::get_action
    );
}

void Application::lua_print(const char *s) {
    Window *w = get_application_window();
    w->set_action(Circada::WindowActionChat);
    print(get_window(w), s);
    window_action(0, w);
}

void Application::lua_error(const sol::error& e) {
    lua_error(e.what());
}

void Application::lua_error(const char *s) {
    Window *w = get_application_window();
    w->set_action(Circada::WindowActionAlert);
    print(get_window(w), s);
    window_action(0, w);
}