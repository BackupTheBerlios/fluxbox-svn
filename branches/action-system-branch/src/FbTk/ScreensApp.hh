// ScreensApp.hh for fluxbox 
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: ScreensApp.hh,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#ifndef FBTK_SCREENSAPP_HH
#define FBTK_SCREENSAPP_HH

#include "App.hh"

namespace FbTk {


/**
 * This class is intended to be a general representation for 
 * screens that can be used for FbTk to be aware of multiple
 * screens. 
 * It may be extended to handle more screen data when necessary.
 */
class ScreensApp : public App {
public:
    /// @return singleton instance of App
    static ScreensApp *instance();

    ScreensApp(const char *displayname=0);
    virtual ~ScreensApp();

    /**
     * Does this app use the given screen?
     */
    bool isScreenUsed(int screen) const;

    void setScreenUsed(int screen, bool value = true);

    // same as countscreens, only no call to display etc
    // this is NOT the number of used screens
    inline int numScreens() const { return m_numscreens; }

private:
    static ScreensApp *s_screensapp;

    int m_numscreens;

    // bool array saying whether given screen is used by this app
    bool *m_active_screens; 

};


}; // end namespace FbTk


#endif // FBTK_SCREENSAPP_HH
