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

Application::Application(Configuration& config) throw (ApplicationException)
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

    /* create application window */
    create_application_window(get_project_name(), get_project_name());
}

Application::~Application() {
    endwin();
    for (ScreenWindow::List::iterator it = windows.begin(); it != windows.end(); it++) {
        delete *it;
    }
}

void Application::run() throw (ApplicationException) {
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
    ScreenWindow *sw = text_widget.get_selected_window();
    Session *s = sw->get_circada_session();
    Window *w = sw->get_circada_window();
    std::string line = entry_widget.get_content();

    entry_widget.reset();
    try {
        bool external;
        std::string ctcp;
        std::string msg;
        std::string output = entry_parser.parse(s, w, line, external);
        if (external) {
            execute(output);
        } else if (s) {
            s->send(output);
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
            sprintf(buf, "%lu", i + 1);
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

            const std::string& nick = config.get_value(p[0], "nick");
            if (nick.length()) {
                opts.nick = nick;
            } else {
                const std::string& nick = config.get_value("", "nick");
                if (nick.length()) {
                    opts.nick = nick;
                }
            }

            const std::string& alternative_nick = config.get_value(p[0], "alternative_nick");
            if (alternative_nick.length()) {
                opts.alternative_nick = alternative_nick;
            } else {
                const std::string& alternative_nick = config.get_value("", "alternative_nick");
                if (alternative_nick.length()) {
                    opts.alternative_nick = alternative_nick;
                }
            }

            const std::string& user = config.get_value(p[0], "user");
            if (user.length()) {
                opts.user = user;
            } else {
                const std::string& user = config.get_value("", "user");
                if (user.length()) {
                    opts.user = user;
                }
            }

            const std::string& real_name = config.get_value(p[0], "real_name");
            if (real_name.length()) {
                opts.real_name = real_name;
            } else {
                const std::string& real_name = config.get_value("", "real_name");
                if (real_name.length()) {
                    opts.real_name = real_name;
                }
            }

            const std::string& ca_file = config.get_value(p[0], "ca_file");
            if (ca_file.length()) {
                opts.ca_file = ca_file;
            } else {
                const std::string& ca_file = config.get_value("", "ca_file");
                if (ca_file.length()) {
                    opts.ca_file = ca_file;
                }
            }

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
        print_line(sw, get_now(), "Insufficient parameters.", fmt.fmt_info_normal);
    }
    text_widget.refresh(sw);
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
