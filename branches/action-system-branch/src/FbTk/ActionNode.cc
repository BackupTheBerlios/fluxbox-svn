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

// $Id: ActionNode.cc,v 1.1.2.2 2004/01/28 11:03:35 rathnor Exp $

#include "ActionNode.hh"
#include "Action.hh"

namespace FbTk {

ActionNode::ActionNode(bool is_default):
    ActionBinding(0, true, 0),
    m_default(is_default),
    m_children(0)
{}

ActionNode::ActionNode(std::string &desc, bool is_default):
    ActionBinding(desc),
    m_default(is_default),
    m_children(0)
{
    // TODO: chaining: check for child from actionbinding
    // and propagate up to our list of children

    if (child() != 0)
        mergeBindingChildren(*this);
}

// raw action description
ActionNode::ActionNode(
    unsigned int value,
    bool iskeyvalue,
    unsigned int mods,
    ActionNode *chainnext,
    bool is_default) :
    ActionBinding(value, iskeyvalue, mods, chainnext),
    m_default(is_default),
    m_children(0)
{
    // TODO: chaining: check for child from actionbinding
    // and propogate up to out list of children
    if (child() != 0)
        mergeBindingChildren(*this);
}

ActionNode::~ActionNode() {
    ActionList::iterator it = m_actions.begin();
    ActionList::iterator it_end = m_actions.end();

    for (; it != it_end; ++it) {
        if (!(*it)->isGlobal())
            delete (*it);
    }

    if (m_children != 0) {
        NodeChildren::iterator it = m_children->begin();
        NodeChildren::iterator it_end = m_children->end();
        for (; it != it_end; ++it)
            delete (*it);

        delete m_children;
    }
}

void ActionNode::addAction(Action *action, bool deepest_child) {
    if (!deepest_child || m_children == 0 || m_children->empty())
        m_actions.push_back(action);
    else
        (*m_children->begin())->addAction(action, deepest_child);
}

void ActionNode::addChild(ActionNode *child) {
    NodeChildren::iterator it;
    if (m_children == 0) 
        m_children = new NodeChildren();
    else if ((it = m_children->find(child)) != m_children->end()) {
        // if already a child with that binding, then merge them
        (*it)->merge(child);
        return;
    }

    m_children->insert(child);
}

// merge provided action node into this node
// -> add any children, and any actions
void ActionNode::merge(ActionNode *node) {
    // first, merge any children in
    NodeChildren *children = node->m_children;
    if (children && !children->empty()) {
        if (m_children == 0) { 
            // if they have children, and we don't - steal them
            m_children = children;
            node->m_children = 0;
        } else {
            // otherwise we merge them, and if any clash, we merge them
            NodeChildren::iterator it = children->begin();
            NodeChildren::iterator tmpit;
            NodeChildren::iterator it_end = children->end();
            for (; it != it_end; ++it) {
                if ((tmpit = m_children->find(*it)) == it_end)
                    m_children->insert(*it);
                else
                    (*tmpit)->merge(*it);
            }
            delete children;
            node->m_children = 0;
        }
    } // no more children

    // now merge the actions of the node
    m_actions.splice(m_actions.end(), node->actionList());
    // we steal their actions - don't let it try to delete any
    node->actionList().clear(); 

    delete node;
}

// add the children of this binding to be our children
void ActionNode::mergeBindingChildren(ActionBinding &binding) {
    ActionNode *currnode = this, *tempnode;
    ActionBinding *currbinding = binding.child();
    while (currbinding != 0) {
        tempnode = new ActionNode(currbinding->value(),
                                  currbinding->isKeyValue(),
                                  currbinding->mods(),
                                  0,
                                  m_default);
        currnode->addChild(tempnode);
        currnode = tempnode;
        currbinding = currbinding->child();
    }
}

bool ltActionNode::operator()(const ActionNode* s1, const ActionNode* s2) const
{
    return (*s1) < (*s2);
}


}; // end namespace FbTk
