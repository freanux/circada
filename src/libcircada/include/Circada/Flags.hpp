/*
 *  Flags.hpp
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

#ifndef _CIRCADA_FLAGS_HPP_
#define _CIRCADA_FLAGS_HPP_

#include <Circada/RFC2812.hpp>

#include <string>

namespace Circada {

#define POSSIBLE_FLAGS CHARS_LETTER CHARS_DIGIT
#define FLAGS_MAX_SIZE sizeof(POSSIBLE_FLAGS)

class Flags {
public:
	Flags();
	virtual ~Flags();

	std::string get_flags();
	void set_flags(const std::string& new_flags);
	void clear();
	bool is_flag_set(char flag);

private:
	char flags[FLAGS_MAX_SIZE];
	std::string possible_flags;
};

} /* namespace Circada */

#endif /* _CIRCADA_FLAGS_HPP_ */
