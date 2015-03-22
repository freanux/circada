/*
 *  GlobalSettings.hpp
 *
 *  Created by freanux on Mar 4, 2015
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

#ifndef _CIRCADA_GLOBALSETTINGS_HPP_
#define _CIRCADA_GLOBALSETTINGS_HPP_

#include <Circada/Mutex.hpp>
#include <Circada/Recoder.hpp>

#include <string>

namespace Circada {

class GlobalSettings {
public:
    GlobalSettings();
    virtual ~GlobalSettings();

    const std::string& get_project_name();
    const std::string& get_project_version();
    const std::string& get_quit_message();
    bool get_injection();
    Encodings& get_encodings();

    Encodings encodings;

protected:
    std::string project_name;
    std::string project_version;
    std::string quit_message;
    bool inject_messages;

    Mutex settings_mtx;
};

} /* namespace Circada */

#endif /* _CIRCADA_GLOBALSETTINGS_HPP_ */
