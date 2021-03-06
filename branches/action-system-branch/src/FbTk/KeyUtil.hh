// KeyUtil.hh for FbTk
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

// $Id: KeyUtil.hh,v 1.3.2.2 2004/01/28 11:03:35 rathnor Exp $

#ifndef FBTK_KEYUTIL_HH
#define FBTK_KEYUTIL_HH

#include <string>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <memory>

// We have a special pseudo-modifier of our own
#define DoubleClickMod 14
#define DoubleClickMask (1<<DoubleClickMod)

namespace FbTk {

class KeyUtil {
public:

    KeyUtil();
    ~KeyUtil();

    void init();
    static KeyUtil &instance();

    /**
       Grab the specified key
    */
    static void grabKey(unsigned int key, unsigned int mod);

    /**
       Grab the specified button
       A window of none grabs on all screens
    */
    static void grabButton(unsigned int button, unsigned int mod, Window win = None, int screen_num = -1);

    /**
       convert the string to the keysym
       @return the keysym corresponding to the string, or zero
    */
    static unsigned int getKey(const char *keystr);

    /**
       convert the string to the button value
    */
    static unsigned int getButton(const char *buttonstr);

    /**
       @return the modifier for the modstr else zero on failure.
    */
    static unsigned int getModifier(const char *modstr);

    /**
       @return the button mask for the modstr, or zero on failure.
    */
    static unsigned int getButtonMask(const char *modstr);

    /**
       ungrabs all keys
     */
    static void ungrabKeys();
    static void ungrabButtons();

    /** 
        Strip out modifiers we want to ignore
        @return the cleaned state number
    */
    static unsigned int cleanMods(unsigned int mods) {
        //remove numlock(Mod2), capslock and scrolllock(Mod5)
         return mods & ~(LockMask | Mod2Mask | Mod5Mask);
    }

    /**
       Convert the specified key into appropriate modifier mask
       @return corresponding modifier mask
    */
    static unsigned int keycodeToModmask(unsigned int keycode);

    static std::string keyToString(unsigned int key);
    static std::string buttonToString(unsigned int button);
    static std::string modifierToString(unsigned int modmask);

private:
    void loadModmap();

    XModifierKeymap *m_modmap;

    // These are all equivalent modmasks
    // we just use a static array to make it simple to loop over
    static const unsigned int ignored_modmasks[];

    static std::auto_ptr<KeyUtil> s_keyutil;
};

} // end namespace FbTk


#endif // FBTK_KEYUTIL_HH
