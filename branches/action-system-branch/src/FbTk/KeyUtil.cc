// KeyUtil.cc for FbTk
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: KeyUtil.cc,v 1.4.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "KeyUtil.hh"
#include "ScreensApp.hh"

#include <string>
#include <iostream>

#include <X11/X.h>
#include <stdlib.h>

using namespace std;

namespace FbTk {

std::auto_ptr<KeyUtil> KeyUtil::s_keyutil;

KeyUtil &KeyUtil::instance() {
    if (s_keyutil.get() == 0)
        s_keyutil.reset(new KeyUtil());
    return *s_keyutil.get();
}

// This is an array of modmasks which are all
// equivalent - grabs are done with all of these
const unsigned int KeyUtil::ignored_modmasks[] = 
{
    None,     // no mods
    LockMask, // capslock
    Mod2Mask, // numlock
    Mod5Mask, // scroll lock
    LockMask|Mod2Mask,
    LockMask|Mod5Mask,
    Mod2Mask|Mod5Mask,
    LockMask|Mod2Mask|Mod5Mask,
    None // again to indicate the end
};


KeyUtil::KeyUtil()
    : m_modmap(0)
{
    init();
}

void KeyUtil::init() {
    loadModmap();
}

KeyUtil::~KeyUtil() {
    if (m_modmap)
        XFreeModifiermap(m_modmap);
}

void KeyUtil::loadModmap() {
    if (m_modmap)
        XFreeModifiermap(m_modmap);

    m_modmap = XGetModifierMapping(App::instance()->display());
}



/**
 Grabs a key with the modifier
 and with numlock,capslock and scrollock
*/
void KeyUtil::grabKey(unsigned int key, unsigned int mod) {
    ScreensApp *sapp = ScreensApp::instance();
    Display *display = sapp->display();
    const unsigned int capsmod = LockMask;
    const unsigned int nummod = Mod2Mask;
    const unsigned int scrollmod = Mod5Mask;
    
    for (int screen=0; screen<sapp->numScreens(); screen++) {
        // only grab on screens that we're active on
        if (!sapp->isScreenUsed(screen))
            continue;

        Window root = RootWindow(display, screen);
	cerr<<"grab("<<screen<<", key="<<key<<", mod="<<mod<<")"<<endl;

        for (int m = 0; m == 0 || ignored_modmasks[m] != None; ++m)
            XGrabKey(display, key, mod|ignored_modmasks[m],
                     root, True,
                     GrabModeAsync, GrabModeAsync);

    }
}

/**
 Grabs a button with the modifier
 and with numlock,capslock and scrollock
*/
void KeyUtil::grabButton(unsigned int button, unsigned int mod) {

    ScreensApp *sapp = ScreensApp::instance();
    Display *display = sapp->display();

    const unsigned int capsmod = LockMask;
    const unsigned int nummod = Mod2Mask;
    const unsigned int scrollmod = Mod5Mask;
    unsigned int event_mask = ButtonPressMask|ButtonReleaseMask|PointerMotionMask;

    for (int screen=0; screen<sapp->numScreens(); screen++) {
        // only grab on screens that we're active on
        if (!sapp->isScreenUsed(screen))
            continue;
		
        Window root = RootWindow(display, screen);
	cerr<<"grabButton("<<screen<<", button="<<button<<", mod="<<mod<<")"<<endl;

        for (int m = 0; m == 0 || ignored_modmasks[m] == None; ++m)
            XGrabButton(display, button, mod|ignored_modmasks[m],
                        root, event_mask, True,
                        GrabModeAsync, GrabModeAsync,
                        root, None);
						
    }

}

/**
 @return keycode of keystr on success else 0
*/
unsigned int KeyUtil::getKey(const char *keystr) {
    if (!keystr)
        return 0;
    return XKeysymToKeycode(App::instance()->display(),
                            XStringToKeysym(keystr));
}

/**
   convert the string to the button value
*/
unsigned int KeyUtil::getButton(const char *buttonstr) {
    if (strncasecmp(buttonstr, "Button", 6) != 0)
        return 0;

    int n = atoi(buttonstr+6);
    // sanity check
    if (n < 1 || n >= 10)
        return 0;
    else return Button1 + n-1;
    
}


/**
 @return the modifier for the modstr else zero on failure.
*/
unsigned int KeyUtil::getModifier(const char *modstr) {
    if (!modstr)
        return 0;
    struct t_modlist{
        char *str;
        unsigned int mask;
        bool operator == (const char *modstr) {
            return  (strcasecmp(str, modstr) == 0 && mask !=0);
        }
    } modlist[] = {
        {"SHIFT", ShiftMask},
        {"CONTROL", ControlMask},
        {"MOD1", Mod1Mask},
        {"MOD2", Mod2Mask},
        {"MOD3", Mod3Mask},
        {"MOD4", Mod4Mask},
        {"MOD5", Mod5Mask},
        {0, 0}
    };

    // find mod mask string
    for (unsigned int i=0; modlist[i].str !=0; i++) {
        if (modlist[i] == modstr)		
            return modlist[i].mask;		
    }
	
    return 0;	
}

/**
   @return the button mask for the modstr, or zero on failure.
*/
unsigned int KeyUtil::getButtonMask(const char *modstr) {
    return 0;
}


/// Ungrabs the keys
void KeyUtil::ungrabKeys() {
    ScreensApp *sapp = ScreensApp::instance();
    Display * display = sapp->display();
    for (int screen=0; screen<sapp->numScreens(); screen++) {
        XUngrabKey(display, AnyKey, AnyModifier,
                   RootWindow(display, screen));		
    }
}

unsigned int KeyUtil::keycodeToModmask(unsigned int keycode) {
    XModifierKeymap *modmap = instance().m_modmap;

    if (!modmap) return 0;

    // search through modmap for this keycode
    for (int mod=0; mod < 8; mod++) {
        for (int key=0; key < modmap->max_keypermod; ++key) {
            // modifiermap is an array with 8 sets of keycodes
            // each max_keypermod long, but in a linear array.
            if (modmap->modifiermap[modmap->max_keypermod*mod + key] == keycode) {
                return (1<<mod);
            }
        } 
    }
    // no luck
    return 0;
}

string KeyUtil::keyToString(unsigned int key) {
    KeySym sym = XKeycodeToKeysym(App::instance()->display(), static_cast<KeyCode>(key), 0);
    char tmp[15];
    if (sym == 0) {
        sprintf(tmp, "Keycode%d", key);
        return tmp;
    }

    const char * str = XKeysymToString(sym);
    if (!str) {
        sprintf(tmp, "Keysym%d", sym);
        return tmp;
    } else
        return str;
}

string KeyUtil::buttonToString(unsigned int button) {
    char str[8];
    if (button > 1 && button < 10) {
        sprintf(str, "Button%d", button);
        return str;
    } else
        return "";
}

string KeyUtil::modifierToString(unsigned int modmask) {
    // non-standardised modifier names
    const char *mods[] = {
        "Shift",
        "Lock",
        "Control"
    };
    int specmods = 3;

    if (modmask == 0)
        return "None";

    string result;
    bool first = true;
    char tmp[6];

    int i=0;
    while (modmask != 0) {
        if (modmask % 2) {
            if (!first)
                result += " ";
            if (i < specmods)
                result += mods[i];
            else {
                sprintf(tmp, "Mod%d", (i-specmods+1));
                result += tmp;
            }
            first = false;
        }
        modmask >>= 1;
        ++i;
    }
    return result;
}



} // end namespace FbTk
