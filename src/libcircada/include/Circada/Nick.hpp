/*
 *  Nick.hpp
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

#ifndef _CIRCADA_NICK_HPP_
#define _CIRCADA_NICK_HPP_

#include "Circada/Flags.hpp"
#include "Circada/RFC2812.hpp"

#include <string>
#include <vector>

namespace Circada {

    #define POSSIBLE_NICK_LETTERS CHARS_LETTER CHARS_DIGIT CHARS_SPECIAL "-"
    #define NICK_LETTERS_MAX_SIZE sizeof(POSSIBLE_NICK_LETTERS)

    class ServerNickPrefix {
    public:
        ServerNickPrefix() { }
        virtual ~ServerNickPrefix() { }

        virtual const std::string& get_nick_chars() const = 0;
        virtual const std::string& get_nick_symbols() const = 0;
    };

    class Nick {
    public:
        typedef std::vector<Nick> List;
        static bool is_nick_in_text(const std::string& nick, const std::string& text);

        Nick(const std::string& nick, ServerNickPrefix *snp);
        virtual ~Nick() { }

        bool operator<(const Nick& rhs) const;
        void set_flags(const std::string& new_flags);
        char get_flag();
        void set_nick(const std::string& nick);
        const std::string& get_nick();

    protected:
        std::string nick;
        Flags flags;
        ServerNickPrefix *snp;
        std::string sortnick;

        void set_sortnick();
    };

} /* namespace Circada */

#endif /* _CIRCADA_NICK_HPP_ */
