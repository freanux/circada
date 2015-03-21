/*
 *  UTF8.cpp
 *
 *  Created by freanux on Mar 8, 2015
 *  Copyright 2015 Circada Team. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in he hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "UTF8.hpp"

const unsigned char FirstBitMask = 128;
const unsigned char SecondBitMask = 64;
const unsigned char ThirdBitMask = 32;
const unsigned char FourthBitMask = 16;

UTF8Iterator::UTF8Iterator()
    : string_iterator(), current_code_point(0), is_dirty(true), sequence() { }

UTF8Iterator::UTF8Iterator(std::string::const_iterator it)
    : string_iterator(it), current_code_point(0), is_dirty(true), sequence() { }

UTF8Iterator::UTF8Iterator(std::string::iterator it)
    : string_iterator(it), current_code_point(0), is_dirty(true), sequence() { }

UTF8Iterator::UTF8Iterator(std::string::const_iterator it, std::string::difference_type pos)
    : string_iterator(it), current_code_point(0), is_dirty(true), sequence()
{
    operator+=(pos);
}

UTF8Iterator::UTF8Iterator(std::string::iterator it, std::string::difference_type pos)
    : string_iterator(it), current_code_point(0), is_dirty(true), sequence()
{
    operator+=(pos);
}

UTF8Iterator::UTF8Iterator(const UTF8Iterator& rhs)
    : string_iterator(rhs.string_iterator), current_code_point(rhs.current_code_point),
      is_dirty(rhs.is_dirty), sequence(rhs.sequence) { }

UTF8Iterator& UTF8Iterator::operator=(const UTF8Iterator& rhs) {
    string_iterator = rhs.string_iterator;
    current_code_point = rhs.current_code_point;
    is_dirty = rhs.is_dirty;
    return *this;
}

UTF8Iterator::~UTF8Iterator() { }

UTF8Iterator& UTF8Iterator::operator++() {
    char first_byte = *string_iterator;
    std::string::difference_type offset = 1;

    if(first_byte & FirstBitMask) {
        if(first_byte & ThirdBitMask) {
            if(first_byte & FourthBitMask) {
                offset = 4;
            } else {
                offset = 3;
            }
        }
        else {
            offset = 2;
        }
    }
    string_iterator += offset;
    is_dirty = true;

    return *this;
}

UTF8Iterator UTF8Iterator::operator++(int) {
    UTF8Iterator temp = *this;
    ++(*this);
    return temp;
}

UTF8Iterator& UTF8Iterator::operator--() {
    --string_iterator;
    if(*string_iterator & FirstBitMask) {
        --string_iterator;
        if((*string_iterator & SecondBitMask) == 0) {
            --string_iterator;
            if((*string_iterator & ThirdBitMask) == 0) {
                --string_iterator;
            }
        }
    }
    is_dirty = true;

    return *this;
}

UTF8Iterator UTF8Iterator::operator--(int) {
    UTF8Iterator temp = *this;
    --(*this);
    return temp;
}

unsigned int UTF8Iterator::operator*() const {
    if (is_dirty) {
        calc_current_code_point();
    }

    return current_code_point;
}

bool UTF8Iterator::operator==(const UTF8Iterator& rhs) const {
    return string_iterator ==  rhs.string_iterator;
}

bool UTF8Iterator::operator!=(const UTF8Iterator& rhs) const {
    return string_iterator !=  rhs.string_iterator;
}

bool UTF8Iterator::operator==(std::string::iterator rhs) const {
    return string_iterator ==  rhs;
}

bool UTF8Iterator::operator==(std::string::const_iterator rhs) const {
    return string_iterator ==  rhs;
}

bool UTF8Iterator::operator!=(std::string::iterator rhs) const {
    return string_iterator !=  rhs;
}

bool UTF8Iterator::operator!=(std::string::const_iterator rhs) const {
    return string_iterator !=  rhs;
}

UTF8Iterator UTF8Iterator::operator+(std::string::difference_type pos) {
    UTF8Iterator new_it(*this);
    for (std::string::difference_type i = 0; i < pos; i++) {
        ++new_it;
    }

    return new_it;
}

UTF8Iterator UTF8Iterator::operator-(std::string::difference_type pos) {
    UTF8Iterator new_it(*this);
    for (std::string::difference_type i = 0; i < pos; i++) {
        --(*this);
    }

    return new_it;
}

UTF8Iterator& UTF8Iterator::operator+=(std::string::difference_type pos) {
    for (std::string::difference_type i = 0; i < pos; i++) {
        ++(*this);
    }

    return *this;
}

UTF8Iterator& UTF8Iterator::operator-=(std::string::difference_type pos) {
    for (std::string::difference_type i = 0; i < pos; i++) {
        --(*this);
    }

    return *this;
}

const std::string& UTF8Iterator::get_sequence() const {
    if (is_dirty) {
        calc_current_code_point();
    }
    return sequence;
}

const std::string::const_iterator UTF8Iterator::get_string_iterator() const {
    return string_iterator;
}

void UTF8Iterator::calc_current_code_point() const {
    current_code_point = 0;
    char first_byte = *string_iterator;

    sequence.clear();
    sequence += first_byte;

    if(first_byte & FirstBitMask) {
        if(first_byte & ThirdBitMask) {
            if(first_byte & FourthBitMask) {
                current_code_point = (first_byte & 0x07) << 18;
                char second_byte = *(string_iterator + 1);
                sequence += second_byte;
                current_code_point +=  (second_byte & 0x3f) << 12;
                char third_byte = *(string_iterator + 2);
                sequence += third_byte;
                current_code_point +=  (third_byte & 0x3f) << 6;
                char fourth_byte = *(string_iterator + 3);
                sequence += fourth_byte;
                current_code_point += (fourth_byte & 0x3f);
            }
            else
            {
                current_code_point = (first_byte & 0x0f) << 12;
                char second_byte = *(string_iterator + 1);
                sequence += second_byte;
                current_code_point += (second_byte & 0x3f) << 6;
                char third_byte = *(string_iterator + 2);
                sequence += third_byte;
                current_code_point +=  (third_byte & 0x3f);
            }
        }
        else {
            current_code_point = (first_byte & 0x1f) << 6;
            char second_byte = *(string_iterator + 1);
            sequence += second_byte;
            current_code_point +=  (second_byte & 0x3f);
        }
    }
    else {
        current_code_point = first_byte;
    }

    is_dirty = false;
}

size_t get_utf8_length(const std::string& str) {
    size_t sz = 0;
    UTF8Iterator it = str.begin();
    while (*it) {
        sz++;
        it++;
    }

    return sz;
}
