/*
 *  Parser.cpp
 *
 *  Created by freanux on Feb 21, 2015
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

#include <Circada/Parser.hpp>
#include <Circada/Utils.hpp>

#include <cstring>
#include <iostream>

namespace Circada  {

Parser::Parser() : cleared(true), nicklist(0), nick_suffix(": ") { }

Parser::~Parser() { }

void Parser::set_nicklist(Nick::List *nicklist) {
	this->nicklist = nicklist;
}

void Parser::set_nick_suffix(const std::string& suffix) {
	nick_suffix = suffix;
}

const std::string& Parser::get_nick_suffix() {
	return nick_suffix;
}

std::string Parser::parse(const Session *s, const Window *w, std::string line, bool& external) throw (ParserException) {
	std::string output;

	if (line.length()) {
		if (!w) {
			throw ParserException("Please connect to an IRC server first.");
		}

		if (line[0] == '/') {
			std::string command;
			line = line.substr(1);
			size_t pos = line.find(' ');
			if (pos == std::string::npos) {
				pos = line.length();
				command = line.substr(0, pos);
				line = "";
			} else {
				command = line.substr(0, pos);
				line = line.substr(pos + 1);
				trim(line);
			}
			std::string command_space = command + " ";

			Command *cmd = commands;
			const char *irc_command = 0;
			std::string params;
			bool found = false;
			external = false;
			if (w->get_window_type() == WindowTypeDCC && is_equal(command.c_str(), "me")) {
                output = "\x01";
                output += "ACTION " + line + "\x01";
			} else {
                while (cmd->command) {
                    if (is_equal(cmd->command, command.c_str()) || is_equal(cmd->command, command_space.c_str())) {
                        found = true;
                        external = cmd->external;
                        if (!external && !s && cmd->function) {
                            throw ParserException("There is no IRC server behind this window.");
                        }
                        if (cmd->function) {
                            params = (this->*cmd->function)(s, w, line);
                        } else {
                            params = line;
                        }
                        irc_command = cmd->irc_command;
                        break;
                    }
                    cmd++;
                }
                if (!found) {
                    throw ParserException("Unknown command.");
                }
                if (irc_command) {
                    output.assign(irc_command);
                } else {
                    to_upper(command);
                    output = command;
                }
                output += " " + params;
                trim(output);
			}
		} else {
			if (w->get_window_type() == WindowTypeServer) {
				throw ParserException("You cannot send messages to the server.");
			} else if (w->get_window_type() == WindowTypeDCC) {
			    output = line;
			} else {
                output = "PRIVMSG " + w->get_name() + " :" + line;
			}
		}
	}

	return output;
}

void Parser::complete(std::string *text, int *cursor_pos) {
	if (!text || !cursor_pos) {
		return;
	}

	std::string& line = *text;
	int& curpos = *cursor_pos;

	if (cleared) {
		is_command = false;
		if (line.length()) {
			int testpos = curpos;
			bool found = false;
			while (testpos) {
				testpos--;
				if (line[testpos] == ' ') {
					found = true;
					break;
				}
			}
			if (!found && line[0] == '/') {
				is_command = true;
			}
		}
	}

	if (is_command) {
		if (cleared) {
			completion_origin_string = line;
			completion_origin_cursor_pos = *cursor_pos;
			last_command = commands;
			completion_search_pattern = line.substr(1, curpos - 1);
			completion_search_pattern_len = completion_search_pattern.length();
			cleared = false;
		}
		do_command_completion(line, curpos);
	} else {
		if (cleared) {
			completion_origin_string = line;
			completion_origin_cursor_pos = *cursor_pos;
			last_nick = 0;
			if (!curpos) {
				completion_search_pattern.clear();
				completion_nick_insert_pos = 0;
			} else {
				int spos = curpos - 1;
				while (spos) {
					if (line[spos] == ' ') {
						spos++;
						break;
					}
					spos--;
				}
				completion_search_pattern = line.substr(spos, curpos - spos);
				completion_nick_insert_pos = spos;
			}
			completion_search_pattern_len = completion_search_pattern.length();
			cleared = false;
		}
		do_nick_completion(line, curpos);
	}
}

void Parser::reset_tab_completion() {
	cleared = true;
}

void Parser::do_command_completion(std::string& line, int& curpos) {
	bool found = false;
	while (last_command->command) {
		Command *current_command = last_command;
		last_command++;
		if (!strncasecmp(completion_search_pattern.c_str(), current_command->command, completion_search_pattern_len)) {
			found = true;
			line = "/";
			line += current_command->command;
			line += completion_origin_string.substr(completion_origin_cursor_pos);
			curpos = strlen(current_command->command) + 1;
			break;
		}
	}
	if (!found) {
		last_command = commands;
		line = completion_origin_string;
		curpos = completion_origin_cursor_pos;
	}
}

void Parser::do_nick_completion(std::string& line, int& curpos) {
	if (nicklist) {
		bool found = false;
		int sz = static_cast<int>(nicklist->size());
		while (last_nick < sz) {
			int current_nick = last_nick;
			last_nick++;
			Nick::List& nl = *nicklist;
			Nick& nick = nl[current_nick];
			const std::string& nick_string = nick.get_nick();
			if (!strncasecmp(completion_search_pattern.c_str(), nick_string.c_str(), completion_search_pattern_len)) {
				found = true;
				if (completion_nick_insert_pos) {
					line = completion_origin_string.substr(0, completion_nick_insert_pos);
					line += nick_string;
				} else {
					line = nick_string + nick_suffix;
				}
				curpos = static_cast<int>(line.length());
				line += completion_origin_string.substr(completion_origin_cursor_pos);
				break;
			}
		}
		if (!found) {
			last_nick = 0;
			line = completion_origin_string;
			curpos = completion_origin_cursor_pos;
		}
	}
}

} /* namespace Circada */
