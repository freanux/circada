/*
 *  IOSync.cpp
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

#include <Circada/IOSync.hpp>

#include <cerrno>
#include <cstring>
#include <fcntl.h>

namespace Circada {

    IOSync::IOSync() throw (IOSyncException) {
        memset(signal_buffer, 0, sizeof(signal_buffer));

        int rv = pipe(pipefd);
        if (rv < 0) {
            throw IOSyncException(strerror(errno));
        }
    }

    IOSync::~IOSync() {
        close(pipefd[0]);
        close(pipefd[1]);
    }

    void IOSync::io_sync_set_non_blocking() throw (IOSyncException) {
        int flags = fcntl(pipefd[0], F_GETFL);
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
    }

    void IOSync::io_sync_set_blocking() throw (IOSyncException) {
        int flags = fcntl(pipefd[0], F_GETFL);
        fcntl(pipefd[0], F_SETFL, flags & ~O_NONBLOCK);
    }

    bool IOSync::io_sync_wait_for_event() throw (IOSyncException) {
        if (read(pipefd[0], signal_buffer, sizeof(signal_buffer)) < 1) {
            if (errno == EAGAIN) return false;
            if (errno == EINTR) return false;
            throw IOSyncException("Cannot read from pipe: " + std::string(strerror(errno)));
        }

        return true;
    }

    void IOSync::io_sync_signal_event() throw (IOSyncException) {
        write(pipefd[1], signal_buffer, sizeof(signal_buffer));
    }

} /* namespace Circada */
