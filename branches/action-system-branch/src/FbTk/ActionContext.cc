// ActionContext.cc for fluxbox 
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

// $Id: ActionContext.cc,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "ActionContext.hh"
#include "StringUtil.hh"
#include "KeyUtil.hh"

#include <string>
#include <vector>
#include <iostream>

using namespace std;


#include <string>

namespace FbTk {

ActionBinding::ActionBinding() :
    m_value(0),
    m_iskeyvalue(true),
    m_mods(0),
    m_child(0)
{}

ActionBinding::ActionBinding(ActionBinding &copy):
    m_value(copy.m_value),
    m_iskeyvalue(copy.m_iskeyvalue),
    m_mods(copy.m_mods),
    m_child(0)
{
    if (copy.child() != 0)
        m_child = new ActionBinding(*copy.child());
}


ActionBinding::ActionBinding(const string &desc) :
    m_value(0),
    m_iskeyvalue(true),
    m_mods(0),
    m_child(0)
{
    setFromString(desc);
}

// raw action description
ActionBinding::ActionBinding(
    unsigned int value,
    bool iskeyvalue,
    unsigned int mods,
    ActionBinding *child):

    m_value(value),
    m_iskeyvalue(iskeyvalue),
    m_mods(mods),
    m_child(child)
{}

ActionBinding::~ActionBinding() {
    if (m_child) {
        delete m_child;
        m_child = 0;
    }
}

// Parse the given action description
// This should be a keybinding description, not a 
// command definition
bool ActionBinding::setFromString(const string &str) {
    // get any leading modifiers
    // then get the key/button

    // for now we'll just implement keys
    vector<string> val;
    //Parse arguments
    FbTk::StringUtil::stringtok(val, str.c_str());

    //must have at least 1 argument
    if (val.size() <= 0) {
#ifdef DEBUG
        cerr<<"no tokens"<<endl;
#endif // DEBUG
        return false;
    }

    unsigned int tmp = 0;

    // everything up to the last one is expected to be a modifier
    // the last one is expected to be a key

    // parse all modifiers
    int i=0;
    for (; i < val.size()-1; ++i) {
        // Always force first one to be a modifier
        if (i == 0) {
            // first check if the modifier is "None"
            // since that is a special non-error case
            if (val[i] == "None") {
                m_mods = 0;
                continue;
            }
        }

        // normal key modifier?
        tmp = KeyUtil::getModifier(val[i].c_str());
        if (tmp != 0) {
            m_mods |= tmp;
            continue;
        }

        // special mouse modifier?

        // bad modifier
#ifdef DEBUG
        cerr<<"bad modifier"<<endl;
#endif // DEBUG
        return false;
    }


    // must be a key

    // standard key?
    const char * keystr = val[i].c_str();
    tmp = KeyUtil::getKey(keystr);
    if (tmp != 0) {
        m_value = tmp;
        m_iskeyvalue = true;
        return true;
    }

    // mouse button?
    tmp = KeyUtil::getButton(keystr);
    if (tmp != 0) {
        m_value = tmp;
        m_iskeyvalue = false;
        return true;
    }

    // bad value then
#ifdef DEBUG
    cerr<<"bad key/button"<<endl;
#endif // DEBUG
    return false;
}

string ActionBinding::toString() const {
    string result = KeyUtil::modifierToString(m_mods);
    result += " ";
    if (m_iskeyvalue)
        result += KeyUtil::keyToString(m_value);
    else
        result += KeyUtil::buttonToString(m_value);

    return result;
}




ActionContext::ActionContext():
    win(None),
    rootwin(None),
    x(0),
    y(0),
    x_root(0),
    y_root(0),
    binding(0)
{}

ActionContext::ActionContext(Window a_win, int a_x, int a_y, int a_root_x, int a_root_y, Window a_rootwin, ActionBinding *a_binding):
    win(a_win),
    rootwin(a_rootwin),
    x(a_x),
    y(a_y),
    x_root(a_root_x),
    y_root(a_root_y),
    binding(a_binding)
{}


}; // end namespace FbTk
