/*
 *  LineFetcher.cpp
 *
 *  Created by freanux on Mar 1, 2015
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

#include <Circada/LineFetcher.hpp>

#include <cstring>

namespace Circada {

    LineFetcher::LineFetcher() { }

    LineFetcher::LineFetcher(const std::string& eol) : eol(eol) { }

    LineFetcher::~LineFetcher() { }

    void LineFetcher::flush() {
        data.clear();
    }

    size_t LineFetcher::fetch(Socket& socket, Lines& lines) throw (LineFetcherException) {
        size_t rlen, pos, cnt;
        char buffer[1024];
        std::string str;

        cnt = 0;
        std::memset(buffer, 0, sizeof(buffer));
        try {
            rlen = socket.receive(buffer, sizeof(buffer) - 1);
            if (rlen) {
                data.append(buffer);

                /* if no eol marker is defined, try to find one now */
                if (!eol.length()) {
                    if (data.find("\r\n") != std::string::npos) {
                        eol = "\r\n";
                    } else if (data.find("\r") != std::string::npos) {
                        eol = "\r";
                    } else if (data.find("\n") != std::string::npos) {
                        eol = "\n";
                    }
                }

                /* if eol is defined, parse buffer */
                if (eol.length()) {
                    while ((pos = data.find(eol, 0)) != std::string::npos) {
                        str = data.substr(0, pos);
                        data = data.substr(pos + eol.length());
                        lines.push_back(str);
                        cnt++;
                    }
                }
            }
        } catch (const SocketException& e) {
            throw LineFetcherException(e.what());
        }

        return cnt;
    }

} /* namespace Circada */
