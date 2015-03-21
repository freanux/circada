/*
 *  Formatter.cpp
 *
 *  Created by freanux on Mar 9, 2015
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

#include "Formatter.hpp"

#include <cstdlib>

Format::Format()
    : color(Formatter::get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack)),
      bold(false), italic(false), underline(false) { }

Formatter::Formatter() : current_color(1) {
    fmt_logo.color = get_color_code(FormatterColorBrightYellow, FormatterColorDarkBlack);
    fmt_logo.bold = true;
    fmt_logo_text.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);
    fmt_logo_addition.color = get_color_code(FormatterColorBrightCyan, FormatterColorDarkBlack);

    fmt_dcc_list.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);

    fmt_dcc_info.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);

    fmt_dcc_fail.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);

    fmt_dcc_fail_bold.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);
    fmt_dcc_fail_bold.bold = true;

    fmt_dcc.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);
    fmt_dcc_bold.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);
    fmt_dcc_bold.bold = true;

    fmt_print_app.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);
    fmt_print_app.bold = true;
    fmt_print.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);

    fmt_time.color = get_color_code(FormatterColorDarkWhite, FormatterColorDarkBlack);

    fmt_connection_lost.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);

    fmt_none_dash.color = get_color_code(FormatterColorBrightBlue, FormatterColorDarkBlack);
    fmt_none_colon.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);

    fmt_text_normal.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);

    fmt_notice_dash.color = get_color_code(FormatterColorBrightBlue, FormatterColorDarkBlack);
    fmt_notice_nick.color = get_color_code(FormatterColorBrightMagenta, FormatterColorDarkBlack);
    fmt_notice_server.color = get_color_code(FormatterColorDarkMagenta, FormatterColorDarkBlack);


    fmt_nick_normal_brackets.color = get_color_code(FormatterColorBrightBlack, FormatterColorDarkBlack);
    fmt_nick_normal.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);
    fmt_nick_normal_private.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);

    fmt_nick_highlight_brackets.color = get_color_code(FormatterColorBrightBlack, FormatterColorDarkBlack);
    fmt_nick_highlight.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);
    fmt_nick_highlight.bold = true;

    fmt_nick_highlight_private.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);
    fmt_nick_highlight_private.bold = true;

    fmt_nick_to_me_brackets.color = get_color_code(FormatterColorBrightBlack, FormatterColorDarkBlack);

    fmt_nick_action.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);
    fmt_nick_action.bold = true;

    fmt_join_nick.color = get_color_code(FormatterColorBrightCyan, FormatterColorDarkBlack);
    fmt_leave_nick.color = get_color_code(FormatterColorDarkCyan, FormatterColorDarkBlack);

    // text to me
    fmt_nick_to_me_highlight.color = get_color_code(FormatterColorBrightYellow, FormatterColorDarkBlack);
    fmt_nick_to_me_highlight.bold = true;

    fmt_nick_to_me_normal.color = get_color_code(FormatterColorBrightWhite, FormatterColorDarkBlack);

    fmt_nick_to_me_italic.color = fmt_nick_to_me_highlight.color;
    fmt_nick_to_me_italic.italic = true;

    fmt_nick_to_me_bold.color = fmt_nick_to_me_highlight.color;
    fmt_nick_to_me_bold.bold = true;

    fmt_nick_to_me_underline.color = fmt_nick_to_me_highlight.color;
    fmt_nick_to_me_underline.underline = true;

    // general text
    fmt_info_bold.bold = true;
    fmt_info_brackets.color = get_color_code(FormatterColorBrightBlack, FormatterColorDarkBlack);

    fmt_text_italic.italic = true;
    fmt_text_bold.bold = true;
    fmt_text_underline.underline = true;

    fmt_ctcp.color = get_color_code(FormatterColorBrightGreen, FormatterColorDarkBlack);

    fmt_ctcp_bold.color = fmt_ctcp.color;
    fmt_ctcp_bold.bold = true;

    fmt_ctcp_unhandled.color = fmt_ctcp.color;
    fmt_ctcp_unhandled.underline = true;

    fmt_day_change_line.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);

    fmt_netsplit.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);

    fmt_netsplit_highlight.color = get_color_code(FormatterColorBrightRed, FormatterColorDarkBlack);
    fmt_netsplit_highlight.bold = true;

    fmt_netsplit_over.color = get_color_code(FormatterColorBrightMagenta, FormatterColorDarkBlack);
}

Formatter::~Formatter() { }

unsigned char Formatter::get_color_code(int fg, int bg) {
	return (bg << 4) + fg;
}

void Formatter::parse(const Circada::Message& m, std::string& line) {
    line.clear();

    // add time
    append_format(m.timestamp + " ", fmt_time, line);

    // add text
    bool found = false;
    Command *cmd = commands;
    while (cmd->command) {
        if (Circada::is_equal(m.command, cmd->command)) {
            found = true;
            (this->*cmd->function)(m, line);
            break;
        }
        cmd++;
    }
    if (!found) {
        append_text(m, line);
    }
}

void Formatter::plaintext(const std::string& timestamp, const std::string& text, std::string& line) {
    line.clear();

    /* add time */
    append_format(timestamp + " ", fmt_time, line);
    append_server(line);
    append_format(text, fmt_text_normal, line);
}

void Formatter::append_dcc_msg(const std::string& timestamp, const std::string& nick, bool me, const std::string& ctcp, const std::string& msg, std::string& into) {
    into.clear();
    append_format(timestamp + " ", fmt_time, into);
    if (Circada::is_equal(ctcp, CTCP_ACTION)) {
        append_format(" ", fmt_info_normal, into);
        append_format("* " + nick, fmt_nick_action, into);
    } else {
        if (me) {
            append_format("< ", fmt_nick_highlight_brackets, into);
            append_format(nick, fmt_nick_highlight, into);
            append_format(">", fmt_nick_highlight_brackets, into);
        } else {
            append_format("< ", fmt_nick_normal_brackets, into);
            append_format(nick, fmt_nick_normal, into);
            append_format(">", fmt_nick_normal_brackets, into);
        }
    }
    append_format(" ", fmt_info_normal, into);
    append_cell(into);
    append_formatted_text(msg, fmt_text_normal, into);
}

void Formatter::append_format(const std::string& what, const Format& fmt, std::string& into) {
    into += AttributeSwitch;
    into += AttributeFormat;
    into += fmt.color;
    unsigned char sw = 0;
    if (fmt.bold) sw |= AttributeBold;
    if (fmt.italic) sw |= AttributeItalic;
    if (fmt.underline) sw |= AttributeUnderline;
    into += sw;
    into.append(what);
}

void Formatter::append_cell(std::string& into) {
    into += AttributeSwitch;
    into += AttributeBreakMarker;
}

void Formatter::append_repeater(const Format& fmt, std::string& into) {
    append_format(" ", fmt, into);
    into += AttributeSwitch;
    into += AttributeRepeater;
}

void Formatter::append_server(std::string& into) {
    append_format("-", fmt_none_dash, into);
    append_format("-", fmt_none_colon, into);
    append_format("-", fmt_none_dash, into);
    append_format(" ", fmt_info_normal, into);
    append_cell(into);
}

void Formatter::append_responder(const Circada::Message &m, std::string& into, bool with_server) {
    append_format("-", fmt_notice_dash, into);
    append_format(m.nick, fmt_notice_nick, into);
    if (with_server && m.user_and_host.length()) {
        append_format(" ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.user_and_host, fmt_notice_server, into);
        append_format("]", fmt_info_brackets, into);
    }
    append_format("-", fmt_notice_dash, into);
    append_format(" ", fmt_info_normal, into);
    append_cell(into);
}

void Formatter::append_nick(const Circada::Message &m, std::string& into) {
    if (Circada::is_equal(m.ctcp, CTCP_ACTION)) {
        append_format(" ", fmt_info_normal, into);
        append_format("*" + m.nick_with_prefix, fmt_nick_action, into);
    } else {
        if (m.to_me) {
            append_format("<", fmt_nick_to_me_brackets, into);
            append_format(m.nick_with_prefix, fmt_nick_to_me_highlight, into);
            append_format(">", fmt_nick_to_me_brackets, into);
        } else if (m.its_me) {
                append_format("<", fmt_nick_highlight_brackets, into);
                append_format(m.nick_with_prefix, fmt_nick_highlight, into);
                append_format(">", fmt_nick_highlight_brackets, into);
        } else {
            append_format("<", fmt_nick_normal_brackets, into);
            append_format(m.nick_with_prefix, fmt_nick_normal, into);
            append_format(">", fmt_nick_normal_brackets, into);
        }
    }
    append_format(" ", fmt_info_normal, into);
    append_cell(into);
}

void Formatter::append_text(const Circada::Message &m, std::string& into) {
    append_server(into);
    if (m.pc) {
        std::string output;
        int start_index = 1;
        if (atoi(m.command.c_str())) {
            //output += m.command;
            //output += " ";
            start_index = 1;
        }
        for (size_t i = start_index; i < m.pc; i++) {
            output += m.params[i] + (i < m.pc - 1 ? " " : "");
        }
        append_format(output, fmt_text_normal, into);
    }
}

void Formatter::append_formatted_text(const std::string& what, const Format& fmt, std::string& into) {
    char temp_char;
    size_t start_pos = 0;
    size_t sz = what.size();
    Format current_fmt = fmt;

    while (true) {
        std::string output;
        bool break_out = false;
        for (size_t i = start_pos; i < sz; i++) {
            temp_char = what[i];
            if (temp_char < 32) {
                switch (temp_char) {
                    case 2: /* bold */
                        append_format(output, current_fmt, into);
                        current_fmt.bold = !current_fmt.bold;
                        start_pos = i + 1;
                        break_out = true;
                        break;

                    case 22: /* italic */
                        append_format(output, current_fmt, into);
                        current_fmt.italic = !current_fmt.italic;
                        start_pos = i + 1;
                        break_out = true;
                        break;

                    case 31: /* underline */
                        append_format(output, current_fmt, into);
                        current_fmt.underline = !current_fmt.underline;
                        start_pos = i + 1;
                        break_out = true;
                        break;

                    default: // do nothing
                        output += temp_char;
                        break;
                }
            } else {
                output += temp_char;
            }
            if (break_out) break;
        }
        if (!break_out) {
            append_format(output, current_fmt, into);
            break;
        }
    }
}
