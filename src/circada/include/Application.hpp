/*
 *  Application.hpp
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

#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include "ScreenWindow.hpp"
#include "Terminal.hpp"
#include "StatusWidget.hpp"
#include "EntryWidget.hpp"
#include "TextWidget.hpp"
#include "NicklistWidget.hpp"
#include "TreeViewWidget.hpp"
#include "Formatter.hpp"

#include <Circada/Circada.hpp>
#include <vector>

using namespace Circada;

class ApplicationException : public Exception {
public:
    ApplicationException(const char *msg) : Exception(msg) { }
    ApplicationException(std::string msg) : Exception(msg) { }
};

class Application : public IrcClient, public Parser {
public:
    Application(Configuration& config) throw (ApplicationException);
    virtual ~Application();

    void run() throw (ApplicationException);

private:
    Configuration& config;

    ScreenWindow::List windows;
    Mutex draw_mtx;
    WINDOW *win_main;
    int height, width;
    int nicklist_width;

    ScreenWindow *selected_window;

    TopicWidget topic_widget;
    EntryWidget entry_widget;
    EntryWidget number_widget;
    StatusWidget status_widget;
    TextWidget text_widget;
    NicklistWidget nicklist_widget;
    TreeViewWidget treeview_widget;

    Formatter fmt;
    Parser entry_parser;

    /* managing windows */
    int window_sequence; /* for sorting */

    /* which EntryWidget? */
    bool input_numbers;
    std::string number_input_sign;
    std::string windowbar_separator;

    /* widget flags */
    bool nicklist_visible;
    bool treeview_visible;
    bool highlightwindow_visible;

    /* loop */
    bool running;

    ScreenWindow *create_window(Session *s, Window *w);
    ScreenWindow *get_window(Window *w);
    ScreenWindow *get_window_nolock(Window *w);
    ScreenWindow *get_server_window(Session *s);
    ScreenWindow *get_server_window_nolock(Session *s);
    void destroy_window(Window *w);
    void destroy_window(ScreenWindow *w);
    void set_topic(Window *w, const std::string& topic);
    void set_topic(ScreenWindow *w, const std::string& topic);
    ScreenWindow *set_name(Window *w, const std::string& name);
    ScreenWindow *set_name(ScreenWindow *w, const std::string& name);
    ScreenWindow *set_channel_mode(Window *w, const std::string& mode);
    void set_lag(ScreenWindow *w, double lag_in_s);
    void changes_in_nicklist(Window *w);
    void select_next_window();
    void select_prev_window();

    /* resize and drawing functions */
    void configure();
    void draw();
    void draw_text_browser();
    void append_new_line();
    void parse_entry();
    void set_cursor();
    void update_input_infobar();

    void append_welcome_message(ScreenWindow *w);
    void print(ScreenWindow *w, const std::string& what);
    void select_window(ScreenWindow *w);
    int get_window_nbr(ScreenWindow *w);
    void send_dcc_abort(Window *w, DCCHandle dcc, const std::string& reason);

    void message_router(Session *s, Window *w, const Message& m) throw ();
    void build_window_tree();
    std::string make_tree_nr(ScreenWindow::List::iterator& it);

    /* internal command interpreter */
    typedef std::vector<std::string> Params;
    void execute(const std::string& line);
    void split(const std::string& from, std::vector<std::string>& into, int max_params = 0);
    void execute_connect(const std::string& params);
    void execute_disconnect(const std::string& params);
    void execute_dcc(const std::string& params);
    void execute_query(const std::string& params);
    void execute_clear(const std::string& params);
    void execute_close(const std::string& params);
    void execute_quit(const std::string& params);
    void execute_save(const std::string& params);
    void execute_set(const std::string& params);
    void execute_get(const std::string& params);
    void print_line(ScreenWindow *w, const std::string& timestamp, const std::string& what, Format& fmt);
    void print_line(ScreenWindow *w, const std::string& timestamp, const std::string& what);

    /* irc events */
    virtual void message(Session *s, Window *w, const Message& m) throw ();
    virtual void notice(Session *s, Window *w, const Message& m) throw ();
    virtual void noise(Session *s, Window *w, const Message& m) throw ();
    virtual void ctcp_request(Session *s, Window *w, const Message& m) throw ();
    virtual void ctcp_unhandled_request(Session *s, Window *w, const Message& m) throw ();
    virtual void change_my_mode(Session *s, const std::string& mode) throw ();
    virtual void open_window(Session *s, Window *w) throw ();
    virtual void close_window(Session *s, Window *w) throw ();
    virtual void window_action(Session *s, Window *w) throw ();
    virtual void change_topic(Session *s, Window *w, const std::string& topic) throw ();
    virtual void change_name(Session *s, Window *w, const std::string& name) throw ();
    virtual void change_channel_mode(Session *s, Window *w, const std::string& mode) throw ();
    virtual void new_nicklist(Session *s, Window *w) throw ();
    virtual void add_nick(Session *s, Window *w, const std::string& nick) throw ();
    virtual void remove_nick(Session *s, Window *w, const std::string& nick) throw ();
    virtual void change_nick(Session *s, Window *w, const std::string& old_nick, const std::string& new_nick) throw ();
    virtual void change_my_nick(Session *s, const std::string& old_nick, const std::string& new_nick) throw ();
    virtual void change_nick_mode(Session *s, Window *w, const std::string& nick, const std::string& mode) throw ();
    virtual void away(Session *s) throw ();
    virtual void unaway(Session *s) throw ();
    virtual void lag_update(Session *s, double lag_in_s) throw ();
    virtual void connection_lost(Session *s, const std::string& reason) throw ();
    virtual void dcc_offered_chat_timedout(Session *s, Window *w, const DCCChatHandle dcc, const std::string& reason) throw ();
    virtual void dcc_incoming_chat_request(Session *s, Window *w, const DCCChatHandle dcc) throw ();
    virtual void dcc_offered_xfer_timedout(Session *s, Window *w, const DCCXferHandle dcc, const std::string& reason) throw ();
    virtual void dcc_incoming_xfer_request(Session *s, Window *w, const DCCXferHandle dcc) throw ();
    virtual void dcc_unhandled_request(Session *s, Window *w, const std::string& dcc_request, const Message& m) throw ();
    virtual void dcc_unhandled_chat_request(Session *s, Window *w, const std::string& nick, const std::string& chat_request, const Message& m) throw ();
    virtual void dcc_chat_begins(Window *w, const DCCChatHandle dcc) throw ();
    virtual void dcc_chat_ended(Window *w, const DCCChatHandle dcc, const std::string& reason) throw ();
    virtual void dcc_chat_failed(Window *w, const DCCChatHandle dcc, const std::string& reason) throw ();
    virtual void dcc_message(Window *w, const DCCChatHandle dcc, const std::string& ctcp, const std::string& msg) throw ();
    virtual void dcc_xfer_begins(Window *w, const DCCXferHandle dcc) throw ();
    virtual void dcc_xfer_ended(Window *w, const DCCXferHandle dcc) throw ();
    virtual void dcc_xfer_failed(Window *w, const DCCXferHandle dcc, const std::string& reason) throw ();
    virtual void dcc_send_progress(Window *w, const DCCXferHandle dcc) throw ();
    virtual void dcc_receive_progress(Window *w, const DCCXferHandle dcc) throw ();
};

#endif // _APPLICATION_HPP_
