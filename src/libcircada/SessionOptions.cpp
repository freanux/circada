/*
 *  SessionOptions.cpp
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

#include "Circada/SessionOptions.hpp"

namespace Circada {

    SessionOptions::SessionOptions() throw (EnvironmentException)
        : name(), server(), port(6667),
          nick(Environment::get_username()),
          alternative_nick(nick + "_"),
          user(nick),
          real_name(nick),
          ca_file(), user_invisible(false), receive_wallops(false) { }

    SessionOptions::~SessionOptions() { }

} /* namespace Circada */
