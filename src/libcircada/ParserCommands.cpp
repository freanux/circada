/*
 *  ParserCommands.cpp
 *
 *  Created by freanux on Feb 24, 2015
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

#include "Circada/Parser.hpp"
#include "Circada/Utils.hpp"

namespace Circada {

    class ParserWrongArgumentsException : public ParserException {
    public:
        ParserWrongArgumentsException() : ParserException("Wrong number of arguments.") { }
    };

    class ParserWrongWindowException : public ParserException {
    public:
        ParserWrongWindowException() : ParserException("Wrong window for this command.") { }
    };

    /**************************************************************************
     * AVAILABLE COMMANDS
     **************************************************************************/
    Parser::Command Parser::commands[] = {
        { "accept ", 0, &Parser::cmd_std_1, false },
        { "admin ", 0, 0, false },
        { "authserv ", 0, &Parser::cmd_std_1, false },
        { "as ", "AUTHSERV", &Parser::cmd_std_1, false },
        { "away ", 0, &Parser::cmd_std_1_colon_opt, false },
        { "ban ", "MODE", &Parser::cmd_ban, false },
        { "botserv ", 0, &Parser::cmd_std_1, false },
        { "bs ", "BOTSERV", &Parser::cmd_std_1, false },
        { "chanserv ", 0, &Parser::cmd_std_1, false },
        { "clear", 0, 0, true },
        { "close", 0, 0, true },
        { "connect ", 0, 0, true },
        { "cs ", "CHANSERV", &Parser::cmd_std_1, false },
        { "ctcp ", "PRIVMSG", &Parser::cmd_ctcp, false },
        { "dcc", 0, 0, true },
        { "deop ", 0, 0, false },
        { "devoice ", "MODE", &Parser::cmd_devoice, false },
        { "die ", 0, 0, false },
        { "disconnect ", 0, 0, true },
        { "error ", 0, 0, false },
        { "get ", 0, 0, true },
        { "helpserv ", 0, &Parser::cmd_std_1, false },
        { "hs ", "HELPSERV", &Parser::cmd_std_1, false },
        { "info ", 0, 0, false },
        { "invite ", 0, &Parser::cmd_std_2, false },
        { "ison ", 0, &Parser::cmd_std_1, false },
        { "join ", 0, &Parser::cmd_std_1, false },
        { "kick ", 0, &Parser::cmd_std_2_colon_opt, false },
        { "kill ", 0, 0, false },
        { "knock ", 0, &Parser::cmd_std_1, false },
        { "leave ", "PART", &Parser::cmd_part, false },
        { "leaveall", "JOIN", &Parser::cmd_partall, false },
        { "links ", 0, 0, false },
        { "list ", 0, 0, false },
        { "lusers ", 0, 0, false },
        { "me ", "PRIVMSG", &Parser::cmd_me, false },
        { "memoserv ", 0, &Parser::cmd_std_1, false },
        { "mode ", 0, &Parser::cmd_std_2_opt, false },
        { "motd ", 0, 0, false },
        { "ms ", "MEMOSERV", &Parser::cmd_std_1, false },
        { "msg ", "PRIVMSG", &Parser::cmd_std_2_colon, false },
        { "names ", 0, 0, false },
        { "netsplits", 0,0, true},
        { "notice ", "NOTICE", &Parser::cmd_std_2_colon, false },
        { "nick ", 0, &Parser::cmd_std_1, false },
        { "nickserv ", 0, &Parser::cmd_std_1, false },
        { "ns ", "NICKSERV", &Parser::cmd_std_1, false },
        { "op ", 0, 0, false },
        { "oper ", 0, &Parser::cmd_std_2, false },
        { "operserv ", 0, &Parser::cmd_std_1, false },
        { "part ", 0, &Parser::cmd_part, false },
        { "partall", "JOIN", &Parser::cmd_partall, false },
        { "pass ", 0, &Parser::cmd_std_1, false },
        { "ping ", 0, 0, false },
        { "query ", 0, 0, true },
        { "quit ", 0, &Parser::cmd_std_1_colon_opt_colon, true },
        { "quote ", "", &Parser::cmd_std_1, false },
        { "raw ", "", &Parser::cmd_std_1, false },
        { "reconnect ", 0, 0, true },
        { "rehash ", 0, 0, false },
        { "rejoin ", 0, 0, true },
        { "restart ", 0, 0, false },
        { "rootserv ", 0, &Parser::cmd_std_1, false },
        { "save ", 0, 0, true },
        { "service ", 0, 0, false },
        { "servlist ", 0, 0, false },
        { "set ", 0, 0, true },
        { "sort", 0, 0, true },
        { "spamserv ", 0, &Parser::cmd_std_1, false },
        { "squery ", 0, 0, false },
        { "squit ", 0, 0, false },
        { "ss ", "SPAMSERV", &Parser::cmd_std_1, false },
        { "stats ", 0, 0, false },
        { "statserv ", 0, &Parser::cmd_std_1, false },
        { "summon ", 0, 0, false },
        { "time ", 0, 0, false },
        { "topic ", 0, &Parser::cmd_std_2_colon, false },
        { "trace ", 0, 0, false },
        { "unban ", "MODE", &Parser::cmd_unban, false },
        { "userhost ", 0, 0, false },
        { "users ", 0, 0, false },
        { "version ", 0, 0, false },
        { "voice ", "MODE", &Parser::cmd_voice, false },
        { "wallops ", 0, 0, false },
        { "who ", 0, &Parser::cmd_std_1, false },
        { "whois ", 0, &Parser::cmd_std_1, false },
        { "whowas ", 0, 0, false },

        /* eol */
        { 0, 0, 0, false }
    };

    /**************************************************************************
     * COMMAND IMPLEMENTATION
     **************************************************************************/
    std::string Parser::cmd_std_none(const Session *s, const Window *w, const std::string& p) {
        if (p.length()) {
            throw ParserWrongArgumentsException();
        }

        return "";
    }

    std::string Parser::cmd_std_1(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(1, p, v)) {
            case 1:
                output = v[0];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_1_colon_opt(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(1, p, v)) {
            case 0:
                break;

            case 1:
                output = ":" + v[0];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_1_colon_opt_colon(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(1, p, v)) {
            case 0:
                output = ":";
                break;

            case 1:
                output = ":" + v[0];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_2(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(2, p, v)) {
            case 2:
                output = v[0] + " " + v[1];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_2_opt(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(2, p, v)) {
            case 1:
                output = v[0];
                break;

            case 2:
                output = v[0] + " " + v[1];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_2_colon(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(2, p, v)) {
            case 2:
                output = v[0] + " :" + v[1];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_std_2_colon_opt(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(2, p, v)) {
            case 1:
                output = v[0];
                break;

            case 2:
                output = v[0] + " :" + v[1];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_part(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(2, p, v)) {
            case 0:
                switch (w->get_window_type()) {
                    case WindowTypeChannel:
                        output += w->get_name();
                        break;

                    case WindowTypePrivate:
                        throw ParserException("Use /query to close this window.");

                    default:
                        throw ParserWrongWindowException();
                        break;

                }
                break;

            case 1:
                output = v[0];
                break;

            case 2:
                output = v[0]  + " :" + v[1];
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_partall(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;

        switch (tokenize(1, p, v)) {
            case 0:
                output += "0";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_me(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(1, p, v)) {
            case 1:
                if (w->get_window_type() == WindowTypeServer) {
                    throw ParserWrongWindowException();
                }
                output = w->get_name() + " :\x01";
                output.append(CTCP_ACTION);
                output += " " + v[0] + "\x01";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_ctcp(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(2, p, v)) {
            case 2:
                to_upper(v[1]);
                output = v[0] + " :\x01" + v[1] + "\x01";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_ban(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(1, p, v)) {
            case 1:
                if (w->get_window_type() != WindowTypeChannel) {
                    throw ParserWrongWindowException();
                }
                output = w->get_name() + " " + v[0] + " +b";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_unban(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(1, p, v)) {
            case 1:
                if (w->get_window_type() != WindowTypeChannel) {
                    throw ParserWrongWindowException();
                }
                output = w->get_name() + " " + v[0] + " -b";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_voice(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(1, p, v)) {
            case 1:
                if (w->get_window_type() != WindowTypeChannel) {
                    throw ParserWrongWindowException();
                }
                output = w->get_name() + " " + v[0] + " +v";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

    std::string Parser::cmd_devoice(const Session *s, const Window *w, const std::string& p) {
        TokenizedParams v;
        std::string output;
        switch (tokenize(1, p, v)) {
            case 1:
                if (w->get_window_type() != WindowTypeChannel) {
                    throw ParserWrongWindowException();
                }
                output = w->get_name() + " " + v[0] + " -v";
                break;

            default:
                throw ParserWrongArgumentsException();
        }

        return output;
    }

} /* namespace Circada */
