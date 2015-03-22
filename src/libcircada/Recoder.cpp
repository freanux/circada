/*
 *  Recoder.cpp
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

#include <Circada/Recoder.hpp>

#include <iconv.h>
#include <cstring>

namespace Circada {

const int Recoder::RecodeMaxBuffer = 1024;

Recoder::Recoder(const Encodings& encodings) : encodings(encodings) { }

void Recoder::recode(std::string& text) const {
    const char *srcbuf = text.c_str();
    char dstbuf[RecodeMaxBuffer];

    /* try to recode text with all encodings first */
    size_t sz = encodings.size();
    if (sz) {
        for (size_t i = 0; i < sz; i++) {
            char *psrc = const_cast<char *>(srcbuf);
            char *pdst = dstbuf;
            size_t srclen = text.length();
            size_t dstlen = RecodeMaxBuffer;
            iconv_t conv = iconv_open("UTF-8", encodings[i].c_str());
            if (conv) {
                memset(dstbuf, 0, RecodeMaxBuffer);
                size_t bytes = iconv(conv, static_cast<char **>(&psrc), &srclen, static_cast<char **>(&pdst), &dstlen);
                iconv_close(conv);
                if (bytes < std::string::npos) {
                    text.assign(dstbuf);
                    return;
                }
            }
        }
    }

    /* ok, failed -> now, replace unrecognized characters with question marks */
    char *psrc = const_cast<char *>(srcbuf);
    char *pdst = dstbuf;
    size_t srclen = text.length();
    size_t dstlen = RecodeMaxBuffer;
    memset(dstbuf, 0, RecodeMaxBuffer);
    while (*psrc) {
        iconv_t conv = iconv_open("UTF-8", "UTF-8");
        if (conv) {
            size_t bytes = iconv(conv, static_cast<char **>(&psrc), &srclen, static_cast<char **>(&pdst), &dstlen);
            iconv_close(conv);
            if (bytes == std::string::npos) {
                *pdst = '?';
                pdst++;
                dstlen--;
                psrc++;
                srclen--;
            } else {
                break;
            }
        }
    }
    text.assign(dstbuf);
}

} /* namespace Circada */
