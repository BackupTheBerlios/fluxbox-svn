// Keys.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

//$Id: Keys.cc,v 1.38.2.1 2003/10/28 21:34:52 rathnor Exp $


#include "Keys.hh"

#include "FbTk/StringUtil.hh"
#include "FbTk/ScreensApp.hh"
#include "FbTk/Command.hh"
#include "FbTk/KeyUtil.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/Action.hh"
#include "FbTk/ActionNode.hh"

#include "CommandParser.hh"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif	// HAVE_CTYPE_H

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif	// HAVE_SYS_TYPES_H

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif	// HAVE_SYS_WAIT_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif	// HAVE_UNISTD_H

#ifdef	HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif	// HAVE_SYS_STAT_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <memory>

using namespace std;

Keys::Keys(const char *filename)
{

    if (filename != 0)
        reconfigure(filename, false);
}

Keys::~Keys() {	
    // don't unregister handler, since it is against the root window
}

/** 
    Load and grab keys
    TODO: error checking
    @return true on success else false
*/
bool Keys::reconfigure(const char *filename, bool clear_old) {
    if (!filename)
        return false;

    FbTk::ScreensApp *sapp = FbTk::ScreensApp::instance();
    Display *display = sapp->display();

    // grab any appropriate screens
    for (int i=0; i < sapp->numScreens(); ++i) {
        if (sapp->isScreenUsed(i))
            FbTk::EventManager::instance()->add(m_actionhandler, RootWindow(display, i));
        else
            FbTk::EventManager::instance()->remove(RootWindow(display, i));
    }

    //ungrab all keys
    if (clear_old)
        FbTk::KeyUtil::ungrabKeys();

    XSync(FbTk::App::instance()->display(), False);

    //open the file
    ifstream infile(filename);
    if (!infile)
        return false; // failed to open file

    int line=0;//current line, so we can tell the user where the fault is

    while (!infile.eof()) {
        string linebuffer;

        getline(infile, linebuffer);
        line++;

        // blank or comment
        if (linebuffer.empty() || linebuffer[0] == '#')
            continue;

        size_t colon = linebuffer.find_first_of(':');

        string keyspec = linebuffer.substr(0, colon);
        string cmdspec = linebuffer.substr(colon+1);

        FbTk::ActionNode *node = new FbTk::ActionNode();
        FbTk::Action *action = 0;
        if (!node->setFromString(keyspec)) {
            delete node; // failed parse
            cerr<<"File: "<<filename<<". Error in key spec on line "<<line<<": "<<keyspec<<endl;
            cerr<<"> "<<linebuffer<<endl;
        } else {
            action = CommandParser::instance().parseAction(cmdspec, node);
            if (!action) {
                cerr<<"File: "<<filename<<". Error in action specification on line "<<line<<": "<<cmdspec<<endl;
                cerr<<"> "<<linebuffer<<endl;
                delete node;
            } else {
                node->setAction(action); 
                m_actionhandler.registerAction(*node);

                // don't let this node delete the action, since it has been 
                // copied into the action handler, so we temporarily
                // mark the action as global
                bool global = action->isGlobal();
                if (!global)
                    action->setGlobal(true);
                delete node;
                if (!global)
                    action->setGlobal(false);
            }
        }
    } // end while eof
    
    return true;
}
