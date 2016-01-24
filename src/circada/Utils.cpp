/*
 *  Utils.cpp
 *
 *  Created by freanux on Mar 8, 2015
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

#include "Utils.hpp"
#include "UTF8.hpp"

#include <cstdio>

static const int RecodeMaxSize = 1024;

size_t to_wstring(const std::string& from, std::wstring& to) {
    wchar_t buffer[RecodeMaxSize];
    size_t rv;

    rv = mbstowcs(buffer, from.c_str(), RecodeMaxSize - 1);
    to.assign(buffer);

    return rv;
}

int get_display_width(const std::string& utf8_sequence) {
    /* of one utf8 sequence/character */
    wchar_t dest;
    mbtowc(&dest, utf8_sequence.c_str(), utf8_sequence.length());
    int cwidth = wcwidth(dest);
    if (cwidth < 0) {
        cwidth = 0;
    }

    return cwidth;
}

int get_display_width_string(const std::string& utf8_string) {
    int sz = 0;

    UTF8Iterator it = utf8_string.begin();
    while (*it) {
        sz += get_display_width(it.get_sequence());
        it++;
    }

    return sz;
}