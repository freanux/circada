/*
 *  Utils.hpp
 *
 *  Created by freanux on Feb 17, 2015
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

#ifndef _CIRCADA_UTILS_HPP_
#define _CIRCADA_UTILS_HPP_

#include "Circada/Exception.hpp"

#include <string>
#include <vector>

namespace Circada {

    class UtilsException : public Exception {
    public:
        UtilsException(const char *msg) : Exception(msg) { }
        UtilsException(std::string msg) : Exception(msg) { }
    };

    typedef std::vector<std::string> TokenizedParams;

    int tokenize(int parameter_count, std::string in_str, TokenizedParams& out_params);
    bool is_equal(const std::string& a, const std::string& b);
    bool is_equal(const char *a, const char *b);
    std::string get_current_time();
    void trim(std::string& s);
    void to_lower(std::string& str);
    void to_upper(std::string& str);
    std::string unix_to_date(const std::string& st);
    void initialize_tls();
    void deinitialize_tls();
    bool is_valid_domain_char(const char domain_char);
    bool is_valid_domain_name(const std::string& domain);
    bool is_netsplit(const std::string& quit_message);
    bool is_numeric(const std::string& value);
    void create_directory(const std::string& directory);
    bool file_exists(const std::string& filename);
    std::string get_now();

} /* namespace Circada */

#endif /* _CIRCADA_UTILS_HPP_ */
