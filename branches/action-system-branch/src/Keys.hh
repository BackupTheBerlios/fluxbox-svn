// Keys.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Keys.hh,v 1.29.2.2 2004/01/28 11:02:58 rathnor Exp $

#ifndef KEYS_HH
#define KEYS_HH

#include <string>
#include <vector>
#include <X11/Xlib.h>

#include "FbTk/ActionHandler.hh"
#include "NotCopyable.hh"



/**
 * This is a little wrapper class to combine together
 * the action handler with their specification by 
 * reading a keys file.
 */
class Keys:private FbTk::NotCopyable  {
public:

    /**
       Constructor
       @param filename file to load, default none
    */
    explicit Keys(const char *filename = 0);
    /// destructor
    ~Keys();

    /**
       Reload configuration from filename
       @return true on success, else false
    */
    bool reconfigure(const char *filename, bool clear_old = true);

    inline FbTk::ActionHandler &handler() { return m_actionhandler; }
    inline const FbTk::ActionHandler &handler() const { return m_actionhandler; }

    // for ActionHandler
    // These are the window levels used by fluxbox (so far)
    // they are a mask
    enum { INVALID=0, GLOBAL=1, TOPLEVEL=2, TOPNOTCLIENT=4, TAB=8 } WindowLevel;

private:

    std::string filename;	
    
    FbTk::ActionHandler m_actionhandler;
};

#endif // KEYS_HH
