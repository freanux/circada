/*
 *  DCCManager.hpp
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

#ifndef _CIRCADA_DCCMANAGER_HPP_
#define _CIRCADA_DCCMANAGER_HPP_

#include <Circada/Exception.hpp>
#include <Circada/Configuration.hpp>
#include <Circada/DCC.hpp>
#include <Circada/Session.hpp>
#include <Circada/Mutex.hpp>
#include <Circada/Events.hpp>
#include <Circada/WindowManager.hpp>

namespace Circada {

class DCCManagerException : public Exception {
public:
	DCCManagerException(const char *msg) : Exception(msg) { }
	DCCManagerException(std::string msg) : Exception(msg) { }
};

class DCCManager {
public:
    DCCManager(Configuration& config, Events& evt, WindowManager& win_mgr) throw (DCCManagerException);
	virtual ~DCCManager();

    DCC *create_chat_in(Session *s, const std::string& nick) throw (DCCManagerException);
	DCC *create_xfer_in(Session *s, const std::string& nick, const std::string& filename, std::string& out_filename, u32& filesize) throw (DCCManagerException);
	DCC *create_chat_out(Session *s, const std::string& nick, unsigned long address, unsigned short port) throw (DCCManagerException);
	DCC *create_xfer_out(Session *s, const std::string& nick, const std::string& filename, u32 filesize, unsigned long address, unsigned short port) throw (DCCManagerException);
	void dcc_change_his_nick(Session *s, const std::string& old_nick, const std::string& new_nick);
	void dcc_change_my_nick(Session *s, const std::string& new_nick);
	void detach_dcc_from_irc_server(const DCC *dcc);
	void destroy_all_dccs_in_session(Session *s);
	void destroy_dcc(const DCC *dcc);
	void destroy_dcc_nolock(const DCC *dcc);
	DCC::List& get_dccs();
	std::string get_storage(const std::string& filename);
	std::string get_part_filename(const std::string& filename);
	bool set_resume_position(Session *s, unsigned short port, u32& startpos, DCC*& out_dcc) throw (DCCManagerException);
	Mutex& get_mutex();
	bool is_handle_valid(const DCC *dcc);
	bool is_handle_valid_nolock(const DCC *dcc);
	DCCHandleBase *get_dcc_handle_base(DCCHandle *handle);
	DCCHandle::List get_all_handles(Session *s);
	void accept_dcc_handle(DCCHandle dcc, bool force) throw (DCCInvalidHandleException, DCCOperationNotPermittedException);
	void decline_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException);
	void abort_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException);
	void send_dcc_msg(DCCHandle dcc, const std::string& msg) throw (DCCInvalidHandleException, DCCOperationNotPermittedException, SocketException);
	Window *get_window_from_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException);
	Configuration& get_configuration();

    void dcc_mgr_chat_begins(const DCC *dcc) throw ();
	void dcc_mgr_chat_ended(const DCC *dcc, const std::string& reason) throw ();
	void dcc_mgr_xfer_begins(const DCC *dcc) throw ();
	void dcc_mgr_xfer_ended(const DCC *dcc) throw ();
	void dcc_mgr_message(const DCC *dcc, const std::string& nick, const std::string& ctcp, const std::string& msg) throw ();
	void dcc_mgr_send_progress(const DCC *dcc, u32 sent_bytes, u32 total_bytes) throw ();
	void dcc_mgr_receive_progress(const DCC *dcc, u32 received_bytes, u32 total_bytes) throw ();
	void dcc_mgr_timedout(const DCC *dcc, const std::string& reason) throw ();
	void dcc_mgr_failed(const DCC *dcc, const std::string& reason) throw ();

private:
    Configuration& config;
	Events& evt;
	WindowManager& win_mgr;
	bool destroying;
	std::string storage_directory;

	DCC::List dccs;
	Mutex mtx;

    off_t get_filesize(const std::string& filename) throw (DCCManagerException);
	void get_fileinfo(const std::string& filename, std::string& out_filename, u32& out_size) throw (DCCManagerException);
	void reduce_filename(const std::string& filename, std::string& out_filename);
};

} /* namespace Circada */

#endif /* _CIRCADA_DCCMANAGER_HPP_ */
