// Timer.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Timer.cc for Blackbox - An X11 Window Manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "Timer.hh"

#include "Command.hh"

//use GNU extensions
#ifndef	_GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef	HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif

namespace FbTk {

Timer::TimerList Timer::m_timerlist;

Timer::Timer():m_timing(false), m_once(false), m_interval(0) {

}

Timer::Timer(RefCount<Command> &handler):
    m_handler(handler),
    m_timing(false),
    m_once(false),
    m_interval(0) {
}


Timer::~Timer() {
    if (isTiming()) stop();
}


void Timer::setTimeout(time_t t) {
    m_timeout.tv_sec = t / 1000;
    m_timeout.tv_usec = t;
    m_timeout.tv_usec -= (m_timeout.tv_sec * 1000);
    m_timeout.tv_usec *= 1000;
}


void Timer::setTimeout(const timeval &t) {
    m_timeout.tv_sec = t.tv_sec;
    m_timeout.tv_usec = t.tv_usec;
}

void Timer::setCommand(RefCount<Command> &cmd) {
    m_handler = cmd;
}

void Timer::start() {
    gettimeofday(&m_start, 0);

    // only add Timers that actually DO something
    if ((! m_timing || m_interval != 0) && *m_handler) {
        m_timing = true;
        addTimer(this); //add us to the list
    }        
}


void Timer::stop() {
    m_timing = false;
    removeTimer(this); //remove us from the list
}


void Timer::fireTimeout() {
    if (*m_handler)
        m_handler->execute();
}

void Timer::updateTimers(int fd) {
    fd_set rfds;
    timeval now, tm, *timeout = 0;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

  
    if (!m_timerlist.empty()) {
        gettimeofday(&now, 0);

        tm.tv_sec = tm.tv_usec = 0l;

        Timer *timer = m_timerlist.front();

        const timeval &start = timer->getStartTime();
        const timeval &length = timer->getTimeout();
        tm.tv_sec = start.tv_sec +
            length.tv_sec - now.tv_sec;
        tm.tv_usec = start.tv_usec +
            length.tv_usec - now.tv_usec;

        while (tm.tv_usec >= 1000000) {
            tm.tv_sec++;
            tm.tv_usec -= 1000000;
        }

        while (tm.tv_usec < 0) {
            if (tm.tv_sec > 0) {
                tm.tv_sec--;
                tm.tv_usec += 1000000;
            } else {
                tm.tv_usec = 0;
                break;
            }
        }

        if (tm.tv_sec < 0) {
            tm.tv_sec = 0;
            tm.tv_usec = 0;
        }

        timeout = &tm;
    }

    if (select(fd + 1, &rfds, 0, 0, timeout) != 0)
        // didn't time out! x events pending
        return;

    TimerList::iterator it;

    // someone set the date of the machine BACK
    // so we have to adjust the start_time
    static time_t last_time = time(0);
    if (time(0) < last_time) {
    
        time_t delta = time(0) - last_time;

        for (it = m_timerlist.begin(); it != m_timerlist.end(); it++) {
            (*it)->m_start.tv_sec += delta;
        }
    }


    // check for timer timeout
    gettimeofday(&now, 0);

    //must check end ...the timer might remove
    //it self from the list (should be fixed in the future)
    for(it = m_timerlist.begin(); it != m_timerlist.end(); ) {
        //This is to make sure we don't get an invalid iterator
        //when we do fireTimeout
        Timer &t = *(*it);
        const timeval &start = t.getStartTime();
        const timeval &length = t.getTimeout();

        tm.tv_sec = start.tv_sec +
            length.tv_sec;
        tm.tv_usec = start.tv_usec +
            length.tv_usec;

        if (((now.tv_sec < tm.tv_sec) ||
             (now.tv_sec == tm.tv_sec && now.tv_usec < tm.tv_usec)))
            break;

        t.fireTimeout();
        // restart the current timer so that the start time is updated
        if (! t.doOnce()) {
            if (t.getInterval() != 0) {
                it = m_timerlist.erase(it);
            }
            t.start();

            // Note that this mustn't be done if we're deleting the
            // entry from the list, so therefore it's not in the update
            // section of the for loop
            it++;
        } else {
            // Since the default stop behaviour results in the timer
            // being removed, we must remove it here, so that the iterator
            // lives well. Another option would be to add it to another
            // list, and then just go through that list and stop them all.
            it = m_timerlist.erase(it);
            t.stop();
        }
    }

    last_time = time(0);
}

void Timer::addTimer(Timer *timer) {
    assert(timer);
    int interval = timer->getInterval();
    // interval timers have their timeout change every time they are started!
    if (interval != 0) {
        timeval tm;
        tm.tv_sec = timer->getStartTime().tv_sec;
        tm.tv_usec = timer->getStartTime().tv_usec;

        // now convert to interval
        tm.tv_sec = interval - (tm.tv_sec % interval) - 1;
        tm.tv_usec = 1000000 - tm.tv_usec;
        if (tm.tv_usec == 1000000) {
            tm.tv_usec = 0;
            tm.tv_sec += 1;
        }
        timer->setTimeout(tm);
    }

    TimerList::iterator it = m_timerlist.begin();
    TimerList::iterator it_end = m_timerlist.end();
    int index = 0;
    for (; it != it_end; ++it, ++index) {
        if (((*it)->getTimeout().tv_sec > timer->getTimeout().tv_sec) ||
            (((*it)->getTimeout().tv_sec == timer->getTimeout().tv_sec) &&
             ((*it)->getTimeout().tv_usec >= timer->getTimeout().tv_usec))) {
            break;
        }
    }
    m_timerlist.insert(it, timer); 

}

void Timer::removeTimer(Timer *timer) {
    assert(timer);
    m_timerlist.remove(timer);
}
	
}; // end namespace FbTk
