/*
 *  Utils.hpp
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

#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <string>
#include <cstdlib>
#include <cwchar>

size_t to_wstring(const std::string& from, std::wstring& to);
int get_display_width(const std::string& utf8_sequence);
int get_display_width_string(const std::string& utf8_string);

#endif // _UTILS_HPP_
