/*
 *  IOSync.hpp
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

#ifndef _CIRCADA_IOSYNC_HPP_
#define _CIRCADA_IOSYNC_HPP_

#include "Circada/Exception.hpp"

#include <unistd.h>

namespace Circada {

    class IOSyncException : public Exception {
    public:
        IOSyncException(const char *msg) : Exception(msg) { }
        IOSyncException(const std::string& msg) : Exception(msg) { }
    };

    class IOSync {
    private:
        IOSync(const IOSync& rhs);
        IOSync& operator=(const IOSync& rhs);

    public:
        IOSync() ;
        virtual ~IOSync();

        void io_sync_set_non_blocking();
        void io_sync_set_blocking();
        bool io_sync_wait_for_event();
        void io_sync_signal_event();

    private:
        int pipefd[2];
        char signal_buffer[1];
    };

} /* namespace Circada */

#endif /* _CIRCADA_IOSYNC_HPP_ */
