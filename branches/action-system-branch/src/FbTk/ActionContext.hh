// ActionContext.hh for fluxbox 
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

// $Id: ActionContext.hh,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#ifndef FBTK_ACTIONCONTEXT_HH
#define FBTK_ACTIONCONTEXT_HH

#include <string>
#include <X11/Xlib.h>

/****
 * This header contains a couple of supporting classes for
 * The action system.
 */

namespace FbTk {

/**
 * This is a small class used for clients to specify their bindings.
 * And for passing in a context to actions.
 */

class ActionBinding {
public:
    ActionBinding();

    explicit ActionBinding(ActionBinding &copy);

    // This constructor is used to parse the description string in desc
    ActionBinding(const std::string &desc);

    // For explicit creation of a binding
    ActionBinding(unsigned int value, bool iskeyvalue, unsigned int mods, ActionBinding *child = 0);
    ~ActionBinding();

    // values that define uniqueness are:
    // value, keyvalue, mods, and next
    inline bool operator == (const ActionBinding &binding) const {
        return m_value      == binding.m_value      &&
               m_iskeyvalue == binding.m_iskeyvalue &&
               m_mods       == binding.m_mods;
    }

    // pretty nasty, but needs to be consistent and safe
    inline bool operator < (const ActionBinding &binding) const {
        return m_value      < binding.m_value      || m_value      == binding.m_value      &&
             ( m_iskeyvalue < binding.m_iskeyvalue || m_iskeyvalue == binding.m_iskeyvalue && 
             ( m_mods       < binding.m_mods
               ));

    }

    bool setFromString(const std::string &str);
    // for debugging/messages
    std::string toString() const;

    inline unsigned int value() const { return m_value; }
    inline bool isKeyValue() const { return m_iskeyvalue; }
    inline unsigned int mods() const { return m_mods; }

    inline ActionBinding *child() { return m_child; }
    inline const ActionBinding *child() const { return m_child; }

protected:
    unsigned int m_value;  // the main value for this value
    bool m_iskeyvalue;     // is this a mouse or key value?
    unsigned int m_mods;  // Modifiers for this value

    ActionBinding *m_child;
};

struct ltActionBinding
{
  bool operator()(const ActionBinding* b1, const ActionBinding* b2) const
  {
      return (*b1) < (*b2);
  }
};


/**
 * These details will be built up by the handler and passed to the actions.
 * Intentionally transparent object, it's lifespan should be very short.
 */
class ActionContext {
public:
    ActionContext();
    ActionContext(Window a_win, int a_x, int a_y, 
                  int a_root_x, int a_root_y, Window a_rootwin, ActionBinding *a_binding);
    virtual ~ActionContext() {}

    Window win, rootwin;
    int x, y, x_root, y_root;
    
    ActionBinding *binding;
};

}; // end namespace FbTk


#endif // FBTK_ACTIONCONTEXT_HH
