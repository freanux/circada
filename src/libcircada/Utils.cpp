/*
 *  Utils.cpp
 *
 *  Created by freanux on Feb 17, 2015
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

#include <Circada/Utils.hpp>
#include <Circada/Mutex.hpp>

#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <algorithm>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gnutls/gnutls.h>
#include <gnutls/gnutlsxx.h>

namespace Circada {

Mutex global_mutex;

int tokenize(int parameter_count, std::string in_str, TokenizedParams& out_params) {
	int i, cnt;
	size_t pos;

	cnt = 0;
	for (i = 0; i < parameter_count - 1; i++) {
		if (!in_str.length()) break;
		pos = in_str.find(' ');
		if (pos != std::string::npos) {
			out_params.push_back(in_str.substr(0, pos));
			in_str = in_str.substr(pos + 1);
			cnt++;
		} else {
			out_params.push_back(in_str);
			in_str = "";
			cnt++;
			break;
		}
	}

	if (in_str.length()) {
		out_params.push_back(in_str);
		cnt++;
	}

	return cnt;
}

bool is_equal(const std::string& a, const std::string& b) {
	return !strcasecmp(a.c_str(), b.c_str());
}

bool is_equal(const char *a, const char *b) {
	return !strcasecmp(a, b);
}

std::string get_current_time() {
	ScopeMutex lock(&global_mutex);

	/* asctime() is not thread safe. we have to */
	/* protect this function with a mutex.      */
	std::string res;
	time_t t;
	time(&t);
	struct tm lt;
	localtime_r(&t, &lt);
	res = asctime(&lt);		/* not thread safe!!!! */

	return res.substr(0, res.length() - 1); /* cut away trailing \n */
}

void trim(std::string& s) {
	s.erase(0, s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
}

void to_lower(std::string& str) {
    std::transform(str.begin(), str.end(),str.begin(), ::tolower);
}

void to_upper(std::string& str) {
    std::transform(str.begin(), str.end(),str.begin(), ::toupper);
}

std::string unix_to_date(const std::string& st) {
	std::string res;

	time_t t = atoll(st.c_str());
	struct tm lt;
	localtime_r(&t, &lt);
	res = asctime(&lt);

	return res.substr(0, res.length() - 1); /* cut away trailing \n */
}

void initialize_tls() {
	gnutls_global_init();
}

void deinitialize_tls() {
	gnutls_global_deinit();
}

bool is_valid_domain_char(const char domain_char) {
	if (domain_char >= '0' && domain_char <= '9') return true;
	if (domain_char >= 'a' && domain_char <= 'z') return true;
	if (domain_char >= 'A' && domain_char <= 'Z') return true;
	if (domain_char == '-') return true;
	if (domain_char == '.') return true;
	if (domain_char == '*') return true;

	return false;
}
bool is_valid_domain_name(const std::string& domain) {
	size_t i, sz, dcnt, sdcnt;

	sz = domain.length();
	if (!sz) return false;
	if (domain[0] == '.' || domain[0] == '-') return false;
	if (domain[sz - 1] == '.' || domain[sz - 1] == '-') return false;
	dcnt = 0;
	sdcnt = 0;
	for (i = 0; i < sz; i++) {
		if (!is_valid_domain_char(domain[i])) {
			return false;
		} else if (domain[i] == '.') {
			if (!sdcnt) return false;
			sdcnt = 0;
			dcnt++;
		} else {
			sdcnt++;
		}
	}
	if (!dcnt || sdcnt < 2) return false;

	return true;
}

bool is_netsplit(const std::string& quit_message) {
	/*
	 * examples:
	 * *.net *.split
	 * central.euirc.net irc.inn.at.euirc.net
	 */
	/* ------------------------------------------------------------ */
	/*
	 * rules:
	 * 1) line : text blank text
	 * 2) text : at least 1 point,
	 * 			 no point at the beginning/end,
	 * 			 no dash at the beginning/end,
	 * 			 subdomain at least 1 character (sdcnt)
	 * 			 tld at least 2 character (last sdcnt)
	 * 			 only a-z, 0-9, -
	 * 3) blank: only 1 blank in the middle
	 */

	size_t i, sz, bpos, bcnt;
	std::string left, right;

	/* count blanks */
	sz = quit_message.length();
	bpos = bcnt = 0;
	for (i = 0; i < sz; i++) {
		if (quit_message[i] == ' ') {
			if (!bpos) bpos = i;
			bcnt++;
		}
	}
	if (bcnt != 1) return false;

	/* split into left and right string */
	left = quit_message.substr(0, bpos);
	right = quit_message.substr(bpos + 1);
	if (is_valid_domain_name(left) && is_valid_domain_name(right)) return true;

	return false;
}

bool is_numeric(const std::string& value) {
    return (value.find_first_not_of("0123456789.") == std::string::npos);
}

void create_directory(const std::string& directory) throw (UtilsException) {
	int rv = mkdir(directory.c_str(), S_IRWXU);
	if (rv && errno != EEXIST) {
		throw UtilsException(strerror(errno));
	}
}

bool file_exists(const std::string& filename) {
    struct stat info;
    return (stat(filename.c_str(), &info) == 0);
}

std::string get_now() {
	char buf[16];
	char fmt[] = "%H:%M:%S";
	time_t now = time(NULL);
	struct tm *tp;

	tp = localtime(&now);
	strftime(buf, sizeof(buf), fmt, tp);

	return buf;
}

} /* namespace Circada */

