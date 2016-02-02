/*
 *  FormatterFunctions.cpp
 *
 *  Created by freanux on Mar 10, 2015
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

Formatter::Command Formatter::commands[] = {
    { CMD_PRIVMSG, &Formatter::cmd_privmsg },
    { CMD_NOTICE, &Formatter::cmd_notice },
    { CMD_PONG, &Formatter::cmd_pong },
    { CMD_JOIN, &Formatter::cmd_join },
    { CMD_PART, &Formatter::cmd_part },
    { CMD_KICK, &Formatter::cmd_kick },
    { CMD_NICK, &Formatter::cmd_nick },
    { CMD_QUIT, &Formatter::cmd_quit },
    { CMD_MODE, &Formatter::cmd_mode },
    { CMD_TOPIC, &Formatter::cmd_topic },
    { CMD_INVITE, &Formatter::cmd_invite },
    { CMD_ERROR, &Formatter::cmd_error },
    { INT_MODE_CHAN, &Formatter::int_mode_change },
    { INT_MODE_CHAN_ADD, &Formatter::int_mode_channel_add },
    { INT_MODE_CHAN_REMOVE, &Formatter::int_mode_channel_remove },
    { INT_MODE_CHAN_CHANGE, &Formatter::int_mode_channel_change },
    { INT_MODE_CHAN_SET, &Formatter::int_mode_channel_set },
    { INT_MODE_CHAN_RESET, &Formatter::int_mode_channel_reset },
    { INT_MODE_NICK_CHANGE, &Formatter::int_mode_nick_change },
    { INT_DAY_CHANGE, &Formatter::int_day_change },
    { INT_LAST_VIEWED, &Formatter::int_last_viewed },
    { INT_NETSPLIT, &Formatter::int_netsplit },
    { INT_NETSPLIT_OVER, &Formatter::int_netsplit_over },
    { RPL_UMODEIS, &Formatter::rpl_umodeis },
    { RPL_CHANNELMODEIS, &Formatter::rpl_channelmodeis },
    { RPL_WEBSITE, &Formatter::rpl_website },
    { RPL_TOPICWHOTIME, &Formatter::rpl_topicwhotime },
    { RPL_INVITING, &Formatter::rpl_inviting },
    { RPL_CREATIONTIME, &Formatter::rpl_creationtime },
    { RPL_AWAY, &Formatter::rpl_away },
    { RPL_WHOISUSER, &Formatter::rpl_whoisuser },
    { RPL_WHOISSERVER, &Formatter::rpl_whoisserver },
    { RPL_WHOISIDLE, &Formatter::rpl_whoisidle },
    { RPL_WHOISACCOUNT, &Formatter::rpl_whoisaccount },
    { RPL_WHOREPLY, &Formatter::rpl_whoreply },
    { RPL_BANLIST, &Formatter::rpl_banlist },
    { RPL_ENDOFBANLIST, &Formatter::rpl_endofbanlist },
    { RPL_EXCEPTLIST, &Formatter::rpl_exceptlist },
    { RPL_ENDOFEXCEPTLIST, &Formatter::rpl_endofexceptlist },
    { ERR_INVITEONLYCHAN, &Formatter::err_inviteonlychan },
    { ERR_LINKCHANNEL, &Formatter::err_linkchannel },
    { 0, 0 }
};


void Formatter::cmd_privmsg(const Circada::Message &m, std::string& into) {
    if (m.ctcp.length() && !Circada::is_equal(m.ctcp, CTCP_ACTION)) {
        if (m.its_me && !m.host.length()) {
            append_server(into);
            append_format("You requested CTCP ", fmt_ctcp, into);
            append_format(m.ctcp, fmt_ctcp_bold, into);
            append_format(" from ", fmt_ctcp, into);
            append_format(m.params[0], fmt_text_bold, into);
        } else {
            append_responder(m, into, true);
            if (m.unhandled_ctcp_dcc) {
                append_format("requested ", fmt_ctcp, into);
                append_format("unhandled", fmt_ctcp_unhandled, into);
                append_format(" CTCP ", fmt_ctcp, into);
            } else {
                append_format("requested CTCP ", fmt_ctcp, into);
            }
            append_format(m.ctcp, fmt_ctcp_bold, into);
            append_format(" from ", fmt_ctcp, into);
            append_format(m.params[0], fmt_text_bold, into);
        }
    } else {
        append_nick(m, into);
        if (m.to_me) {
            append_formatted_text(m.params[1], fmt_nick_to_me_normal, into);
        } else {
            append_formatted_text(m.params[1], fmt_text_normal, into);
        }
    }
}

void Formatter::cmd_notice(const Circada::Message &m, std::string& into) {
    append_responder(m, into);
    append_formatted_text(m.params[1], fmt_text_normal, into);
}

void Formatter::cmd_pong(const Circada::Message &m, std::string& into) {
    append_responder(m, into);
    append_format("PONG: " + m.params[1], fmt_info_normal, into);
}

void Formatter::cmd_join(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick + " ", fmt_join_nick, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.user_and_host, fmt_join_nick, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" has joined ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::cmd_part(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick + " ", fmt_leave_nick, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.user_and_host, fmt_leave_nick, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" has left ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
    if (m.pc > 1 && m.params[1].length()) {
        append_format(" ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.params[1], fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
    }
}

void Formatter::cmd_kick(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_leave_nick, into);
    append_format(" was kicked from ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
    append_format(" by ", fmt_info_normal, into);
    append_format(m.nick, fmt_info_bold, into);
    if (m.pc > 2) {
        append_format(" ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.params[2], fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
    }
}

void Formatter::cmd_nick(const Circada::Message &m, std::string& into) {
    append_server(into);
    if (m.its_me) {
        append_format("You're now known as ", fmt_info_normal, into);
    } else {
        append_format(m.nick, fmt_leave_nick, into);
        append_format(" is now known as ", fmt_info_normal, into);
    }
    append_format(m.params[0], fmt_join_nick, into);
}

void Formatter::cmd_quit(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick + " ", fmt_leave_nick, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.user_and_host, fmt_leave_nick, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" has quit", fmt_info_normal, into);
    if (m.pc > 0 && m.params[0].length()) {
        append_format(" ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.params[0], fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
    }
}

void Formatter::cmd_mode(const Circada::Message &m, std::string& into) {
    append_server(into);
    if (m.pc == 2) {
        append_format("Mode change ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.params[1], fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
        append_format((m.session->is_channel(m.params[0]) ? " for channel " : " for user "), fmt_info_bold, into);
    } else {
        append_format("Mode ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(m.params[1], fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
        append_format(" for user ", fmt_info_bold, into);
    }
    append_format(m.params[0], fmt_info_bold, into);
    if (!Circada::is_equal(m.nick, m.params[0])) {
        append_format(" by ", fmt_info_normal, into);
        append_format(m.nick, fmt_info_bold, into);
    }
}

void Formatter::cmd_topic(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" changed the topic of ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_normal, into);
    append_format(" to: ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_normal, into);
}

void Formatter::cmd_invite(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" invites you to join the channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_normal, into);
}

void Formatter::cmd_error(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("ERROR: ", fmt_info_bold, into);
    append_format(m.params[0], fmt_info_normal, into);
}

void Formatter::int_mode_change(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Mode change ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" for channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
    append_format(" by ", fmt_info_normal, into);
    append_format(m.nick, fmt_info_bold, into);
}

void Formatter::int_mode_channel_add(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" adds user ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" to list ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" in channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::int_mode_channel_remove(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" removes user ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" from list ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" in channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::int_mode_channel_change(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format((m.pc > 2 ? " sets mode " : " resets mode "), fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    if (m.pc > 2) {
        append_format(m.params[2], fmt_info_bold, into);
    }
    append_format(" in channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::int_mode_channel_set(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" sets mode ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" to ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" in channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::int_mode_channel_reset(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.nick, fmt_info_bold, into);
    append_format(" resets mode ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" in channel ", fmt_info_normal, into);
    append_format(m.params[0], fmt_info_bold, into);
}

void Formatter::int_mode_nick_change(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Mode change ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" for user ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" by ", fmt_info_normal, into);
    append_format(m.nick, fmt_info_bold, into);
}

void Formatter::int_day_change(const Circada::Message &m, std::string& into) {
    append_format("------", fmt_day_change_line, into);
    append_format(" " + m.params[0] + " ", fmt_info_bold, into);
    append_repeater(fmt_day_change_line, into);
}

void Formatter::int_last_viewed(const Circada::Message &m, std::string& into) {
    append_repeater(fmt_last_viewed, into);
}

void Formatter::int_netsplit(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Netsplit ", fmt_netsplit, into);
    append_format(m.params[0], fmt_netsplit_highlight, into);
    append_format(" <-|-> ", fmt_netsplit, into);
    append_format(m.params[1], fmt_netsplit_highlight, into);
}

void Formatter::int_netsplit_over(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Netsplit", fmt_netsplit_over, into);
    append_format(" over. Following users join...:", fmt_info_normal, into);
}

void Formatter::rpl_umodeis(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Your user mode is ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[1], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
}

void Formatter::rpl_channelmodeis(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Mode for channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" is ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[2], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
}

void Formatter::rpl_website(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Homepage for channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(": ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_normal, into);
}

void Formatter::rpl_topicwhotime(const Circada::Message &m, std::string& into) {
    append_server(into);

    std::string s, n;
    size_t p;
    p = m.params[2].find('!');
    if (p != std::string::npos) {
        n = m.params[2].substr(0, p);
        s = m.params[2].substr(p + 1);
    } else {
        n = m.params[2];
        s = "";
    }

    append_format("Topic set by ", fmt_info_normal, into);
    append_format(n, fmt_info_bold, into);
    if (s.length()) {
        append_format(" ", fmt_info_normal, into);
        append_format("[", fmt_info_brackets, into);
        append_format(s, fmt_info_normal, into);
        append_format("]", fmt_info_brackets, into);
    }
    append_format(" ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(Circada::unix_to_date(m.params[3]), fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
}

void Formatter::rpl_inviting(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Inviting ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" to ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
}

void Formatter::rpl_creationtime(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" created ", fmt_info_normal, into);
    append_format(Circada::unix_to_date(m.params[2]), fmt_info_bold, into);
}

void Formatter::rpl_away(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" is away: " + m.params[2], fmt_info_normal, into);
}

void Formatter::rpl_whoisuser(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" ", fmt_info_normal, into);
    append_format("[", fmt_info_brackets, into);
    append_format(m.params[2] + "@" + m.params[3], fmt_info_normal, into);
    append_format("]", fmt_info_brackets, into);
    append_format(" with real name ", fmt_info_normal, into);
    append_format(m.params[5], fmt_info_bold, into);
}

void Formatter::rpl_whoisserver(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" is hosted on ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" ", fmt_info_normal, into);
    append_format("(" + m.params[3] + ")", fmt_info_normal, into);
}

void Formatter::rpl_whoisidle(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" is ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
    append_format(" seconds idle, signed on " + Circada::unix_to_date(m.params[3]), fmt_info_normal, into);
}

void Formatter::rpl_whoisaccount(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" is logged in as ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
}

void Formatter::rpl_whoreply(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(" " + m.params[1] + " ", fmt_notice_nick, into);
    append_format(m.params[5], fmt_info_bold, into);
    append_format(" " + m.params[2] + "@" + m.params[3] + " " + m.params[4], fmt_info_normal, into);

    std::string p;
    for (size_t i = 6; i < m.pc; i++) {
        p += " " + m.params[i];
    }
    if (p.length()) {
        append_format(p, fmt_info_normal, into);
    }
}

void Formatter::rpl_banlist(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(" ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
}

void Formatter::rpl_endofbanlist(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("End of ban list of channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
}

void Formatter::rpl_exceptlist(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format(" ", fmt_info_normal, into);
    append_format(m.params[2], fmt_info_bold, into);
}

void Formatter::rpl_endofexceptlist(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("End of exception list of channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
}

void Formatter::err_inviteonlychan(const Circada::Message &m, std::string& into) {
    append_server(into);
    append_format("Can't join to channel ", fmt_info_normal, into);
    append_format(m.params[1], fmt_info_bold, into);
    append_format(" (You must be invited)", fmt_info_normal, into);
}

void Formatter::err_linkchannel(const Circada::Message &m, std::string& into) {
    append_server(into);
    if (m.pc > 3) {
        append_format("Channel ", fmt_info_normal, into);
        append_format(m.params[1], fmt_info_bold, into);
        append_format(" linked to ", fmt_info_normal, into);
        append_format(m.params[2], fmt_info_bold, into);
        append_format(" (" + m.params[3] + ")", fmt_info_normal, into);
    } else {
        append_text(m, into);
    }
}

