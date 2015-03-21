/*
 *  UTF8.hpp
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

#ifndef _UTF8_HPP_
#define _UTF8_HPP_

#include <string>

/* IMPORTANT NOTES:
 * This UTF8Iterator does NOT check, if a std::string is a valid UTF8 string.
 * You are able to overrun/underrun a malformed UTF8 string.
 */

class UTF8Iterator : public std::iterator<std::bidirectional_iterator_tag, int,
    std::string::difference_type, const int *, const int&>
{
public:
    UTF8Iterator();
    UTF8Iterator(std::string::const_iterator it);
    UTF8Iterator(std::string::iterator it);
    UTF8Iterator(std::string::const_iterator it, std::string::difference_type pos);
    UTF8Iterator(std::string::iterator it, std::string::difference_type pos);
    UTF8Iterator(const UTF8Iterator& rhs);
    UTF8Iterator& operator=(const UTF8Iterator& rhs);
    ~UTF8Iterator();

    UTF8Iterator& operator++();
    UTF8Iterator operator++(int);
    UTF8Iterator& operator--();
    UTF8Iterator operator--(int);
    unsigned int operator*() const;
    bool operator==(const UTF8Iterator& rhs) const;
    bool operator!=(const UTF8Iterator& rhs) const;
    bool operator==(std::string::iterator rhs) const;
    bool operator==(std::string::const_iterator rhs) const;
    bool operator!=(std::string::iterator rhs) const;
    bool operator!=(std::string::const_iterator rhs) const;
    UTF8Iterator operator+(std::string::difference_type pos);
    UTF8Iterator operator-(std::string::difference_type pos);
    UTF8Iterator& operator+=(std::string::difference_type pos);
    UTF8Iterator& operator-=(std::string::difference_type pos);
    const std::string& get_sequence() const;
    const std::string::const_iterator get_string_iterator() const;

private:
    std::string::const_iterator string_iterator;
    mutable unsigned int current_code_point;
    mutable bool is_dirty;
    mutable std::string sequence;

    void calc_current_code_point() const;
};

size_t get_utf8_length(const std::string& str);

#endif // _UTF8_HPP_
