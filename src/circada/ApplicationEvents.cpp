/*
 *  ApplicationEvents.cpp
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

void Application::message(Session *s, Window *w, const Message& m) throw () {
    if (!is_equal(m.ctcp, "dcc")) {
        message_router(s, w, m);
    }
}

void Application::notice(Session *s, Window *w, const Message& m) throw () {
    if (!is_equal(m.ctcp, "dcc")) {
        message_router(s, w, m);
    }
}

void Application::noise(Session *s, Window *w, const Message& m) throw () {
    if (!is_equal(m.ctcp, "dcc")) {
        message_router(s, w, m);
    }
}

void Application::ctcp_request(Session *s, Window *w, const Message& m) throw () {
    if (!is_equal(m.ctcp, "dcc")) {
        message_router(s, w, m);
    }
}

void Application::ctcp_unhandled_request(Session *s, Window *w, const Message& m) throw () {
    message_router(s, w, m);
}

void Application::change_my_mode(Session *s, const std::string& mode) throw () {
    ScopeMutex lock(&draw_mtx);
    status_widget.set_nick_mode(s->get_flags());
    status_widget.draw();
    set_cursor();
}

void Application::open_window(Session *s, Window *w) throw () {
    ScreenWindow *sw = create_window(s, w);

    if (!text_widget.get_selected_window() ||
        w->get_window_type() == WindowTypeServer ||
        w->get_window_type() == WindowTypeChannel)
    {
        select_window(sw);
    } else if (treeview_visible) {
        configure();
    }

    if (w->get_window_type() == WindowTypeApplication) {
        append_welcome_message(sw);
    }
}

void Application::close_window(Session *s, Window *w) throw () {
    if (selected_window->get_circada_window() == w) {
        select_prev_window();
    }
    destroy_window(w);
    if (treeview_visible) {
        configure();
    } else {
        ScopeMutex lock(&draw_mtx);
        status_widget.draw();
    }
    ScopeMutex lock(&draw_mtx);
    set_cursor();
}

void Application::window_action(Session *s, Window *w) throw () {
    if (w->get_action() == WindowActionAlert) {
        beep();
    }

    if (selected_window && selected_window->get_circada_window() == w) {
        w->reset_action();
    } else {
        ScopeMutex lock(&draw_mtx);
        if (treeview_visible) {
            treeview_widget.draw(selected_window);
        }
        status_widget.draw();
        set_cursor();
    }
}

void Application::change_topic(Session *s, Window *w, const std::string& topic) throw () {
    set_topic(w, topic);
}

void Application::change_name(Session *s, Window *w, const std::string& name) throw () {
    set_name(w, name);
}

void Application::change_channel_mode(Session *s, Window *w, const std::string& mode) throw () {
    set_channel_mode(w, mode);
}

void Application::new_nicklist(Session *s, Window *w) throw () {
    changes_in_nicklist(w);
}

void Application::add_nick(Session *s, Window *w, const std::string& nick) throw () {
    changes_in_nicklist(w);
}

void Application::remove_nick(Session *s, Window *w, const std::string& nick) throw () {
    changes_in_nicklist(w);
}

void Application::change_nick(Session *s, Window *w, const std::string& old_nick, const std::string& new_nick) throw () {
    changes_in_nicklist(w);
}

void Application::change_my_nick(Session *s, const std::string& old_nick, const std::string& new_nick) throw () {
    ScopeMutex lock(&draw_mtx);
    status_widget.set_nick(s->get_nick());
    status_widget.draw();
    set_cursor();
}

void Application::change_nick_mode(Session *s, Window *w, const std::string& nick, const std::string& mode) throw () {
    changes_in_nicklist(w);
}

void Application::away(Session *s) throw () {
    if (selected_window->get_circada_session() == s) {
        ScopeMutex lock(&draw_mtx);
        status_widget.set_nick_away(s->am_i_away());
        status_widget.draw();
        set_cursor();
    }
}

void Application::unaway(Session *s) throw () {
    if (selected_window->get_circada_session() == s) {
        ScopeMutex lock(&draw_mtx);
        status_widget.set_nick_away(s->am_i_away());
        status_widget.draw();
        set_cursor();
    }
}

void Application::lag_update(Session *s, double lag_in_s) throw () {
    if (selected_window->get_circada_session() == s) {
        set_lag(selected_window, lag_in_s);
    }
}

void Application::connection_lost(Session *s, const std::string& reason) throw () {
    ScreenWindow *sw = get_server_window(s);
    ScopeMutex lock(&draw_mtx);

    std::string info;

    fmt.append_format("Connection to server lost: ", fmt.fmt_connection_lost, info);
    fmt.append_format(reason, fmt.fmt_connection_lost, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_offered_chat_timedout(Session *s, Window *w, const DCCChatHandle dcc, const std::string& reason) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("DCC CHAT with ", fmt.fmt_dcc_fail, info);
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
    fmt.append_format(" timed out: ", fmt.fmt_dcc_fail, info);
    fmt.append_format(reason, fmt.fmt_dcc_fail, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_incoming_chat_request(Session *s, Window *w, const DCCChatHandle dcc) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("Incoming ", fmt.fmt_dcc, info);
    fmt.append_format("DCC CHAT", fmt.fmt_dcc_bold, info);
    fmt.append_format(" request from ", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_offered_xfer_timedout(Session *s, Window *w, const DCCXferHandle dcc, const std::string& reason) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("DCC XFER", fmt.fmt_dcc_fail, info);
    if (dcc.get_direction() == DCCDirectionIncoming)  {
        fmt.append_format(" to ", fmt.fmt_dcc_fail, info);
    } else {
        fmt.append_format(" from ", fmt.fmt_dcc_fail, info);
    }
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_fail_bold, info);
    fmt.append_format(" (", fmt.fmt_dcc_fail, info);
    fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_fail_bold, info);
    fmt.append_format(")", fmt.fmt_dcc_fail, info);
    fmt.append_format(" timed out: ", fmt.fmt_dcc_fail, info);
    fmt.append_format(reason, fmt.fmt_dcc_fail, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_incoming_xfer_request(Session *s, Window *w, const DCCXferHandle dcc) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("Incoming ", fmt.fmt_dcc, info);
    fmt.append_format("DCC XFER", fmt.fmt_dcc_bold, info);
    fmt.append_format(" request from ", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
    fmt.append_format(". (", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_bold, info);
    fmt.append_format(")", fmt.fmt_dcc, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_unhandled_request(Session *s, Window *w, const std::string& dcc_request, const Message& m) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("Unhandled", fmt.fmt_dcc_fail, info);
    fmt.append_format(" DCC request (", fmt.fmt_dcc, info);
    fmt.append_format(dcc_request, fmt.fmt_dcc_bold, info);
    fmt.append_format(") from ", fmt.fmt_dcc, info);
    fmt.append_format(m.nick, fmt.fmt_dcc_bold, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_unhandled_chat_request(Session *s, Window *w, const std::string& nick, const std::string& chat_request, const Message& m) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    std::string info;

    fmt.append_format("Unhandled", fmt.fmt_dcc_fail, info);
    fmt.append_format(" DCC CHAT request (", fmt.fmt_dcc, info);
    fmt.append_format(chat_request, fmt.fmt_dcc_bold, info);
    fmt.append_format(") from ", fmt.fmt_dcc, info);
    fmt.append_format(m.nick, fmt.fmt_dcc_bold, info);

    print_line(sw, get_now(), info);
    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_chat_begins(Window *w, const DCCChatHandle dcc) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    std::string info;

    fmt.append_format("DCC CHAT with ", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
    fmt.append_format(" started.", fmt.fmt_dcc, info);

    print_line(sw, get_now(), info);
    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_chat_ended(Window *w, const DCCChatHandle dcc, const std::string& reason) throw () {
    send_dcc_abort(w, dcc, reason);
}

void Application::dcc_chat_failed(Window *w, const DCCChatHandle dcc, const std::string& reason) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);

    std::string info;

    fmt.append_format("DCC CHAT with ", fmt.fmt_dcc_fail, info);
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_fail_bold, info);
    fmt.append_format(" failed: ", fmt.fmt_dcc_fail, info);
    fmt.append_format(reason, fmt.fmt_dcc_fail, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_message(Window *w, const DCCChatHandle dcc, const std::string& ctcp, const std::string& msg) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    std::string line;
    fmt.append_dcc_msg(get_now(), dcc.get_his_nick(), false, ctcp, msg, line);
    sw->add_formatted_line(line);
    text_widget.draw_line(sw, line);
    text_widget.refresh(sw);

    if (sw != selected_window) {
        if (sw->get_circada_window()->set_action(WindowActionChat)) {
            status_widget.draw();
        }
    }

    set_cursor();
}

void Application::dcc_xfer_begins(Window *w, const DCCXferHandle dcc) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (!sw) {
        sw = get_window_nolock(get_application_window());
    }
    std::string info;

    fmt.append_format("DCC XFER", fmt.fmt_dcc, info);
    if (dcc.get_direction() == DCCDirectionIncoming)  {
        fmt.append_format(" to ", fmt.fmt_dcc_info, info);
    } else {
        fmt.append_format(" from ", fmt.fmt_dcc_info, info);
    }
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
    fmt.append_format(" started. Transferring ", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_bold, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_xfer_ended(Window *w, const DCCXferHandle dcc) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (!sw) {
        sw = get_window_nolock(get_application_window());
    }

    std::string info;

    fmt.append_format("DCC XFER", fmt.fmt_dcc, info);
    if (dcc.get_direction() == DCCDirectionIncoming)  {
        fmt.append_format(" to ", fmt.fmt_dcc_info, info);
    } else {
        fmt.append_format(" from ", fmt.fmt_dcc_info, info);
    }
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_bold, info);
    fmt.append_format(" ended. ", fmt.fmt_dcc, info);
    fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_bold, info);
    fmt.append_format(" successfully transferred.", fmt.fmt_dcc, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_xfer_failed(Window *w, const DCCXferHandle dcc, const std::string& reason) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    if (!sw) {
        sw = get_window_nolock(get_application_window());
    }

    std::string info;

    fmt.append_format("DCC XFER", fmt.fmt_dcc_fail, info);
    if (dcc.get_direction() == DCCDirectionIncoming)  {
        fmt.append_format(" to ", fmt.fmt_dcc_fail, info);
    } else {
        fmt.append_format(" from ", fmt.fmt_dcc_fail, info);
    }
    fmt.append_format(dcc.get_his_nick(), fmt.fmt_dcc_fail_bold, info);
    fmt.append_format(" failed. (", fmt.fmt_dcc_fail, info);
    fmt.append_format(dcc.get_filename(), fmt.fmt_dcc_fail_bold, info);
    fmt.append_format("): ", fmt.fmt_dcc_fail, info);
    fmt.append_format(reason, fmt.fmt_dcc_fail, info);

    print_line(sw, get_now(), info);

    text_widget.refresh(sw);
    set_cursor();
}

void Application::dcc_send_progress(Window *w, const DCCXferHandle dcc) throw () {
    if (w) {
        ScopeMutex lock(&draw_mtx);
        char buffer[128];
        ScreenWindow *sw = get_window_nolock(w);
        sprintf(buffer, "%d/%d", dcc.get_transferred_bytes(), dcc.get_filesize());
        std::string info("Sending " + dcc.get_filename() +", ");
        info += buffer;
        print_line(sw, get_now(), info, fmt.fmt_dcc_info);
        text_widget.refresh(sw);
        set_cursor();
    }
}

void Application::dcc_receive_progress(Window *w, const DCCXferHandle dcc) throw () {
    if (w) {
        ScopeMutex lock(&draw_mtx);
        char buffer[128];
        ScreenWindow *sw = get_window_nolock(w);
        sprintf(buffer, "%d/%d", dcc.get_transferred_bytes(), dcc.get_filesize());
        std::string info("Receiving " + dcc.get_filename() +", ");
        info += buffer;
        print_line(sw, get_now(), info, fmt.fmt_dcc_info);
        text_widget.refresh(sw);
        set_cursor();
    }
}

/* message router */
void Application::message_router(Session *s, Window *w, const Message& m) throw () {
    ScopeMutex lock(&draw_mtx);
    ScreenWindow *sw = get_window_nolock(w);
    const std::string& line = sw->add_line(fmt, m);
    if (selected_window == sw) {
        text_widget.draw_line(sw, line);
        text_widget.refresh(sw);
        set_cursor();
    }
}
