/*
 *  Configuration.cpp
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

#include "Circada/Configuration.hpp"
#include "Circada/Environment.hpp"
#include "Circada/Utils.hpp"

#include <fstream>
#include <unistd.h>
#include <pwd.h>
#include <cstdlib>

namespace Circada {

    const char *Configuration::ConfigurationFile = "config";

    Configuration::Configuration(const std::string& working_directory) : modified(false) {
        try {
            this->working_directory = Environment::get_home_directory() + "/" + working_directory;
        } catch (const EnvironmentException& e) {
            throw ConfigurationException(e.what());
        }
        try {
            create_directory(this->working_directory);
        } catch (const UtilsException& e) {
            throw ConfigurationException("Cannot create working directory: " + std::string(e.what()));
        }
        load();
    }

    Configuration::~Configuration() {
        /* try to save */
        try {
            save();
        } catch (...) {
            /* chomp */
        }
    }

    const std::string& Configuration::get_working_directory() {
        return working_directory;
    }

    void Configuration::load() {
        ScopeMutex lock(&mtx);

        std::string filename = working_directory + "/";
        filename += ConfigurationFile;

        std::ifstream f(filename.c_str());

        if (f.is_open()) {
            std::string line;
            while (getline(f, line)) {
                if (line.length()) {
                    size_t pos = line.find('=');
                    if (pos != std::string::npos) {
                        entries[line.substr(0, pos)] = line.substr(pos + 1);
                    } else {
                        throw ConfigurationException("Malformed expression in configuration file.");
                    }
                }
            }
        }
    }

    void Configuration::save() {
        ScopeMutex lock(&mtx);

        if (modified) {
            std::string filename = working_directory + "/";
            filename += ConfigurationFile;

            std::ofstream f(filename.c_str());
            if (!f.is_open()) {
                throw ConfigurationException("Cannot open file for writing: " + filename);
            }

            for (Entries::iterator it = entries.begin(); it != entries.end(); it++) {
                f << it->first << "=" << it->second << std::endl;
            }

            modified = false;
        }
    }

    void Configuration::set_value(const std::string& category, const std::string& key, const std::string& value) {
        ScopeMutex lock(&mtx);
        set_value_nolock(category, key, value);
    }

    const std::string& Configuration::get_value(const std::string& category, const std::string& key) {
        return get_value(category, key, empty_string);
    }

    const std::string& Configuration::get_value(const std::string& category, const std::string& key, const std::string& defaults) {
        ScopeMutex lock(&mtx);

        Entries::iterator it = entries.find(category + "." + key);
        if (it == entries.end()) {
            return defaults;
        }

        return it->second;
    }

    bool Configuration::is_true(const std::string& value) {
        return (atoi(value.c_str()) != 0);
    }

    const Configuration::Entries Configuration::get_entries() const {
        return entries;
    }

    void Configuration::validation(const std::string& s) {
        static std::string allowed_characters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_");

        size_t sz = s.length();
        for (size_t i = 0; i < sz; i++) {
            if (allowed_characters.find(s[i]) == std::string::npos) {
                throw ConfigurationException("Invalid character in category/key.");
            }
        }
    }

    void Configuration::set_value_nolock(const std::string& category, const std::string& key, const std::string& value) {
        validation(category);
        validation(key);

        if (value.length()) {
            entries[category + "." + key] = value;
            modified = true;
        } else {
            Entries::iterator it = entries.find(category + "." + key);
            if (it != entries.end()) {
                entries.erase(it);
                modified = true;
            }
        }
    }

} /* namespace Circada */
