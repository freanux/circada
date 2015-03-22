/*
 *  Flags.cpp
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

#include <Circada/Flags.hpp>

namespace Circada {

Flags::Flags() : possible_flags(POSSIBLE_FLAGS) {
    clear();
}

Flags::~Flags() { }

std::string Flags::get_flags() {
    std::string tmp;

    for (size_t i = 0; i < FLAGS_MAX_SIZE; i++) {
        if (flags[i] == '1') {
            tmp.push_back(possible_flags[i]);
        }
    }

    return (tmp.length() ? "+" + tmp : "");
}

void Flags::set_flags(const std::string& new_flags) {
    char set = '1';

    size_t sz = new_flags.size();
    size_t pos;
    for (size_t i = 0; i < sz; i++) {
        if (new_flags[i] == '+') {
            set = '1';
        } else if (new_flags[i] == '-') {
            set = '0';
        } else if ((pos = possible_flags.find(new_flags[i])) != std::string::npos) {
            flags[pos] = set;
        }
    }
}

void Flags::clear() {
    for (size_t i = 0; i < FLAGS_MAX_SIZE; i++) {
        flags[i] = '0';
    }
}

bool Flags::is_flag_set(char flag) {
    size_t pos = possible_flags.find(flag);
    if (pos != std::string::npos) {
        return (flags[pos] == '1');
    }

    return false;
}

} /* namespace Circada */
