/*
 *  Nick.cpp
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

#include <Circada/Nick.hpp>
#include <Circada/Utils.hpp>

#include <cstring>

namespace Circada {

bool Nick::is_nick_in_text(const std::string& nick, const std::string& text) {
	if (is_equal(nick, text)) return true;
	if (text.find(" " + nick) != std::string::npos) return true;
	if (text.find(nick + " ") != std::string::npos) return true;
	if (text.find("~" + nick) != std::string::npos) return true;
	if (text.find(nick + "~") != std::string::npos) return true;
	if (text.find(nick + ":") != std::string::npos) return true;
	if (text.find(nick + ",") != std::string::npos) return true;

	return false;
}

Nick::Nick(const std::string& nick, ServerNickPrefix *snp) {
	this->snp = snp;
	set_nick(nick);
}

bool Nick::operator<(const Nick& rhs) const {
	return (strcasecmp(sortnick.c_str(), rhs.sortnick.c_str()) < 0);
}

void Nick::set_flags(const std::string& new_flags) {
	flags.set_flags(new_flags);
	set_sortnick();
}

char Nick::get_flag() {
    if (snp) {
        const std::string& nick_chars = snp->get_nick_chars();
        const std::string& nick_symbols = snp->get_nick_symbols();
        size_t sz = nick_chars.length();

        for (size_t i = 0; i < sz; i++) {
            if (flags.is_flag_set(nick_chars[i])) {
                return nick_symbols[i];
            }
        }
    }

	return ' ';
}

void Nick::set_nick(const std::string& nick) {
    if (snp) {
        const std::string& nick_chars = snp->get_nick_chars();
        const std::string& nick_symbols = snp->get_nick_symbols();

        size_t pos = nick_symbols.find(nick[0]);
        if (pos != std::string::npos) {
            std::string tmp;
            tmp.push_back(nick_chars[pos]);
            flags.set_flags(tmp);
            this->nick = nick.substr(1);
        } else {
            this->nick = nick;
        }
    } else {
        this->nick = nick;
    }
	set_sortnick();
}

const std::string& Nick::get_nick() {
	return nick;
}

void Nick::set_sortnick() {
    if (snp) {
        const std::string& nick_symbols = snp->get_nick_symbols();
        size_t pos;

        sortnick.clear();
        char flag = get_flag();
        if ((pos = nick_symbols.find(flag)) != std::string::npos) {
            sortnick.push_back(static_cast<char>(pos + 1));
        } else {
            sortnick.push_back(flag);
        }
        sortnick += nick;
    } else {
        sortnick = nick;
    }
}

} /* namespace Circada */
