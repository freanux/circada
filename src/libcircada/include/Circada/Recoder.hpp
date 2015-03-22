/*
 *  Recoder.hpp
 *
 *  Created by freanux on Feb 22, 2015
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

#ifndef _CIRCADA_RECODER_HPP_
#define _CIRCADA_RECODER_HPP_

#include <string>
#include <vector>

namespace Circada {

typedef std::vector<std::string> Encodings;

class Recoder {
public:
    Recoder(const Encodings& encodings);
    virtual ~Recoder() { }

    void recode(std::string& text) const;

private:
    static const int RecodeMaxBuffer;
    Encodings encodings;
};

} /* namespace Circada */

#endif /* _CIRCADA_RECODER_HPP_ */
