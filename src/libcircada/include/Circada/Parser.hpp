/*
 *  Parser.hpp
 *
 *  Created by freanux on Feb 21, 2015
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

#ifndef _CIRCADA_PARSER_HPP_
#define _CIRCADA_PARSER_HPP_

#include <Circada/Exception.hpp>
#include <Circada/Session.hpp>
#include <Circada/Window.hpp>

#include <string>
#include <vector>

namespace Circada  {

class ParserException : public Exception {
public:
    ParserException(const char *msg) : Exception(msg) { }
    ParserException(const std::string& msg) : Exception(msg) { }
};

class Parser {
private:
    Parser(const Parser& rhs);
    Parser& operator=(const Parser& rhs);

public:
    Parser();
    virtual ~Parser();

    void set_nicklist(Nick::List *nicklist);
    void set_nick_suffix(const std::string& suffix);
    const std::string& get_nick_suffix();

    std::string parse(const Session *s, const Window *w, std::string line, bool& external) throw (ParserException);

    void complete(std::string *text, int *cursor_pos);
    void reset_tab_completion();

private:
    typedef std::string (Parser::*ParserFunction)(const Session *s, const Window *w, const std::string& params);

    struct Command {
        const char *command;
        const char *irc_command;
        ParserFunction function;
        bool external;
    };

    static Command commands[];

    bool cleared;
    Nick::List *nicklist;
    std::string nick_suffix;
    std::string completion_origin_string;
    int completion_origin_cursor_pos;
    Command *last_command;
    int last_nick;
    std::string completion_search_pattern;
    int completion_search_pattern_len;
    int completion_nick_insert_pos;
    bool is_command;

    void do_command_completion(std::string& line, int& curpos);
    void do_nick_completion(std::string& line, int& curpos);

    std::string cmd_std_none(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_1(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_1_colon_opt(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_1_colon_opt_colon(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_2(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_2_opt(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_2_colon(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_std_2_colon_opt(const Session *s, const Window *w, const std::string& p) throw (ParserException);

    std::string cmd_part(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_partall(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_query(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_me(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_ctcp(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_ban(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_unban(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_voice(const Session *s, const Window *w, const std::string& p) throw (ParserException);
    std::string cmd_devoice(const Session *s, const Window *w, const std::string& p) throw (ParserException);
};

} /* namespace Circada */

#endif /* _CIRCADA_PARSER_HPP_ */
