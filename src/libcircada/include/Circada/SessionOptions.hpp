/*
 *  SessionOptions.hpp
 *
 *  Created by freanux on Feb 24, 2015
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

#ifndef _CIRCADA_SESSIONOPTIONS_HPP_
#define _CIRCADA_SESSIONOPTIONS_HPP_

#include "Circada/Environment.hpp"

#include <string>

namespace Circada {

    class SessionOptions {
    public:
        SessionOptions();
        virtual ~SessionOptions();

        std::string name;
        std::string server;
        unsigned short port;
        std::string nick;
        std::string alternative_nick;
        std::string user;
        std::string real_name;
        std::string ca_file;
        std::string cert_file;
        std::string key_file;
        std::string tls_priority;
        bool user_invisible;    /* try to request to be invisible    */
        bool receive_wallops;   /* try to request to receive wallops */
    };

} /* namespace Circada */

#endif /* _CIRCADA_SESSIONOPTIONS_HPP_ */
