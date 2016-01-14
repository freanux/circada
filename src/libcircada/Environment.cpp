/*
 *  Environment.cpp
 *
 *  Created by freanux on Mar 7, 2015
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

#include "Circada/Environment.hpp"

#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

namespace Circada {

    std::string Environment::get_username() throw (EnvironmentException) {
        struct passwd *pw;
        pw = getpwuid(geteuid());
        if (!pw) {
            throw EnvironmentException();
        }
        return std::string(pw->pw_name);
    }

    std::string Environment::get_home_directory() throw (EnvironmentException) {
        struct passwd *pw;
        pw = getpwuid(geteuid());
        if (!pw) {
            throw EnvironmentException();
        }
        return std::string(pw->pw_dir);
    }

    std::string Environment::get_uname() throw (EnvironmentException) {
        struct utsname ud;
        uname(&ud);
        return std::string(ud.sysname);
    }

} /* namespace Circada */
