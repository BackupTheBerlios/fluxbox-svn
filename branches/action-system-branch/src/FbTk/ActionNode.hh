// ActionNode.hh for fluxbox 
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

// $Id: ActionNode.hh,v 1.1.2.2 2004/01/28 11:03:35 rathnor Exp $

#ifndef FBTK_ACTIONNODE_HH
#define FBTK_ACTIONNODE_HH

#include "ActionContext.hh"

#include <set>
#include <list>
#include <string>

namespace FbTk {

class Action;
class ActionNode;

struct ltActionNode
{
    bool operator()(const ActionNode* s1, const ActionNode* s2) const;

};



/**
 * An ActionNode is the representation for the sequence of events needed
 * For triggering an action.
 */
class ActionNode : public ActionBinding {
public:
    typedef std::list<Action *> ActionList;
    typedef std::set<ActionNode *, ltActionNode> NodeChildren;

    ActionNode(bool is_default = false);

    // This constructor is used to parse the description string in desc
    ActionNode(std::string &desc, bool is_default = false);

    // For explicit creation of a binding
    ActionNode(unsigned int value, bool iskeyvalue, unsigned int mods, ActionNode *chainnext, bool is_default);
    ~ActionNode();

    // values that define uniqueness are:
    // value, keyvalue, mods, and next
    inline bool operator == (const ActionNode &node) const {
        return ActionBinding::operator==(node);
    }

    // pretty nasty, but needs to be consistent and safe
    inline bool operator < (const ActionNode &node) const {
        return ActionBinding::operator<(node);
    }

    // use deepest_child when you've just parsed a keybinding that
    // may have represented a chain (thus the action should be 
    // at the end of the chain)
    void addAction(Action *action, bool deepest_child = false);

    // provided child may be deleted - it now belongs to this node
    // BEWARE that the child() function from ActionBinding makes
    // no sense for [external] use with ActionNodes
    void addChild(ActionNode *child);

    // merge given action node into this one
    // deletes provided node
    void merge(ActionNode *node);

    inline ActionList &actionList() { return m_actions; }
    inline bool hasChildren() { return m_children != 0 && !m_children->empty(); }

    // should always check hasChildren() first
    inline NodeChildren children() { return *m_children; }

private:
    void mergeBindingChildren(ActionBinding &binding);

    bool m_default;       // is this a "default" binding (i.e. overrideable)

    ActionList m_actions;
    // chaining
    NodeChildren *m_children; // pointer since not many will have it...
};


}; // end namespace FbTk

#endif // FBTK_ACTIONNODE_HH
