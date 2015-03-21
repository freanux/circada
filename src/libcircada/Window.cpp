/*
 *  Window.cpp
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

#include <Circada/Window.hpp>
#include <Circada/Session.hpp>
#include <Circada/Utils.hpp>

#include <algorithm>

namespace Circada {

/******************************************************************************
 * SessionWindow
 ******************************************************************************/
SessionWindow::SessionWindow(const std::string& name, const std::string& topic)
    : session(0), type(WindowTypeApplication), name(name), topic(topic), snp(0),
      dcc(0) { }

SessionWindow::SessionWindow(Session *s, WindowType type, const std::string& name, ServerNickPrefix *snp)
    : session(s), type(type), name(name), dcc_name(name), topic(), snp(snp),
      dcc(0), dcc_type(DCCTypeNone)
{
	action = (type == WindowTypePrivate ? WindowActionAlert : WindowActionNoise);
}

SessionWindow::SessionWindow(Session *s, const DCC *dcc, const std::string& name, ServerNickPrefix *snp)
    : session(s), type(WindowTypeDCC),
      name(name), dcc_name((dcc->get_type() == DCCTypeChat ? "=" : "*") + name),
      topic(), snp(snp), dcc(dcc), dcc_type(dcc->get_type())
{
	action = WindowActionAlert;
}

Session *SessionWindow::get_session() {
    return session;
}

void SessionWindow::set_name(const std::string& name) {
	this->name = name;
	if (dcc_type == DCCTypeChat) {
        this->dcc_name = "=" + name;
	} else if (dcc_type == DCCTypeXfer) {
	    this->dcc_name = "*" + name;
	} else {
	    this->dcc_name = name;
	}
}

const std::string& SessionWindow::get_plain_name() const {
    return name;
}

bool SessionWindow::set_topic(const std::string& topic, bool force) {
	if (!this->topic.length() || this->topic != topic || force) {
		this->topic = topic;
		return true;
	}

	return false;
}

bool SessionWindow::set_topic(const std::string& topic) {
	return set_topic(topic, true);
}

bool SessionWindow::set_action(WindowAction action) {
	if (action > this->action) {
		this->action = action;
		return true;
	}

	return false;
}

void SessionWindow::set_flags(const std::string& new_flags, bool new_set) {
	if (new_set) flags.clear();
	flags.set_flags(new_flags);
}

void SessionWindow::add_nick(const std::string& nick, bool no_sort) {
	Nick n(nick, snp);
	remove_nick(n.get_nick());
	nicks.push_back(n);
	if (!no_sort) sort_nicks();
}

void SessionWindow::change_nick(const std::string& old_nick, const std::string& new_nick) {
	remove_nick_from_netsplits(old_nick);

	size_t sz = nicks.size();
	for (size_t i = 0; i < sz; i ++) {
		if (is_equal(nicks[i].get_nick(), old_nick)) {
			nicks[i].set_nick(new_nick);
			break;
		}
	}
	sort_nicks();
}

void SessionWindow::remove_nick(const std::string& nick) {
	size_t sz = nicks.size();
	for (size_t i = 0; i < sz; i ++) {
		if (is_equal(nicks[i].get_nick(), nick)) {
			nicks.erase(nicks.begin() + i);
			break;
		}
	}
}

void SessionWindow::sort_nicks() {
	std::sort(nicks.begin(), nicks.end());
}

bool SessionWindow::print_netsplit(const std::string& quit_msg, struct timeval now) {
	Netsplit& ns = get_netsplit(quit_msg);
	if (now.tv_sec - ns.last_netsplit.tv_sec < 5) {
		ns.last_netsplit = now;
		return false;
	}
	ns.last_netsplit = now;

	return true;
}

void SessionWindow::add_netsplit_nick(const std::string& quit_msg, const std::string& nick) {
	get_netsplit(quit_msg).nicks.push_back(nick);
}

bool SessionWindow::is_netsplit_over(const std::string& nick) {
	for (Netsplits::iterator it = netsplits.begin(); it != netsplits.end(); it++) {
		Netsplit::Nicks& n = it->second.nicks;
        /* look for nick */
		size_t sz = n.size();
        for (size_t i = 0; i < sz; i++) {
            if (is_equal(n[i], nick)) {
                netsplits.erase(it);
                return true;
            }
		}
	}

	/* delete empty netsplit lists */
	// TODO: if a netsplit exists longer than 1 day, delete this list.
	bool found;
	do {
		found = false;
		for (Netsplits::iterator it = netsplits.begin(); it != netsplits.end(); it++) {
			Netsplit::Nicks& n = it->second.nicks;
			if (!n.size()) {
				/* no more nicks in netsplit list, */
				/* -> all are gone, delete list    */
				netsplits.erase(it);
				found = true;
				break;
			}

		}
	} while (found);

	return false;
}

const DCC *SessionWindow::get_dcc() const {
	return dcc;
}

void SessionWindow::set_dcc(const DCC *dcc) {
    this->dcc = dcc;
    this->dcc_type = dcc->get_type();
}

bool SessionWindow::is_dcc_window() const {
    return (type == WindowTypeDCC);
}

DCCType SessionWindow::get_ddc_type() const {
    return dcc_type;
}

void SessionWindow::reset_dcc_handler() {
    dcc = 0;
}

Netsplit& SessionWindow::get_netsplit(const std::string& quit_msg) {
	if (netsplits.find(quit_msg) == netsplits.end()) {
		Netsplit ns;
		ns.last_netsplit.tv_sec = 0;
		ns.last_netsplit.tv_usec = 0;
		netsplits[quit_msg] = ns;
	}

	return netsplits[quit_msg];
}

void SessionWindow::remove_nick_from_netsplits(const std::string& nick) {
	for (Netsplits::iterator it = netsplits.begin(); it != netsplits.end(); it++) {
		Netsplit::Nicks& n = it->second.nicks;
		for (Netsplit::Nicks::iterator itn = n.begin(); itn != n.end(); itn++) {
			if (is_equal(*itn, nick)) {
				n.erase(itn);
				return;
			}
		}
	}
}

/******************************************************************************
 * Window
 ******************************************************************************/
WindowType SessionWindow::get_window_type() const {
	return type;
}

const std::string& SessionWindow::get_name() const {
    if (dcc_type == DCCTypeChat) {
        return dcc_name;
    } else if (dcc_type == DCCTypeXfer) {
        return dcc_name;
    }

    return name;
}

const std::string& SessionWindow::get_topic() const {
	return topic;
}

std::string SessionWindow::get_flags() {
	if (type == WindowTypeChannel) {
		return flags.get_flags();
	}

	return "";
}

WindowAction SessionWindow::get_action() {
	return action;
}

void SessionWindow::reset_action() {
	this->action = WindowActionNone;
}

Nick::List& SessionWindow::get_nicks() {
	return nicks;
}

Nick *SessionWindow::get_nick(const std::string& nick) {
	size_t sz = nicks.size();

	for (size_t i = 0; i < sz; i++) {
		if (is_equal(nicks[i].get_nick(), nick)) return &nicks[i];
	}

	return 0;
}

char SessionWindow::get_nick_flag(const std::string& nick) {
	Nick *n = get_nick(nick);

	if (n) {
		return n->get_flag();
	}

	return ' ';
}

} /* namespace Circada */
