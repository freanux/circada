/*
 *  Formatter.hpp
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

#ifndef _FORMATTER_HPP_
#define _FORMATTER_HPP_

#include <Circada/Circada.hpp>

#include <vector>

enum FormatterColor {
    FormatterColorDarkBlack = 0,
    FormatterColorDarkRed = 1,
    FormatterColorDarkGreen = 2,
    FormatterColorDarkYellow = 3,
    FormatterColorDarkBlue = 4,
    FormatterColorDarkMagenta = 5,
    FormatterColorDarkCyan = 6,
    FormatterColorDarkWhite = 7,
    FormatterColorBrightBlack = 8,
    FormatterColorBrightRed = 9,
    FormatterColorBrightGreen = 10,
    FormatterColorBrightYellow = 11,
    FormatterColorBrightBlue = 12,
    FormatterColorBrightMagenta = 13,
    FormatterColorBrightCyan = 14,
    FormatterColorBrightWhite = 15
};

struct Format {
    Format();
    unsigned char color;
    bool bold;
    bool italic;
    bool underline;
};

class Formatter {
public:
    static const char AttributeSwitch = '\x00';
    static const char AttributeFormat = '\x00';
    static const char AttributeBreakMarker = '\x01';
    static const char AttributeRepeater = '\x02';
    static const int AttributeBold = 1;
    static const int AttributeItalic = 2;
    static const int AttributeUnderline = 4;

    struct Attribute {
        unsigned char color;
        unsigned char switches;
    };

    Formatter();
    virtual ~Formatter();

    static unsigned char get_color_code(int fg, int bg);

    Format fmt_logo;
    Format fmt_logo_text;
    Format fmt_logo_addition;

    Format fmt_print_app;
    Format fmt_print;

    Format fmt_dcc_list;
    Format fmt_dcc_info;
    Format fmt_dcc_fail;
    Format fmt_dcc_fail_bold;

    Format fmt_dcc;
    Format fmt_dcc_bold;

    Format fmt_time;

    Format fmt_connection_lost;

    Format fmt_none_dash;
    Format fmt_none_colon;

    Format fmt_notice_dash;
    Format fmt_notice_nick;
    Format fmt_notice_server;

    Format fmt_nick_normal_brackets;
    Format fmt_nick_normal;
    Format fmt_nick_normal_private;

    Format fmt_nick_highlight_brackets;
    Format fmt_nick_highlight;
    Format fmt_nick_highlight_private;

    Format fmt_nick_to_me_brackets;
    Format fmt_nick_to_me_highlight;

    Format fmt_nick_to_me_normal;
    Format fmt_nick_to_me_italic;
    Format fmt_nick_to_me_bold;
    Format fmt_nick_to_me_underline;

    Format fmt_nick_action;

    Format fmt_info_normal;
    Format fmt_info_bold;
    Format fmt_info_brackets;

    Format fmt_text_normal;
    Format fmt_text_italic;
    Format fmt_text_bold;
    Format fmt_text_underline;

    Format fmt_join_nick;
    Format fmt_leave_nick;

    Format fmt_ctcp;
    Format fmt_ctcp_bold;
    Format fmt_ctcp_unhandled;

    Format fmt_day_change_line;
    Format fmt_last_viewed;
    Format fmt_netsplit;
    Format fmt_netsplit_highlight;
    Format fmt_netsplit_over;

    Format fmt_alert_channel;

    void parse(const Circada::Message& m, std::string& line, const char *from);
    void plaintext(const std::string& timestamp, const std::string& text, std::string& line);
    void append_format(const std::string& what, const Format& fmt, std::string& into);
    void append_dcc_msg(const std::string& timestamp, const std::string& nick, bool me, const std::string& ctcp, const std::string& msg, std::string& into);

private:
    typedef std::vector<int> Colors;
    typedef void (Formatter::*CommandFunction)(const Circada::Message &m, std::string& into);
    struct Command {
        const char *command;
        CommandFunction function;
    };

    int current_color;
    Colors colors;
    static Command commands[];

    void append_cell(std::string& into);
    void append_repeater(const Format& fmt, std::string& into);
    void append_server(std::string& into);
    void append_responder(const Circada::Message &m, std::string& into, bool with_server = false);
    void append_nick(const Circada::Message &m, std::string& into);
    void append_text(const Circada::Message &m, std::string& into);
    void append_formatted_text(const std::string& what, const Format& fmt, std::string& into);

    void cmd_privmsg(const Circada::Message &m, std::string& into);
    void cmd_notice(const Circada::Message &m, std::string& into);
    void cmd_pong(const Circada::Message &m, std::string& into);
    void cmd_join(const Circada::Message &m, std::string& into);
    void cmd_part(const Circada::Message &m, std::string& into);
    void cmd_kick(const Circada::Message &m, std::string& into);
    void cmd_nick(const Circada::Message &m, std::string& into);
    void cmd_quit(const Circada::Message &m, std::string& into);
    void cmd_mode(const Circada::Message &m, std::string& into);
    void cmd_topic(const Circada::Message &m, std::string& into);
    void cmd_invite(const Circada::Message &m, std::string& into);
    void cmd_error(const Circada::Message &m, std::string& into);
    void int_mode_change(const Circada::Message &m, std::string& into);
    void int_mode_channel_add(const Circada::Message &m, std::string& into);
    void int_mode_channel_remove(const Circada::Message &m, std::string& into);
    void int_mode_channel_change(const Circada::Message &m, std::string& into);
    void int_mode_channel_set(const Circada::Message &m, std::string& into);
    void int_mode_channel_reset(const Circada::Message &m, std::string& into);
    void int_mode_nick_change(const Circada::Message &m, std::string& into);
    void int_day_change(const Circada::Message &m, std::string& into);
    void int_last_viewed(const Circada::Message &m, std::string& into);
    void int_netsplit(const Circada::Message &m, std::string& into);
    void int_netsplit_over(const Circada::Message &m, std::string& into);
    void rpl_umodeis(const Circada::Message &m, std::string& into);
    void rpl_channelmodeis(const Circada::Message &m, std::string& into);
    void rpl_website(const Circada::Message &m, std::string& into);
    void rpl_topicwhotime(const Circada::Message &m, std::string& into);
    void rpl_inviting(const Circada::Message &m, std::string& into);
    void rpl_creationtime(const Circada::Message &m, std::string& into);
    void rpl_away(const Circada::Message &m, std::string& into);
    void rpl_whoisuser(const Circada::Message &m, std::string& into);
    void rpl_whoisserver(const Circada::Message &m, std::string& into);
    void rpl_whoisidle(const Circada::Message &m, std::string& into);
    void rpl_whoisaccount(const Circada::Message &m, std::string& into);
    void rpl_whoreply(const Circada::Message &m, std::string& into);
    void rpl_banlist(const Circada::Message &m, std::string& into);
    void rpl_endofbanlist(const Circada::Message &m, std::string& into);
    void rpl_exceptlist(const Circada::Message &m, std::string& into);
    void rpl_endofexceptlist(const Circada::Message &m, std::string& into);
    void err_inviteonlychan(const Circada::Message &m, std::string& into);
    void err_linkchannel(const Circada::Message &m, std::string& into);
};

#endif // _FORMATTER_HPP_
