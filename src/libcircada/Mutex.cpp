/*
 *  Mutex.cpp
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

#include <Circada/Mutex.hpp>

#include <algorithm>

namespace Circada {

    Mutex::Mutex() throw (MutexException) {
        try {
            mutex = new mutex_t;
        } catch (const std::exception& e) {
            throw MutexException(e.what());
        }
        pthread_mutex_init(&mutex->h_mutex, NULL);
    }

    Mutex::~Mutex() {
        pthread_mutex_destroy(&mutex->h_mutex);
        delete mutex;
    }

    void Mutex::lock() {
        pthread_mutex_lock(&mutex->h_mutex);
    }

    bool Mutex::try_lock() {
        return (pthread_mutex_trylock(&mutex->h_mutex) == 0);
    }

    void Mutex::unlock() {
        pthread_mutex_unlock(&mutex->h_mutex);
    }

    ScopeMutex::ScopeMutex(Mutex *mtx) : mtx(mtx) {
        this->mtx->lock();
    }

    ScopeMutex::~ScopeMutex() {
        this->mtx->unlock();
    }

    void ScopeMutex::lock() {
        this->mtx->lock();
    }

    void ScopeMutex::unlock() {
        this->mtx->lock();
    }

} /* namespace Circada */
