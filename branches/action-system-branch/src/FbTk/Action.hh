// Action.hh for fluxbox 
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

// $Id: Action.hh,v 1.1.2.2 2004/01/28 11:03:34 rathnor Exp $


#ifndef FBTK_ACTION_HH
#define FBTK_ACTION_HH

#include <X11/Xlib.h>

namespace FbTk {

class ActionContext;

/**
 * Class Action is where all the action is.
 * I envisage it as replacing the current Commands 
 * (or simply being called Command :-)  )
 */

class Action {
public:
    Action(bool start, bool stop, bool motion, bool isglobal = false, bool ispassthrough = false);
    virtual ~Action() {}

    // Called on a "Press" event
    // the arguments give the location of the action,
    // from window, the co-ordinates in the window
    // and the coordinates in the root window.
    // the coordinates are undefined for "focused window" events
    // returns a bool to indicate success
    // an unsuccessful start will not grab/motion
    virtual bool start(ActionContext &context) { return true; }

    // Called on a "Release" event
    virtual bool stop(ActionContext &context) { return true; }

    // Called on a "motion" event
    virtual bool motion(ActionContext &context) { return true; }

    // we store explicitly which actions this has so we don't
    // construct and call the functions unless absolutely necessary
    inline bool hasStartAction() const  { return has_start;  }
    inline bool hasStopAction() const   { return has_stop;   }

    // Motion actions can't have regular keys as part of their
    // description (since they don't "hold down", they repeat).
    inline bool hasMotionAction() const { return has_motion; }

    // Actions can be marked as global (i.e. static) so that they
    // aren't deleted by the action system - it is up to the 
    // application to remove them when they are finished with.
    inline bool isGlobal() const { return global; }
    inline void setGlobal(bool isglobal) { global = isglobal; }

    // Actions can be marked ass passthrough. That means that the
    // user action(s) that trigger it are replayed to the client
    inline bool isPassthrough() const { return passthrough; }
    inline void setPassthrough(bool ispassthrough) { passthrough = ispassthrough; }

    // mouse motion actions can have a specific cursor
    // defaults to none == window's normal cursor
    virtual Cursor cursor();

    /* "Level" that this action performs at.
     * An application can define it's levels however it desires
     * It will be asked for the level of a given window, and 
     * the action of that level (if it exists) will be called.
     *
     * Note that keyboard actions come from grabs on the root window
     * and will thus be the same level as is returned for that.
     * default to 0 just to save forcing this to be implemented
     * thus I recommend that "root" windows return level 0, ditto "global" actions.
     *
     * Also note that negative levels are considered invalid (thus
     * the return value for "this window has no known level").
     * See the getLevel function in ActionHandler.
     *
     * There is no need for levels to be consecutive/densely populated.
     */
    virtual int getLevel() { return 0; }

private:
    bool has_start, has_motion, has_stop, global, passthrough;

};

}; // end namespace FbTk


#endif // FBTK_ACTION_HH
