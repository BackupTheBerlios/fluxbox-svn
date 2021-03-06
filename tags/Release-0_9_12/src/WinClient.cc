// WinClient.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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

// $Id$

#include "WinClient.hh"

#include "Window.hh"
#include "fluxbox.hh"
#include "Screen.hh"
#include "FbAtoms.hh"
#include "EventManager.hh"
#include "Xutil.hh"

#include "FbTk/I18n.hh"

#include <iostream>
#include <algorithm>
#include <iterator>
#ifdef HAVE_CASSERT
  #include <cassert>
#else
  #include <assert.h>
#endif

using namespace std;

WinClient::WinClient(Window win, BScreen &screen, FluxboxWindow *fbwin):FbTk::FbWindow(win),
                     transient_for(0),
                     window_group(0),
                     x(0), y(0), old_bw(0),
                     min_width(1), min_height(1),
                     max_width(0), max_height(0),
                     width_inc(1), height_inc(1),
                     min_aspect_x(0), min_aspect_y(0),
                     max_aspect_x(0), max_aspect_y(0),
                     base_width(1), base_height(1),
                     initial_state(0),
                     normal_hint_flags(0),
                     wm_hint_flags(0),
                     m_win(fbwin),
                     m_modal(0),
                     send_focus_message(false),
                     send_close_message(false),
                     m_win_gravity(0),
                     m_title(""), m_icon_title(""),
                     m_class_name(""), m_instance_name(""),
                     m_blackbox_hint(0),
                     m_mwm_hint(0),
                     m_focus_mode(F_PASSIVE),
                     m_diesig(*this), m_screen(screen), 
                     m_strut(0) {
    updateWMProtocols();
    updateBlackboxHints();
    updateMWMHints();
    updateWMHints();
    updateWMNormalHints();
    updateWMClassHint();
    updateTitle();
    updateIconTitle();
    Fluxbox::instance()->saveWindowSearch(win, this);
    if (window_group != None) 
        Fluxbox::instance()->saveGroupSearch(window_group, this);

}

WinClient::~WinClient() {
#ifdef DEBUG
    cerr<<__FILE__<<"(~"<<__FUNCTION__<<")[this="<<this<<"]"<<endl;
#endif // DEBUG

    FbTk::EventManager::instance()->remove(window());

    clearStrut();

    if (m_win != 0)
        m_win->removeClient(*this);

    // this takes care of any focus issues
    m_diesig.notify();

    Fluxbox *fluxbox = Fluxbox::instance();

    if (transient_for != 0) {
        assert(transient_for != this);
        transient_for->transientList().remove(this);
        transient_for = 0;
    }

    while (!transients.empty()) {
        transients.back()->transient_for = 0;
        transients.pop_back();
    }

    screen().removeNetizen(window());

    if (window_group != 0) {
        fluxbox->removeGroupSearch(window_group);
        window_group = 0;
    }

    if (m_mwm_hint != 0)
        XFree(m_mwm_hint);

    if (m_blackbox_hint != 0)
        XFree(m_blackbox_hint);

    if (window())
        fluxbox->removeWindowSearch(window());

    m_win = 0;
}

void WinClient::updateRect(int x, int y, 
                        unsigned int width, unsigned int height) {
    Display *disp = FbTk::App::instance()->display();
    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = disp;
    event.xconfigure.event = window();
    event.xconfigure.window = window();
    event.xconfigure.x = x;
    event.xconfigure.y = y;
    event.xconfigure.width = width;
    event.xconfigure.height = height;
    //!! TODO
    event.xconfigure.border_width = 1;//client.old_bw;
    //!! TODO
    event.xconfigure.above = None; //m_frame.window().window();
    event.xconfigure.override_redirect = false;

    XSendEvent(disp, window(), False, StructureNotifyMask, &event);

}

bool WinClient::acceptsFocus() const {
    return (m_focus_mode == F_LOCALLYACTIVE || 
            m_focus_mode == F_PASSIVE || 
            m_focus_mode == F_GLOBALLYACTIVE && send_focus_message);
}

bool WinClient::sendFocus() {
    if (!send_focus_message)
        return false;
#ifdef DEBUG
    cerr<<"WinClient::"<<__FUNCTION__<<": this = "<<this<<
        " window = 0x"<<hex<<window()<<dec<<endl;
#endif // DEBUG
    Display *disp = FbTk::App::instance()->display();
    // setup focus msg
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
    ce.xclient.display = disp;
    ce.xclient.window = window();
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = FbAtoms::instance()->getWMTakeFocusAtom();
    ce.xclient.data.l[1] = Fluxbox::instance()->getLastTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    // send focus msg
    XSendEvent(disp, window(), false, NoEventMask, &ce);
    return true;
}

void WinClient::sendClose(bool forceful) {
    if (forceful || !send_close_message)
        XKillClient(FbTk::App::instance()->display(), window());
    else {
        // send WM_DELETE message
        Display *disp = FbTk::App::instance()->display();
        // fill in XClientMessage structure for delete message
        XEvent ce;
        ce.xclient.type = ClientMessage;
        ce.xclient.message_type = FbAtoms::instance()->getWMProtocolsAtom();
        ce.xclient.display = disp;
        ce.xclient.window = window();
        ce.xclient.format = 32;
        ce.xclient.data.l[0] = FbAtoms::instance()->getWMDeleteAtom();
        ce.xclient.data.l[1] = CurrentTime;
        ce.xclient.data.l[2] = 0l;
        ce.xclient.data.l[3] = 0l;
        ce.xclient.data.l[4] = 0l;
        // send event delete message to client window
        XSendEvent(disp, window(), false, NoEventMask, &ce);
    }
}

bool WinClient::getAttrib(XWindowAttributes &attr) const {
    return XGetWindowAttributes(FbTk::App::instance()->display(), window(), &attr);
}

bool WinClient::getWMName(XTextProperty &textprop) const {
    return XGetWMName(FbTk::App::instance()->display(), window(), &textprop);
}

bool WinClient::getWMIconName(XTextProperty &textprop) const {
    return XGetWMName(FbTk::App::instance()->display(), window(), &textprop);
}

const std::string &WinClient::getWMClassName() const {
    return m_instance_name;
}

const std::string &WinClient::getWMClassClass() const {
    return m_class_name;
}

void WinClient::updateWMClassHint() {
    XClassHint ch;
    if (XGetClassHint(FbTk::App::instance()->display(), window(), &ch) == 0) {
#ifdef DEBUG
        cerr<<"WinClient: Failed to read class hint!"<<endl;
#endif //DEBUG
    } else {        

        if (ch.res_name != 0) {
            m_instance_name = const_cast<char *>(ch.res_name);
            XFree(ch.res_name);
            ch.res_name = 0;
        } else 
            m_instance_name = "";
        
        if (ch.res_class != 0) {
            m_class_name = const_cast<char *>(ch.res_class);
            XFree(ch.res_class);
            ch.res_class = 0;
        } else
            m_class_name = "";
    }
}

void WinClient::updateTransientInfo() {
#ifdef DEBUG
    cerr<<__FUNCTION__<<": m_win = "<<m_win<<endl;
#endif // DEBUG
    if (m_win == 0)
        return;

    // remove us from parent
    if (transientFor() != 0) {
        transientFor()->transientList().remove(this);
    }
    
    transient_for = 0;
    Display *disp = FbTk::App::instance()->display();
    // determine if this is a transient window
    Window win = 0;
    if (!XGetTransientForHint(disp, window(), &win)) {
#ifdef DEBUG
        cerr<<__FUNCTION__<<": window() = 0x"<<hex<<window()<<dec<<"Failed to read transient for hint."<<endl;
#endif // DEBUG
        return;
    }

    // we can't be transient to ourself
    if (win == window()) {
#ifdef DEBUG
        cerr<<__FUNCTION__<<": transient to ourself"<<endl;
#endif // DEBUG
        return;
    }
	
    if (win != None && m_win->screen().rootWindow() == win) {
        // transient for root window... =  transient for group
        // I don't think we are group-aware yet
        return; 
    }


    transient_for = Fluxbox::instance()->searchWindow(win);

#ifdef DEBUG
    cerr<<__FUNCTION__<<": transient_for window = 0x"<<hex<<win<<dec<<endl;
    cerr<<__FUNCTION__<<": transient_for = "<<transient_for<<endl;
#endif // DEBUG
    // make sure we don't have deadlock loop in transient chain
    for (WinClient *w = this; w != 0; w = w->transient_for) {
        if (w == w->transient_for) {
            w->transient_for = 0;
            break;
        }
    }

    if (transientFor() != 0) {
        // we need to add ourself to the right client in
        // the transientFor() window so we search client
        transient_for->transientList().push_back(this);

        if (transientFor()->fbwindow() && transientFor()->fbwindow()->isStuck())
            m_win->stick();
    }
}


void WinClient::updateTitle() {
    // why 512? very very long wmnames seem to either 
    // crash fluxbox or to make it have high cpuload
    // see also: 
    //    http://www.securityfocus.com/archive/1/382398/2004-11-24/2004-11-30/2
    // 
    // TODO: - find out why this mostly happens when using xft-fonts
    //       - why other windowmanagers (pekwm/pwm3/openbox etc) are
    //         also influenced
    //
    // the limitation to 512 chars only avoids running in that trap
    m_title = string(Xutil::getWMName(window()) ,0 , 512);
}

void WinClient::updateIconTitle() {
    XTextProperty text_prop;
    char **list = 0;
    int num = 0;

    if (getWMIconName(text_prop)) {
        if (text_prop.value && text_prop.nitems > 0) {
            if (text_prop.encoding != XA_STRING) {
                text_prop.nitems = strlen((char *) text_prop.value);

                if (XmbTextPropertyToTextList(FbTk::App::instance()->display(), &text_prop,
                                               &list, &num) == Success &&
                    num > 0 && *list) {
                    m_icon_title = (char *)*list;
                    XFreeStringList(list);
                } else
                    m_icon_title = text_prop.value ? (char *)text_prop.value : "";
            } else
                m_icon_title = text_prop.value ? (char *)text_prop.value : "";

            if (text_prop.value)
                XFree((char *) text_prop.value);
        } else
            m_icon_title = title();
    } else
        m_icon_title = title();

}

void WinClient::saveBlackboxAttribs(FluxboxWindow::BlackboxAttributes &blackbox_attribs) {
    changeProperty(FbAtoms::instance()->getFluxboxAttributesAtom(),
                   PropModeReplace, XA_CARDINAL, 32,
                   (unsigned char *)&blackbox_attribs,
                   FluxboxWindow::PropBlackboxAttributesElements
        );
}

void WinClient::updateBlackboxHints() {
    int format;
    Atom atom_return;
    unsigned long num, len;
    FbAtoms *atoms = FbAtoms::instance();

    if (m_blackbox_hint) {
        XFree(m_blackbox_hint);
        m_blackbox_hint = 0;
    }

    if (property(atoms->getFluxboxHintsAtom(), 0,
                 PropBlackboxHintsElements, False,
                 atoms->getFluxboxHintsAtom(), &atom_return,
                 &format, &num, &len,
                 (unsigned char **) &m_blackbox_hint) &&
        m_blackbox_hint) {

        if (num != (unsigned)PropBlackboxHintsElements) {
            XFree(m_blackbox_hint);
            m_blackbox_hint = 0;
        }
    }
}

void WinClient::updateMWMHints() {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;

    if (m_mwm_hint) {
        XFree(m_mwm_hint);
        m_mwm_hint = 0;
    }
    Atom motif_wm_hints = FbAtoms::instance()->getMWMHintsAtom();

    if (!(property(motif_wm_hints, 0,
                   PropMwmHintsElements, false,
                   motif_wm_hints, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &m_mwm_hint) &&
          m_mwm_hint)) {
        if (num != static_cast<unsigned int>(PropMwmHintsElements)) {
            XFree(m_mwm_hint);
            m_mwm_hint = 0;
            return;
        }
    }
}

void WinClient::updateWMHints() {
    XWMHints *wmhint = XGetWMHints(FbTk::App::instance()->display(), window());
    if (! wmhint) {
        m_focus_mode = F_PASSIVE;
        window_group = None;
        initial_state = NormalState;
    } else {
        wm_hint_flags = wmhint->flags;
        /*
         * ICCCM 4.1.7
         *---------------------------------------------
         * Input Model      Input Field   WM_TAKE_FOCUS
         *---------------------------------------------
         * No Input          False         Absent
         * Passive           True          Absent
         * Locally Active    True          Present
         * Globally Active   False         Present
         *---------------------------------------------
         * Here: WM_TAKE_FOCUS = send_focus_message
         *       Input Field   = wmhint->input
         *       Input Model   = m_focus_mode
         */
        if (wmhint->flags & InputHint) {
            if (wmhint->input) {
                if (send_focus_message)
                    m_focus_mode = F_LOCALLYACTIVE;
                else
                    m_focus_mode = F_PASSIVE;
            } else {
                if (send_focus_message)
                    m_focus_mode = F_GLOBALLYACTIVE;
                else
                    m_focus_mode = F_NOINPUT;
            }
        } else // InputHint not present: ignoring send_focus_message and assuming F_PASSIVE
            m_focus_mode = F_PASSIVE; 

        if (wmhint->flags & StateHint)
            initial_state = wmhint->initial_state;
        else
            initial_state = NormalState;

        if (wmhint->flags & WindowGroupHint) {
            if (! window_group)
                window_group = wmhint->window_group;
        } else
            window_group = None;

        XFree(wmhint);
    }
}


void WinClient::updateWMNormalHints() {
    long icccm_mask;
    XSizeHints sizehint;
    if (! XGetWMNormalHints(FbTk::App::instance()->display(), window(), &sizehint, &icccm_mask)) {
        min_width = min_height =
            base_width = base_height =
            width_inc = height_inc = 1;
        max_width = 0; // unbounded
        max_height = 0;
        min_aspect_x = min_aspect_y =
            max_aspect_x = max_aspect_y = 0;
        m_win_gravity = NorthWestGravity;
    } else {
        normal_hint_flags = sizehint.flags;

        if (sizehint.flags & PMinSize) {
            min_width = sizehint.min_width;
            min_height = sizehint.min_height;
            if (!(sizehint.flags & PBaseSize)) {
                base_width = min_width;
                base_height = min_height;
            }
        } else {
            min_width = min_height = 1;
            base_width = base_height = 0;
        }

        if (sizehint.flags & PBaseSize) {
            base_width = sizehint.base_width;
            base_height = sizehint.base_height;
            if (!(sizehint.flags & PMinSize)) {
                min_width = base_width;
                min_height = base_height;
            }
        } // default set in PMinSize

        if (sizehint.flags & PMaxSize) {
            max_width = sizehint.max_width;
            max_height = sizehint.max_height;
        } else {
            max_width = 0; // unbounded
            max_height = 0;
        }

        if (sizehint.flags & PResizeInc) {
            width_inc = sizehint.width_inc;
            height_inc = sizehint.height_inc;
        } else
            width_inc = height_inc = 1;

        if (sizehint.flags & PAspect) {
            min_aspect_x = sizehint.min_aspect.x;
            min_aspect_y = sizehint.min_aspect.y;
            max_aspect_x = sizehint.max_aspect.x;
            max_aspect_y = sizehint.max_aspect.y;
        } else
            min_aspect_x = min_aspect_y =
                max_aspect_x = max_aspect_y = 0;

        if (sizehint.flags & PWinGravity)
            m_win_gravity = sizehint.win_gravity;
        else
            m_win_gravity = NorthWestGravity;

    }
}

Window WinClient::getGroupLeftWindow() const {
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);

    Window *data = 0;
    if (property(group_left_hint, 0,
                   1, false,
                   XA_WINDOW, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &data) &&
        data) {
        if (num != 1) {
            XFree(data);
            return None;
        } else {
            Window ret = *data;
            XFree(data);
            return ret;
        }
    }
    return None;
}


void WinClient::setGroupLeftWindow(Window win) {
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);
    changeProperty(group_left_hint, XA_WINDOW, 32, 
                   PropModeReplace, (unsigned char *) &win, 1);
}

bool WinClient::hasGroupLeftWindow() const {
    // try to find _FLUXBOX_GROUP_LEFT atom in window
    // if we have one then we have a group left window
    int format;
    Atom atom_return;
    unsigned long num = 0, len = 0;
    Atom group_left_hint = XInternAtom(FbTk::App::instance()->display(), "_FLUXBOX_GROUP_LEFT", False);

    Window *data = 0;
    if (property(group_left_hint, 0,
                   1, false,
                   XA_WINDOW, &atom_return,
                   &format, &num, &len,
                   (unsigned char **) &data) &&
        data) {
            XFree(data);
            if (num != 1)
                return false;
            else
                return true;
    }

    return false;
}

void WinClient::addModal() {
    ++m_modal;
    if (transient_for)
        transient_for->addModal();
}

void WinClient::removeModal() {
    --m_modal;
    if (transient_for)
        transient_for->removeModal();
}

bool WinClient::validateClient() const {
    Display *display = FbTk::App::instance()->display();
    FbTk::App::instance()->sync(false);

    XEvent e;
    if (( XCheckTypedWindowEvent(display, window(), DestroyNotify, &e) ||
          XCheckTypedWindowEvent(display, window(), UnmapNotify, &e)) 
        && XPutBackEvent(display, &e)) {
        Fluxbox::instance()->ungrab();
        return false;
    }

    return true;
}

void WinClient::setStrut(Strut *strut) {    
    clearStrut();
    m_strut = strut;
}

void WinClient::clearStrut() {
    if (m_strut != 0) {
        screen().clearStrut(m_strut);
        m_strut = 0;
    }
}

bool WinClient::focus() {
    if (m_win == 0)
        return false;
    else
        return m_win->setCurrentClient(*this, true);
}

void WinClient::updateWMProtocols() {
    Atom *proto = 0;
    int num_return = 0;
    FbAtoms *fbatoms = FbAtoms::instance();

    if (XGetWMProtocols(FbTk::App::instance()->display(), window(), &proto, &num_return)) {

        // defaults
        send_focus_message = false;
        send_close_message = false;
        // could be added to netizens twice...
        for (int i = 0; i < num_return; ++i) {
            if (proto[i] == fbatoms->getWMDeleteAtom())
                send_close_message = true;
            else if (proto[i] == fbatoms->getWMTakeFocusAtom())
                send_focus_message = true;
            else if (proto[i] == fbatoms->getFluxboxStructureMessagesAtom())
                screen().addNetizen(window());
        }

        XFree(proto);
        if (m_win)
            m_win->updateFunctions();
#ifdef DEBUG
    } else {
        cerr<<"Warning: Failed to read WM Protocols. "<<endl;
#endif // DEBUG
    }

}

/* For aspect ratios
   Note that its slightly simplified in that only the
   line gradient is given - this is because for aspect
   ratios, we always have the line going through the origin
   
   * Based on this formula:
   http://astronomy.swin.edu.au/~pbourke/geometry/pointline/

   Note that a gradient from origin goes through ( grad , 1 )
 */

void closestPointToLine(double &ret_x, double &ret_y, 
                        double point_x, double point_y,
                        double gradient) {
    double u = (point_x * gradient + point_y) /
        (gradient*gradient + 1);

    ret_x = u*gradient;
    ret_y = u;
}

/**
 * Changes width and height to the nearest (lower) value
 * that conforms to it's size hints.
 *
 * display_* give the values that would be displayed
 * to the user when resizing.
 * We use pointers for display_* since they are optional.
 *
 * See ICCCM section 4.1.2.3
 */
void WinClient::applySizeHints(int &width, int &height, 
                               int *display_width, int *display_height) {

    int i = width, j = height;

    // Check minimum size
    if (width < 0 || width < static_cast<signed>(min_width)) 
        width = min_width;

    if (height < 0 || height < static_cast<signed>(min_height))
        height = min_height;

    // Check maximum size
    if (max_width > 0 && width > static_cast<signed>(max_width))
        width = max_width;

    if (max_height > 0 && height > static_cast<signed>(max_height))
        height = max_height;

    // we apply aspect ratios before incrementals
    // Too difficult to exactly satisfy both incremental+aspect 
    // in most situations 
    // (they really shouldn't happen at the same time anyway).

    /* aspect ratios are applied exclusive to the base_width
     *
     * min_aspect_x      width      max_aspect_x
     * ------------  <  -------  <  ------------
     * min_aspect_y      height     max_aspect_y
     *
     * beware of integer maximum (so I'll use doubles instead and divide)
     *
     * The trick is how to get back to the aspect ratio with minimal
     * change - do we modify x, y or both?
     * A: we minimise the distance between the current point, and
     *    the target aspect ratio (consider them as x,y coordinates)
     *  Consider that the aspect ratio is a line, and the current
     *  w/h is a point, so we're just using the formula for
     *  shortest distance from a point to a line!
     */

    if (min_aspect_y > 0 && max_aspect_y > 0 &&
        (height - base_height) > 0) {
        double widthd = static_cast<double>(width - base_width);
        double heightd = static_cast<double>(height - base_height);

        double min = static_cast<double>(min_aspect_x) / 
            static_cast<double>(min_aspect_y); 

        double max = static_cast<double>(max_aspect_x) / 
            static_cast<double>(max_aspect_y); 

        double actual = widthd / heightd;

        if (max > 0 && min > 0 && actual > 0) { // don't even try otherwise
            bool changed = false;
            if (actual < min) {
                changed = true;
                closestPointToLine(widthd, heightd, widthd, heightd, min);
            } else if (actual > max) {
                changed = true;
                closestPointToLine(widthd, heightd, widthd, heightd, max);
            }

            if (changed) {
                width = static_cast<int>(widthd) + base_width;
                height = static_cast<int>(heightd) + base_height;
            }
        }
    }

    // enforce incremental size limits, wrt base size
    // only calculate this if we really need to
    i = (width - base_width) / width_inc;
    width = i*width_inc + base_width;

    j = (height - base_height) / height_inc;
    height = j*height_inc + base_height;

    if (display_width)
        *display_width = i;

    if (display_height)
        *display_height = j;
}
