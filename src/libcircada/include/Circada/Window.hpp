/*
 *  Window.hpp
 *
 *  Created by freanux on Feb 15, 2015
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

#ifndef _CIRCADA_WINDOW_HPP_
#define _CIRCADA_WINDOW_HPP_

#include "Circada/Flags.hpp"
#include "Circada/Nick.hpp"
#include "Circada/DCC.hpp"

#include <vector>
#include <map>
#include <time.h>

namespace Circada {

    enum WindowType {
        WindowTypeApplication = 0,
        WindowTypeServer,
        WindowTypeChannel,
        WindowTypePrivate,
        WindowTypeDCC
    };

    enum WindowAction {
        WindowActionNone = 0,
        WindowActionNoise,
        WindowActionChat,
        WindowActionAlert
    };

    struct Netsplit {
        typedef std::vector<std::string> Nicks;

        Nicks nicks;
        struct timeval last_netsplit;
    };

    class Window {
    public:
        Window() { }
        virtual ~Window() { }

    public:
        virtual WindowType get_window_type() const = 0;
        virtual const std::string& get_name() const = 0;
        virtual const std::string& get_topic() const = 0;
        virtual std::string get_flags() = 0;
        virtual WindowAction get_action() = 0;
        virtual bool set_action(WindowAction action) = 0;
        virtual void reset_action() = 0;

        virtual Nick::List& get_nicks() = 0;
        virtual Nick *get_nick(const std::string& nick) = 0;
        virtual char get_nick_flag(const std::string& nick) = 0;
    };

    class Session;

    class SessionWindow : public Window {
    private:
        SessionWindow(const SessionWindow& rhs);
        SessionWindow operator=(const SessionWindow& rhs);

    public:
        typedef std::vector<SessionWindow *> List;
        typedef std::map<std::string, Netsplit> Netsplits;

        SessionWindow(const std::string& name, const std::string& topic);
        SessionWindow(Session *s, WindowType type, const std::string& name, ServerNickPrefix *snp);
        SessionWindow(Session *s, const DCC *dcc, const std::string& name, ServerNickPrefix *snp);
        virtual ~SessionWindow() { }

        Session *get_session();
        void set_name(const std::string& name);
        const std::string& get_plain_name() const;
        bool set_topic(const std::string& topic, bool force);
        bool set_topic(const std::string& topic);
        bool set_action(WindowAction action);
        void set_flags(const std::string& new_flags, bool new_set);
        void add_nick(const std::string& nick, bool no_sort);
        void change_nick(const std::string& old_nick, const std::string& new_nick);
        void remove_nick(const std::string& nick);
        void sort_nicks();
        bool print_netsplit(const std::string& quit_msg, struct timeval now);
        void add_netsplit_nick(const std::string& quit_msg, const std::string& nick);
        bool is_netsplit_over(const std::string& nick);
        const DCC *get_dcc() const;
        void set_dcc(const DCC *dcc);
        DCCType get_ddc_type() const;
        bool is_dcc_window() const;

        void reset_dcc_handler();

        /* Window */
        virtual WindowType get_window_type() const;
        virtual const std::string& get_name() const;
        virtual const std::string& get_topic() const;
        virtual std::string get_flags();
        virtual WindowAction get_action();
        virtual void reset_action();

        virtual Nick::List& get_nicks();
        virtual Nick *get_nick(const std::string& nick);
        virtual char get_nick_flag(const std::string& nick);

    private:
        Session *session;
        WindowType type;
        std::string name;
        std::string dcc_name;
        std::string topic;
        ServerNickPrefix *snp;
        const DCC *dcc;
        DCCType dcc_type;

        Flags flags;
        WindowAction action;

        Nick::List nicks;
        Netsplits netsplits;

        Netsplit& get_netsplit(const std::string& quit_msg);
        void remove_nick_from_netsplits(const std::string& nick);
    };

} /* namespace Circada */

#endif /* _CIRCADA_WINDOW_HPP_ */
