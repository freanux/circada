/*
 *  Configuration.hpp
 *
 *  Created by freanux on Mar 6, 2015
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

#ifndef _CIRCADA_CONFIGURATION_HPP_
#define _CIRCADA_CONFIGURATION_HPP_

#include "Circada/Exception.hpp"
#include "Circada/Mutex.hpp"

#include <string>
#include <map>

namespace Circada {

    class ConfigurationException : public Exception {
    public:
        ConfigurationException(const char *msg) : Exception(msg) { }
        ConfigurationException(std::string msg) : Exception(msg) { }
    };

    class Configuration {
    public:
        Configuration(const std::string& working_directory) throw (ConfigurationException);
        virtual ~Configuration();

        const std::string& get_working_directory();
        void load() throw (ConfigurationException);
        void save() throw (ConfigurationException);
        void set_value(const std::string& category, const std::string& key, const std::string& value) throw (ConfigurationException);
        const std::string& get_value(const std::string& category, const std::string& key) throw (ConfigurationException);
        const std::string& get_value(const std::string& category, const std::string& key, const std::string& defaults) throw (ConfigurationException);
        bool is_true(const std::string& value);

    private:
        static const char *ConfigurationFile;
        typedef std::map<std::string, std::string> Entries;

        bool modified;
        std::string working_directory;
        std::string empty_string;
        Entries entries;
        Mutex mtx;

        void load_defaults();
        void validation(const std::string& s) throw (ConfigurationException);
        void set_value_nolock(const std::string& category, const std::string& key, const std::string& value) throw (ConfigurationException);
    };

} /* namespace Circada */

#endif /* _CIRCADA_CONFIGURATION_HPP_ */
