/*
 *  DCCManager.cpp
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

#include <Circada/DCCManager.hpp>
#include <Circada/Utils.hpp>

#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include <iostream>

namespace Circada {

DCCManager::DCCManager(Configuration& config, Events& evt, WindowManager& win_mgr) throw (DCCManagerException)
    : config(config), evt(evt), win_mgr(win_mgr), destroying(false)
{
    /* create transfer directory */
    storage_directory = config.get_working_directory() + "/transfer";
    try {
        create_directory(storage_directory);
    } catch (const UtilsException& e) {
        throw DCCManagerException("Cannot create transfer directory: " + std::string(e.what()));
    }
}

DCCManager::~DCCManager() {
    destroying = true;
    ScopeMutex lock(&mtx);
    while (dccs.size()) {
        DCC *dcc = dccs[0];
        destroy_dcc_nolock(dcc);
    }
}

DCC *DCCManager::create_chat_in(Session *s, const std::string& nick) throw (DCCManagerException) {
    /* we are offering a chat, we create an incoming socket */
    DCCChatIn *dcc = 0;
    try {
        dcc = new DCCChatIn(s, *this, nick);
        dcc->start();
    } catch (const std::exception& e) {
        if (dcc) delete dcc;
        throw DCCManagerException(e.what());
    }

    ScopeMutex lock(&mtx);
    dccs.push_back(dcc);

    return dcc;
}

DCC *DCCManager::create_xfer_in(Session *s, const std::string& nick, const std::string& filename, std::string& out_filename, u32& filesize) throw (DCCManagerException) {
    /* we are offering a file, we create an incoming socket */
    DCCXferIn *dcc = 0;
    try {
        get_fileinfo(filename, out_filename, filesize);
        dcc = new DCCXferIn(s, *this, nick, filename, filesize);
        dcc->start();
    } catch (const std::exception& e) {
        if (dcc) delete dcc;
        throw DCCManagerException(e.what());
    }

    ScopeMutex lock(&mtx);
    dccs.push_back(dcc);

    return dcc;
}

DCC *DCCManager::create_chat_out(Session *s, const std::string& nick, unsigned long address, unsigned short port) throw (DCCManagerException) {
    /* we can connect to an offered socket */
    DCCChatOut *dcc = 0;
    try {
        dcc = new DCCChatOut(s, *this, nick, address, port);
    } catch (const std::exception& e) {
        throw DCCManagerException(e.what());
    }

    ScopeMutex lock(&mtx);
    dccs.push_back(dcc);

    return dcc;
}

DCC *DCCManager::create_xfer_out(Session *s, const std::string& nick, const std::string& filename, u32 filesize, unsigned long address, unsigned short port) throw (DCCManagerException)
{
    /* we can connect to an offered socket */
    DCCXferOut *dcc = 0;
    try {
        dcc = new DCCXferOut(s, *this, nick, filename, filesize, address, port);
    } catch (const std::exception& e) {
        throw DCCManagerException(e.what());
    }

    ScopeMutex lock(&mtx);
    dccs.push_back(dcc);

    return dcc;
}

void DCCManager::dcc_change_his_nick(Session *s, const std::string& old_nick, const std::string& new_nick) {
    if (s) {
        ScopeMutex lock(&mtx);
        for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
            DCC *tmp_dcc = *it;
            if (tmp_dcc->get_session() == s && is_equal(tmp_dcc->get_his_nick(), old_nick)) {
                tmp_dcc->set_his_nick(new_nick);
            }
        }
    }
}

void DCCManager::dcc_change_my_nick(Session *s, const std::string& new_nick) {
    if (s) {
        ScopeMutex lock(&mtx);
        for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
            DCC *tmp_dcc = *it;
            if (tmp_dcc->get_session() == s) {
                tmp_dcc->set_my_nick(new_nick);
            }
        }
    }
}

void DCCManager::detach_dcc_from_irc_server(const DCC *dcc) {
    ScopeMutex lock(&mtx);

    for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
        DCC *tmp_dcc = *it;
        if (tmp_dcc == dcc) {
            tmp_dcc->reset_session();
            break;
        }
    }
}

void DCCManager::destroy_all_dccs_in_session(Session *s) {
    ScopeMutex lock(&mtx);
    bool found;
    do {
        found = false;
        for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
            DCC *tmp_dcc = *it;
            if (tmp_dcc->get_session() == s) {
                found = true;
                destroy_dcc_nolock(tmp_dcc);
                break;
            }
        }
    } while (found);
}

void DCCManager::destroy_dcc(const DCC *dcc) {
    ScopeMutex lock(&mtx);
    destroy_dcc_nolock(dcc);
}

void DCCManager::destroy_dcc_nolock(const DCC *dcc) {
    DCC *my_dcc = const_cast<DCC *>(dcc);
    my_dcc->set_will_be_killed();
    my_dcc->stop();
    win_mgr.detach_window(my_dcc);

    for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
        DCC *tmp_dcc = *it;
        if (tmp_dcc == my_dcc) {
            dccs.erase(it);
            break;
        }
    }

    delete my_dcc;
}

DCC::List& DCCManager::get_dccs() {
    return dccs;
}

std::string DCCManager::get_storage(const std::string& filename) {
    std::string storage = storage_directory + "/" + filename;
    return storage;
}

std::string DCCManager::get_part_filename(const std::string& filename) {
    return "~" + filename + ".part";
}

bool DCCManager::set_resume_position(Session *s, unsigned short port, u32& startpos, DCC*& out_dcc) throw (DCCManagerException) {
    ScopeMutex lock(&mtx);

    out_dcc = 0;
    for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
        DCC *dcc = *it;
        if (dcc->get_type() == DCCTypeXfer) {
            if (dcc->get_dccio().get_port() == port) {
                DCCXfer *dcc_xfer = static_cast<DCCXfer *>(dcc);
                out_dcc = dcc_xfer;
                switch (dcc->get_dccio().get_direction()) {
                    case DCCDirectionIncoming:
                    {
                        try {
                            if (startpos > get_filesize(dcc_xfer->get_filename())) {
                                startpos = 0;
                            }
                        } catch (const std::exception&) {
                            startpos = 0;
                        }
                        break;
                    }

                    case DCCDirectionOutgoing:
                    {
                        try {
                            std::string outfile = get_storage(dcc_xfer->get_filename());
                            if (startpos > get_filesize(outfile)) {
                                throw DCCManagerException("Resume position is out of received file.");
                            }
                        } catch (const std::exception& e) {
                            throw DCCManagerException(e.what());
                        }
                        break;
                    }
                }
                dcc_xfer->set_resume_position(startpos);
                return true;
            }
        }
    }

    return false;
}

Mutex& DCCManager::get_mutex() {
    return mtx;
}

bool DCCManager::is_handle_valid(const DCC *dcc) {
    ScopeMutex lock(&mtx);
    return is_handle_valid_nolock(dcc);
}

bool DCCManager::is_handle_valid_nolock(const DCC *dcc) {
    for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
        DCC *tmp_dcc = *it;
        if (tmp_dcc == dcc) {
            return true;
        }
    }

    return false;
}

DCCHandleBase *DCCManager::get_dcc_handle_base(DCCHandle *handle) {
    return reinterpret_cast<DCCHandleBase *>(handle);
}

DCCHandle::List DCCManager::get_all_handles(Session *s) {
    ScopeMutex lock(&mtx);
    DCCHandle::List handles;
    for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
        DCC *tmp_dcc = *it;
        if (!s || tmp_dcc->get_session() == s) {
            DCCHandle handle(*this, tmp_dcc);
            handles.push_back(handle);
        }
    }

    return handles;
}

void DCCManager::accept_dcc_handle(DCCHandle dcc, bool force) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    ScopeMutex lock(&mtx);
    DCCHandleBase& base = *get_dcc_handle_base(&dcc);
    DCC *tmp_dcc = const_cast<DCC *>(base.get_handle());
    if (!is_handle_valid_nolock(tmp_dcc)) {
        throw DCCInvalidHandleException();
    }
    if (tmp_dcc->is_running()) {
        throw DCCOperationNotPermittedException();
    }
    Session *s = tmp_dcc->get_session();
    if (!force && s && tmp_dcc->get_type() == DCCTypeXfer && tmp_dcc->get_dccio().get_direction() == DCCDirectionOutgoing) {
        DCCXfer *dcc_xfer = static_cast<DCCXfer *>(tmp_dcc);
        const std::string& filename = dcc_xfer->get_filename();
        /* check, if receiving file is in progress */
        for (DCC::List::iterator it = dccs.begin(); it != dccs.end(); it++) {
            DCC *list_dcc = *it;
            if (list_dcc->get_type() == DCCTypeXfer) {
                DCCXfer *dcc_xfer = static_cast<DCCXfer *>(list_dcc);
                if (dcc_xfer->get_filename() == filename && list_dcc->is_running()) {
                    throw DCCOperationNotPermittedException();
                }
            }
        }
        /* setup resume position */
        if (file_exists(get_storage(get_part_filename(filename)))) {
            try {
                u32 startpos = static_cast<u32>(get_filesize(get_storage(filename)));
                /* avoid a send hang bug in weechat */
                if (startpos && dcc_xfer->get_filesize() == startpos) {
                    startpos--;
                }
                /* send resume request */
                const std::string& nick = dcc_xfer->get_his_nick();
                char port_str[32];
                char startpos_str[32];
                sprintf(port_str, "%hu", dcc_xfer->get_dccio().get_port());
                sprintf(startpos_str, "%u", startpos);
                std::string reply("PRIVMSG " + nick + " :\x01");
                reply += "DCC RESUME " + filename + " ";
                reply += port_str;
                reply += " ";
                reply += startpos_str;
                reply += "\x01";
                s->send(reply);
            } catch (const Exception& e) {
                /* start over */
                tmp_dcc->start();
            }
        } else {
            tmp_dcc->start();
        }
    } else {
        tmp_dcc->start();
    }
}

void DCCManager::decline_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    ScopeMutex lock(&mtx);
    DCCHandleBase& base = *get_dcc_handle_base(&dcc);
    const DCC *tmp_dcc = base.get_handle();
    if (!is_handle_valid_nolock(tmp_dcc)) {
        throw DCCInvalidHandleException();
    }
    if (tmp_dcc->is_running()) {
        throw DCCOperationNotPermittedException();
    }
    destroy_dcc_nolock(tmp_dcc);
}

void DCCManager::abort_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException) {
    ScopeMutex lock(&mtx);
    DCCHandleBase& base = *get_dcc_handle_base(&dcc);
    const DCC *tmp_dcc = base.get_handle();
    if (!is_handle_valid_nolock(tmp_dcc)) {
        throw DCCInvalidHandleException();
    }
    destroy_dcc_nolock(tmp_dcc);
}

void DCCManager::send_dcc_msg(DCCHandle dcc, const std::string& msg) throw (DCCInvalidHandleException, DCCOperationNotPermittedException, SocketException) {
    ScopeMutex lock(&mtx);
    DCCHandleBase& base = *get_dcc_handle_base(&dcc);
    const DCC *tmp_dcc = base.get_handle();
    if (!is_handle_valid_nolock(tmp_dcc)) {
        throw DCCInvalidHandleException();
    }
    if (tmp_dcc->get_type() != DCCTypeChat) {
        throw DCCOperationNotPermittedException();
    }
    tmp_dcc->get_dccio().get_socket().send(msg + "\n");
}

Window *DCCManager::get_window_from_dcc_handle(DCCHandle dcc) throw (DCCInvalidHandleException, DCCOperationNotPermittedException) {
    DCCHandleBase& base = *get_dcc_handle_base(&dcc);
    const DCC *tmp_dcc = base.get_handle();
    return win_mgr.get_window(tmp_dcc, "");
}

Configuration& DCCManager::get_configuration() {
    return config;
}

void DCCManager::dcc_mgr_chat_begins(const DCC *dcc) throw () {
    SessionWindow *w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
    detach_dcc_from_irc_server(dcc);
    std::string topic("DCC CHAT with " + dcc->get_his_nick());
    w->set_topic(topic);
    evt.change_topic(0, w, topic);
    evt.dcc_chat_begins(w, DCCChatHandle(*this, dcc));
}

void DCCManager::dcc_mgr_chat_ended(const DCC *dcc, const std::string& reason) throw () {
    if (!destroying) {
        SessionWindow *w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
        win_mgr.detach_window(dcc);
        evt.dcc_chat_ended(w, DCCChatHandle(*this, dcc), reason);
        if (!dcc->get_will_be_killed()) {
            destroy_dcc(dcc);
        }
    }
}

void DCCManager::dcc_mgr_xfer_begins(const DCC *dcc) throw () {
    SessionWindow *w = 0;
    if (config.is_true(config.get_value("", "dcc_xfer_in_window", "0"))) {
        w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
    }
    detach_dcc_from_irc_server(dcc);
    if (w) {
        std::string topic("DCC XFER with " + dcc->get_his_nick());
        w->set_topic(topic);
        evt.change_topic(0, w, topic);
    }
    evt.dcc_xfer_begins(w, DCCXferHandle(*this, dcc));
}

void DCCManager::dcc_mgr_xfer_ended(const DCC *dcc) throw () {
    if (!destroying) {
        SessionWindow *w = 0;
        if (config.is_true(config.get_value("", "dcc_xfer_in_window", "0"))) {
            w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
        }
        win_mgr.detach_window(dcc);
        evt.dcc_xfer_ended(w, DCCXferHandle(*this, dcc));
        if (!dcc->get_will_be_killed()) {
            destroy_dcc(dcc);
        }
    }
}

void DCCManager::dcc_mgr_message(const DCC *dcc, const std::string& nick, const std::string& ctcp, const std::string& msg) throw () {
    SessionWindow *w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
    evt.dcc_message(w, DCCChatHandle(*this, dcc), ctcp, msg);
}

void DCCManager::dcc_mgr_send_progress(const DCC *dcc, u32 sent_bytes, u32 total_bytes) throw () {
    SessionWindow *w = 0;
    if (config.is_true(config.get_value("", "dcc_xfer_in_window", "0"))) {
        w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
    }
    evt.dcc_send_progress(w, DCCXferHandle(*this, dcc));
}

void DCCManager::dcc_mgr_receive_progress(const DCC *dcc, u32 received_bytes, u32 total_bytes) throw () {
    SessionWindow *w = 0;
    if (config.is_true(config.get_value("", "dcc_xfer_in_window", "0"))) {
        w = win_mgr.create_window(&evt, dcc, dcc->get_my_nick(), dcc->get_his_nick());
    }
    evt.dcc_receive_progress(w, DCCXferHandle(*this, dcc));
}

void DCCManager::dcc_mgr_timedout(const DCC *dcc, const std::string& reason) throw () {
    if (!dcc->get_will_be_killed()) {
        Session *s = dcc->get_session();
        if (s) {
            Window *w = s->get_server_window();
            if (w) {
                if (dcc->get_type() == DCCTypeChat) {
                    evt.dcc_offered_chat_timedout(s, w, DCCChatHandle(*this, dcc), reason);
                } else {
                    evt.dcc_offered_xfer_timedout(s, w, DCCXferHandle(*this, dcc), reason);
                }
            }
        }
        destroy_dcc(dcc);
    }
}

void DCCManager::dcc_mgr_failed(const DCC *dcc, const std::string& reason) throw () {
    if (!destroying) {
        SessionWindow *w = win_mgr.get_window(dcc, dcc->get_his_nick());
        win_mgr.detach_window(dcc);
        if (!dcc->get_will_be_killed()) {
            if (dcc->get_type() == DCCTypeChat) {
                evt.dcc_chat_failed(w, DCCChatHandle(*this, dcc), reason);
            } else {
                evt.dcc_xfer_failed(w, DCCXferHandle(*this, dcc), reason);
            }
            destroy_dcc(dcc);
        }
    }
}

off_t DCCManager::get_filesize(const std::string& filename) throw (DCCManagerException) {
    struct stat info;
    if (stat(filename.c_str(), &info) < 0) {
        throw DCCManagerException(strerror(errno));
    }

    return info.st_size;
}

void DCCManager::get_fileinfo(const std::string& filename, std::string& out_filename, u32& out_size) throw (DCCManagerException) {
    off_t sz = get_filesize(filename);
    if (sz > static_cast<u32>(-1)) {
        throw DCCManagerException("File size too big (size > 4294967295 bytes).");
    }
    out_size = static_cast<u32>(sz);
    reduce_filename(filename, out_filename);
}

void DCCManager::reduce_filename(const std::string& filename, std::string& out_filename) {
    static std::string allowed_characters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.");

    out_filename.clear();
    size_t pos = filename.length();
    do {
        pos--;
        char c = filename[pos];
        if (c == '/' || c == '\\') {
            break;
        }
        if (allowed_characters.find(c) == std::string::npos) {
            out_filename.insert(0, 1, '_');
        } else {
            out_filename.insert(0, 1, c);
        }
    } while (pos);
}

} /* namespace Circada */
