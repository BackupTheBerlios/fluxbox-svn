// AtomHandler for fluxbox
// Copyright (c) 2002-2004 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//               2003-2004 Simon Bowden    (rathnor at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: AtomHandler.hh,v 1.14 2004/01/19 18:22:37 fluxgen Exp $

#ifndef ATOMHANDLER_HH
#define ATOMHANDLER_HH

#include <X11/Xlib.h>

class FluxboxWindow;
class WinClient;
class BScreen;

class AtomHandler {
public:

    virtual ~AtomHandler() { }

    virtual void initForScreen(BScreen &screen) = 0;
    virtual void setupFrame(FluxboxWindow &win) = 0;
    virtual void setupClient(WinClient &winclient) = 0;

    virtual void updateFocusedWindow(BScreen &screen, Window win) = 0; 
    virtual void updateClientList(BScreen &screen) = 0;
    virtual void updateWorkspaceNames(BScreen &screen) = 0;
    virtual void updateCurrentWorkspace(BScreen &screen) = 0;
    virtual void updateWorkspaceCount(BScreen &screen) = 0;
    virtual void updateWorkarea(BScreen &screen) = 0;

    virtual void updateFrameClose(FluxboxWindow &win) = 0;
    virtual void updateClientClose(WinClient &winclient) = 0;
    virtual void updateWorkspace(FluxboxWindow &win) = 0;
    virtual void updateState(FluxboxWindow &win) = 0;
    virtual void updateHints(FluxboxWindow &win) = 0;
    virtual void updateLayer(FluxboxWindow &win) = 0;

    virtual bool checkClientMessage(const XClientMessageEvent &ce, 
                                    BScreen * screen, WinClient * const winclient) = 0;

    virtual bool propertyNotify(WinClient &winclient, Atom the_property) = 0;

    /// should this object be updated or not?
    bool update() const { return m_update; }
protected:
    AtomHandler():m_update(true) { }

    void disableUpdate() { m_update = false; }
    void enableUpdate() { m_update = true; }	
private:
    bool m_update; ///< do we get update or not
};

#endif // ATOMHANDLER_HH
