/*
 *  Mutex.hpp
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

#ifndef _CIRCADA_MUTEX_HPP_
#define _CIRCADA_MUTEX_HPP_

#include <Circada/Exception.hpp>

#include <pthread.h>

namespace Circada {

class MutexException : public Exception {
public:
    MutexException(const char *msg) : Exception(msg) { }
    MutexException(std::string msg) : Exception(msg) { }
};

class Mutex {
private:
    Mutex(const Mutex& rhs);
    Mutex& operator=(const Mutex& rhs);

    typedef struct {
        pthread_mutex_t h_mutex;
    } mutex_t;

public:
    Mutex() throw (MutexException);
    virtual ~Mutex();

    void lock();
    bool try_lock();
    void unlock();

private:
    mutex_t *mutex;
};

class ScopeMutex {
private:
    ScopeMutex(const ScopeMutex& rhs);
    ScopeMutex& operator=(const ScopeMutex& rhs);

public:
    ScopeMutex(Mutex *mtx);
    virtual ~ScopeMutex();

    void lock();
    void unlock();

private:
    Mutex *mtx;
};

} /* namespace Circada */

#endif /* _CIRCADA_MUTEX_HPP_ */
