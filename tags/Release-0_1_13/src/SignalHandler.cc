// SignalHandler.cc for Fluxbox Window Manager
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: SignalHandler.cc,v 1.3 2002/10/12 13:28:03 fluxgen Exp $

#include "SignalHandler.hh"

namespace FbTk {

EventHandler<SignalEvent> *SignalHandler::s_signal_handler[NSIG];

SignalHandler::SignalHandler() {
	// clear signal list
	for (int i=0; i < NSIG; ++i)
		s_signal_handler[i] = 0;
}

SignalHandler *SignalHandler::instance() {
	static SignalHandler singleton;
	return &singleton;
}

bool SignalHandler::registerHandler(int signum, EventHandler<SignalEvent> *eh, 
	EventHandler<SignalEvent> **oldhandler_ret) {
	// must be less than NSIG
	if (signum >= NSIG)
		return false;

	// get old signal handler for this signum
	if (oldhandler_ret != 0)
		*oldhandler_ret = s_signal_handler[signum];
	
	struct sigaction sa;
	// set callback
	sa.sa_handler = SignalHandler::handleSignal;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	
	if (sigaction(signum, &sa, 0) == -1)
		return false;
	
	s_signal_handler[signum] = eh;
	
	return true;
}

void SignalHandler::removeHandler(int signum) {
	if (signum < NSIG)
		s_signal_handler[signum] = 0; // clear handler pointer
}

void SignalHandler::handleSignal(int signum) {
	if (signum >= NSIG)
		return;
	// make sure we got a handler for this signal
	if (s_signal_handler[signum] != 0) {
		SignalEvent sigev;
		sigev.signum = signum;
		s_signal_handler[signum]->handleEvent(&sigev);
	}
}

}; 
