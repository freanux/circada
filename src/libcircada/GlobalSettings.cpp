/*
 *  GlobalSettings.cpp
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

#include <Circada/GlobalSettings.hpp>
#include <Circada/Global.hpp>

namespace Circada {

GlobalSettings::GlobalSettings()
    : project_name(PROJECT_NAME), project_version(PROJECT_VERSION), quit_message(QUIT_MESSAGE), inject_messages(true) { }

GlobalSettings::~GlobalSettings() { }

const std::string& GlobalSettings::get_project_name() {
    ScopeMutex lock(&settings_mtx);
    return project_name;
}

const std::string& GlobalSettings::get_project_version() {
    ScopeMutex lock(&settings_mtx);
    return project_version;
}

const std::string& GlobalSettings::get_quit_message() {
    return quit_message;
}

bool GlobalSettings::get_injection() {
    ScopeMutex lock(&settings_mtx);
    return this->inject_messages;
}

Encodings& GlobalSettings::get_encodings() {
    ScopeMutex lock(&settings_mtx);
    return encodings;
}

} /* namespace Circada */
