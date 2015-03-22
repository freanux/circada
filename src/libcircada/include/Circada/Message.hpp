/*
 *  Message.hpp
 *
 *  Created by freanux on Feb 15, 2015
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

#ifndef _CIRCADA_MESSAGE_HPP_
#define _CIRCADA_MESSAGE_HPP_

#include <Circada/Recoder.hpp>

#include <string>
#include <vector>

namespace Circada {

class Session;
class Window;

class Message {
public:
    typedef std::vector<std::string> Params;

    Message() { }
    virtual ~Message() { }

    void parse(Session *s, const std::string& message, const Recoder *recoder = 0);
    void make_midnight();

    Session *session;
    std::string timestamp;
    std::string line;
    std::string nick;
    std::string nick_with_prefix;
    std::string user_and_host;
    std::string user;
    std::string host;
    std::string command;
    std::string ctcp;
    std::string op_notices;
    Params params;
    size_t pc;
    bool injected;
    bool its_me;
    bool to_me;
    bool unhandled_ctcp_dcc;
};

} /* namespace Circada */

#endif /* _CIRCADA_MESSAGE_HPP_ */
