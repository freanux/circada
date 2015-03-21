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

#ifndef _CIRCADA_THREAD_HPP_
#define _CIRCADA_THREAD_HPP_

#include <pthread.h>
#include <signal.h>

namespace Circada {

class Thread {
public:
	Thread();
	virtual ~Thread();

protected:
	bool thread_start();
	void thread_join();
	void thread_signal(int sig);
	void thread_cancel();
	void thread_detach();
	virtual void thread() = 0;

private:
	pthread_t t;

	static void *thread_helper(void *o);
};

} /* namespace Circada */

#endif /* _CIRCADA_THREAD_HPP_ */
