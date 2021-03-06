// FbRun.hh
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: FbRun.cc,v 1.2 2002/09/03 10:55:02 fluxgen Exp $

#include "FbRun.hh"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <unistd.h>

#include <iostream>

using namespace std;
FbRun::FbRun(Display *disp, int x, int y, size_t width):
m_font(disp, "fixed"),
m_win(None),
m_display(disp),
m_bevel(4),
m_gc(None),
m_end(false) {

	createWindow(x, y, width + m_bevel, m_font.height());
	// create GC
	XGCValues gcv;

	if (m_font.fontStruct())
		gcv.font = 	m_font.fontStruct()->fid;

	m_gc = XCreateGC(m_display, m_win, GCFont, &gcv);
}

FbRun::~FbRun() {
	hide();
	XDestroyWindow(m_display, m_win);
}

void FbRun::run(const std::string &command) {
	//fork and execute program
	if (!fork()) {
		setsid();
		execl("/bin/sh", "/bin/sh", "-c", command.c_str(), 0);
		exit(0); //exit fork
	}

	hide();

	m_end = true; // mark end of processing
}

bool FbRun::loadFont(const string &fontname) {
	if (!m_font.load(fontname.c_str()))
		return false;

	// reconfigure m_gc for the new font
	XGCValues gcv;
	if (m_font.fontStruct())
		gcv.font = m_font.fontStruct()->fid;
	XChangeGC(m_display, m_gc, GCFont, &gcv);

	// resize to fit new font height
	resize(m_width, m_font.height() + m_bevel);
	return true;
}

void FbRun::setForeground(const XColor &color) {
	XSetForeground(m_display, m_gc, color.pixel);
	redrawLabel();
}

void FbRun::setBackground(const XColor &color) {
	XSetWindowBackground(m_display, m_win, color.pixel);
	redrawLabel();
}


void FbRun::setText(const string &text) {
	m_runtext = text;
	redrawLabel();
}

void FbRun::setTitle(const string &title) {
	assert(m_win);
	XStoreName(m_display, m_win, const_cast<char *>(title.c_str()));	
}

void FbRun::move(int x, int y) {
	XMoveWindow(m_display, m_win, x, y);
}

void FbRun::resize(size_t width, size_t height) {
	assert(m_win);
	XResizeWindow(m_display, m_win, width, height);
	m_width = width;
	m_height = height + m_bevel;
	setNoMaximize();
}

void FbRun::show() {
	assert(m_win);
	XMapWindow(m_display, m_win);
}

void FbRun::hide() {
	assert(m_win);
	XUnmapWindow(m_display, m_win);
}

void FbRun::redrawLabel() {
	assert(m_win);

	XClearWindow(m_display, m_win);
	drawString(m_bevel/2, m_height - m_bevel/2,
		m_runtext.c_str(), m_runtext.size());

}

void FbRun::drawString(int x, int y,
	const char *text, size_t len) {
	assert(m_win);
	assert(m_gc);

	if (FbTk::Font::multibyte()) {
		XmbDrawString(m_display, m_win, m_font.fontSet(),
			m_gc, x, y,
			text, len);
	} else {
		XDrawString(m_display, m_win,
			m_gc, x, y,
			text, len);
	}
}


void FbRun::createWindow(int x, int y, size_t width, size_t height) {
	m_win = XCreateSimpleWindow(m_display, // display
		DefaultRootWindow(m_display), // parent windows
		x, y,
		width, height,
		1,  // border_width
		0,  // border
		WhitePixel(m_display, DefaultScreen(m_display))); // background

	if (m_win == None)
		throw string("Failed to create FbRun window!");

	XSelectInput(m_display, m_win, KeyPressMask|ExposureMask);

	setNoMaximize();

	m_width = width;
	m_height = height;

}

void FbRun::handleEvent(XEvent * const xev) {
	switch (xev->type) {
		case KeyPress: {
			KeySym ks;
			char keychar[1];
			XLookupString(&xev->xkey, keychar, 1, &ks, 0);
			if (ks == XK_Escape) {
				m_end = true;
				hide();
				return; // no more processing
			} else if (ks == XK_Return) {
				run(m_runtext);
				m_runtext = ""; // clear text
			} else if (ks == XK_BackSpace && m_runtext.size() != 0) {
				m_runtext.erase(m_runtext.size()-1);
				redrawLabel();
			} else if (! IsModifierKey(ks) && !IsCursorKey(ks)) {
				m_runtext+=keychar[0]; // append character
				redrawLabel(); 
			}
		} break;
		case Expose:
			redrawLabel();
		break;
		default:
		break;
	}
}

void FbRun::getSize(size_t &width, size_t &height) {
	XWindowAttributes attr;
	XGetWindowAttributes(m_display, m_win, &attr);
	width = attr.width;
	height = attr.height;
}

void FbRun::setNoMaximize() {

	size_t width, height;

	getSize(width, height);

	// we don't need to maximize this window
	XSizeHints sh;
	sh.flags = PMaxSize | PMinSize;
	sh.max_width = width;
	sh.max_height = height;
	sh.min_width = width;
	sh.min_height = height;
	XSetWMNormalHints(m_display, m_win, &sh);
}
