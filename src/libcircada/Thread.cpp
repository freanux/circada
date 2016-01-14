/*
 *  Thread.hpp
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

#include "Circada/Thread.hpp"

namespace Circada {

    Thread::Thread() : t(0) { }

    Thread::~Thread() { }

    bool Thread::thread_start() {
        return (pthread_create(&t, NULL, thread_helper, this) == 0);
    }

    void Thread::thread_join() {
        pthread_join(t, NULL);
    }

    void Thread::thread_signal(int sig) {
        pthread_kill(t, sig);
    }

    void Thread::thread_cancel() {
        pthread_cancel(t);
    }

    void Thread::thread_detach() {
        pthread_detach(t);
    }

    void *Thread::thread_helper(void *o) {
        ((Thread *)o)->thread();
        return NULL;
    }

} /* namespace Circada */
