// ActionNode.cc for fluxbox 
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

// $Id: ActionNode.cc,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "ActionNode.hh"
#include "Action.hh"

#include <string>

namespace FbTk {

ActionNode::ActionNode(bool is_default):
    ActionBinding(0, true, 0),
    m_sibling(0),
    m_default(is_default),
    m_action(0)
{}

ActionNode::ActionNode(std::string &desc, bool is_default):
    ActionBinding(desc),
    m_sibling(0),
    m_default(is_default),
    m_action(0)
{}

// raw action description
ActionNode::ActionNode(
    unsigned int value,
    bool iskeyvalue,
    unsigned int mods,
    ActionNode *chainnext,
    bool is_default) :
    ActionBinding(value, iskeyvalue, mods, chainnext),
    m_sibling(0),
    m_default(is_default),
    m_action(0)
{}

ActionNode::~ActionNode() {
    if (m_action && !m_action->isGlobal()) {
        delete m_action;
        m_action = 0;
    }

    // delete whole chain, and siblings
    if (m_sibling) {
        delete m_sibling;
        m_sibling = 0;
    }
}

void ActionNode::setAction(Action *action) {
    if (m_action && !m_action->isGlobal())
        delete m_action;
    m_action = action;
}

}; // end namespace FbTk
