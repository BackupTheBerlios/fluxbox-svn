// WinClient.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id: WinClient.hh,v 1.2 2003/04/14 12:08:21 fluxgen Exp $

#ifndef WINCLIENT_HH
#define WINCLIENT_HH

#include "BaseDisplay.hh"
#include "Subject.hh"
#include "FbWindow.hh"

#include <X11/Xutil.h>
#include <string>

class FluxboxWindow;

/// Holds client window info 
class WinClient:public FbTk::FbWindow {
public:
    typedef std::list<FluxboxWindow *> TransientList;

    WinClient(Window win, FluxboxWindow &fbwin);

    ~WinClient();
    void updateRect(int x, int y, unsigned int width, unsigned int height);
    void sendFocus();
    void sendClose();
    void reparent(Window win, int x, int y);
    bool getAttrib(XWindowAttributes &attr) const;
    bool getWMName(XTextProperty &textprop) const;
    bool getWMIconName(XTextProperty &textprop) const;
    void updateTitle();
    void updateIconTitle();

    /// notifies when this client dies
    FbTk::Subject &dieSig() { return m_diesig; }

    /// updates transient window information
    void updateTransientInfo();
    FluxboxWindow *transientFor() { return transient_for; }
    const FluxboxWindow *transientFor() const { return transient_for; }
    TransientList &transientList() { return transients; }
    const TransientList &transientList() const { return transients; }
    bool operator == (const FluxboxWindow &win) const {
        return (m_win == &win);
    }

    const std::string &title() const { return m_title; }
    const std::string &iconTitle() const { return m_icon_title; }
    const FluxboxWindow *fbwindow() const { return m_win; }
    FluxboxWindow *fbwindow() { return m_win; }
    /**
       !! TODO !!
       remove or move these to private
     */

    FluxboxWindow *transient_for; // which window are we a transient for?
    std::list<FluxboxWindow *> transients;  // which windows are our transients?
    Window window_group;

 
    int x, y, old_bw;
    unsigned int
        min_width, min_height, max_width, max_height, width_inc, height_inc,
        min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
        base_width, base_height, win_gravity;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;

    // this structure only contains 3 elements... the Motif 2.0 structure contains
    // 5... we only need the first 3... so that is all we will define
    typedef struct MwmHints {
        unsigned long flags;       // Motif wm flags
        unsigned long functions;   // Motif wm functions
        unsigned long decorations; // Motif wm decorations
    } MwmHints;

    MwmHints *mwm_hint;
    BaseDisplay::BlackboxHints *blackbox_hint;
    FluxboxWindow *m_win;
    class WinClientSubj: public FbTk::Subject {
    public:
        explicit WinClientSubj(WinClient &client):m_winclient(client) { }
        WinClient &winClient() { return m_winclient; }
    private:
        WinClient &m_winclient;
    };

private:
    std::string m_title, m_icon_title;
    WinClientSubj m_diesig;
};

#endif // WINCLIENT_HH
