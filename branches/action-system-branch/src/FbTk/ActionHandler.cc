// ActionHandler.cc for fluxbox 
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

// $Id: ActionHandler.cc,v 1.1.2.2 2004/01/28 11:03:35 rathnor Exp $

#include "ActionHandler.hh"
#include "ActionNode.hh"
#include "ActionContext.hh"
#include "Action.hh"
#include "KeyUtil.hh"
#include "App.hh"
#include "FbWindow.hh"

#include <iostream>
using namespace std;

namespace FbTk {

ActionHandler::ActionHandler(GetLevelFunction func):
    m_active_mouseaction(0),
    m_active_keyaction(0),
    m_currchain(0),
    m_getlevel(func)
{}
    
unsigned int ActionHandler::getLevel(Window win) {
    // given no function, use 0 as the default rather than invalid
    if (m_getlevel == 0)
        return 0;
    else
        return m_getlevel(win);
}

void ActionHandler::handleEvent(XEvent &ev) {
    switch (ev.type) {
    case ButtonPress:
        buttonPressEvent(ev.xbutton);
        break;
    case ButtonRelease:
        buttonReleaseEvent(ev.xbutton);
        break;
    case KeyPress:
        keyPressEvent(ev.xkey);
        break;
    case KeyRelease:
        keyReleaseEvent(ev.xkey);
        break;
    case MotionNotify:
        motionNotifyEvent(ev.xmotion);
        break;
    }
}

void ActionHandler::buttonPressEvent(XButtonEvent &ev) {
    if (m_last_buttonpress_button == ev.button && 
        (ev.time - m_last_buttonpress_time) < FbTk::App::instance()->doubleClickDelay()) {
        ev.state |= DoubleClickMask;
        m_last_buttonpress_button = 0;
    } else {
        m_last_buttonpress_button = ev.button;
    }
    m_last_buttonpress_time = ev.time;

    processButtonEvent(ev, true);
}

void ActionHandler::buttonReleaseEvent(XButtonEvent &ev) {
    processButtonEvent(ev, false);
}

void ActionHandler::motionNotifyEvent(XMotionEvent &ev) {
    ActionContext context;
    if (m_active_mouseaction) {
        ActionNode *event = new ActionNode(
            0, // no key/mouse for this
            false,
            KeyUtil::cleanMods(ev.state),
            0,
            false);

        context.x = ev.x;
        context.y = ev.y;
        context.x_root = ev.x_root;
        context.y_root = ev.y_root;
        context.win = ev.window;
        context.rootwin = ev.root;

        m_active_mouseaction->motion(context);

        delete event;
    }
}

void ActionHandler::keyPressEvent(XKeyEvent &ev) {
    processKeyEvent(ev, true);
}

void ActionHandler::keyReleaseEvent(XKeyEvent &ev) {
    processKeyEvent(ev, false);
}

void ActionHandler::processButtonEvent(XButtonEvent &ev, bool is_press) {
    ActionContext context;
    ActionNode *event = new ActionNode(
        ev.button,
        false,
        KeyUtil::cleanMods(ev.state),
        0,
        false);

    context.x = ev.x;
    context.y = ev.y;
    context.x_root = ev.x_root;
    context.y_root = ev.y_root;
    context.win = (ev.subwindow==None)?ev.window:ev.subwindow;
    context.rootwin = ev.root;
    context.binding = event;

    processEvent(*event, context, is_press);
    delete event;
}

void ActionHandler::processKeyEvent(XKeyEvent &ev, bool is_press) {
    ActionContext context;
    ActionNode *event = new ActionNode(
       ev.keycode,
       true,
       KeyUtil::cleanMods(ev.state),
       0,
       false);

    context.x = ev.x;
    context.y = ev.y;
    context.x_root = ev.x_root;
    context.y_root = ev.y_root;
    context.win = (ev.subwindow==None)?ev.window:ev.subwindow;
    context.rootwin = ev.root;
    context.binding = event;

    processEvent(*event, context, is_press);
    delete event;
}


// does most of the work
void ActionHandler::processEvent(ActionNode &event, ActionContext &context, bool is_press) {
    // It is a press or release action
    // so, we need to search our bindings for a matching 
    // Action, and act appropriately

    ActionNode *leafnode = 0;

    // if we are already in the middle of a chain, see if this is 
    // a continuation of the chain
    if (m_currchain) {
        // TODO chaining
        // Walk through the children of the current chained action
        // to check for matches with this one.
    } 

    bool found = false;

    // not in chain currently, or this wasn't a continuation.
    if (!leafnode) {
        ActionNodes::iterator it = m_bindings.find(&event);
        // if it's a double click, and nothing found, try as single click
        if (is_press && (context.binding->mods() & DoubleClickMask != 0) &&
            it == m_bindings.end()) {
            context.binding->setMods(context.binding->mods() & ~DoubleClickMask);
            it = m_bindings.find(&event);
        }

        if (it == m_bindings.end()) {
            if ((m_active_keyaction || m_active_mouseaction) && !is_press) {
                Action *foundaction = 0;
                // let it fall through to check for grab stops
                if (context.binding->isKeyValue())
                    foundaction = m_active_keyaction;
                else
                    foundaction = m_active_mouseaction;
    
                triggerAction(foundaction, context, is_press);
                found = true;
            }
        } else {

            leafnode = (*it);
            if (leafnode->hasChildren()) { // it is part of a chain
                m_currchain = leafnode;
                cerr<<"skip event - chained"<<endl;
                // return (not fall through) since we do want to absorb the keypress
                if (leafnode->actionList().empty())
                    return;
            }

            // no nodes have siblings at the root level
            unsigned int winlevel = getLevel(context.win);
            ActionNode::ActionList::iterator it = leafnode->actionList().begin();
            ActionNode::ActionList::iterator it_end = leafnode->actionList().end();
            cerr<<"winlevel = "<<winlevel<<" of win = "<<hex<<context.win<<dec<<endl;
            for (; it != it_end; ++it) {
                unsigned int level = (*it)->getLevel();
                cerr<<"check level "<<level<<endl;
                if (level == 0 || (level & winlevel) != 0) {
                    triggerAction(*it, context, is_press);
                    found = true;
                }
            }
        }
    }

    // note that leafnode can't be zero any more

    // TODO: probably need to do something about m_active_*action about here

    // so... figure out which action (if any) for this leafnode

    if (!found) {
        // if it's a mouse click, let it through since we didn't find anything
        cerr<<"None Found"<<endl;
        if (!context.binding->isKeyValue()) {
            XAllowEvents(FbTk::App::instance()->display(), ReplayPointer, m_last_buttonpress_time);
            cerr<<"Allow Events"<<endl;
        }
        return;
    }

} 


// reset action table (e.g. when re-reading keybinding file).
void ActionHandler::resetActions() {
    ActionNodes::iterator it = m_bindings.begin();
    ActionNodes::iterator it_end = m_bindings.end();
    for (; it != it_end; ++it) {
        // each node recurses itself
        delete (*it);
    }

    KeyUtil::ungrabKeys();
    KeyUtil::ungrabButtons();
    m_bindings.clear();
}

/*****************************
 *  ActionHandler and Grabs  *
 *****************************
 *
 * Here is a bit of discussion of the way actionhandler deals
 * with keyboard and pointer grabs.
 *
 * So, here are the main points and ideas:
 *
 * - ActionHandler maintains a base-level map containing all of
 *   the initial keys for the start of any chain.
 *   The entries in this map should always be grabbed.
 *
 * - keychains will add a special "null action" entry to the base map
 *   to indicate that they have grabbed a certain key if it wasn't 
 *   already grabbed.
 *
 * - we ungrab on reset and at end of chains (For any additional grabs that 
 *   were made for the chain)
 *
 * - When a motion action is encountered, the whole pointer is grabbed.
 *
 * - Keyboard grabs are always made with appropriate modifiers
 *   as specified in ActionHandler::GrabMask
 *
 * - All key grabs are made on all _root_ windows (only).
 * - All mouse grabs are made on the client windows, and other windows 
 *   must ask for buttonpress/release/motion events to be given.
 *
 * Chaining
 * 
 * Chaining is where an action is reached by a sequence of more than
 * one binding. Particularly, the deeper bindings shoud not be 
 * normally grabbed (as they may be more trivial key combinations).
 *
 * Thus, we maintain a special list of additional temporary grabs that
 * must be kept in mind when doing new grabbing/ungrabbing and when 
 * a chaining-related event occurs (be it something to stop an active chain).
 *
 */



// Register this actionbinding, which may contain one or more
// will be added for the appropriate key/button combos
// The action only applies to win (which may be for focus or click
// depending upon whether the action is pointer or focus relative)
void ActionHandler::registerAction(ActionNode &binding) {
    ActionNodes::iterator it = m_bindings.find(&binding);
    ActionNode *m_binding = 0;
    // TODO: chaining (should actionlist be spliced deeper?)
    if (it == m_bindings.end()) {
        // copy and add
        m_binding = new ActionNode(binding); // copy
        grabBinding(*m_binding); // set grabs
        m_bindings.insert(m_binding);
    } else {
        m_binding = (*it);
        grabBinding(*m_binding, &binding.actionList());
        m_binding->actionList().splice(m_binding->actionList().end(), binding.actionList());
    }

}

void ActionHandler::grabBinding(ActionNode &binding, ActionNode::ActionList *actionlist) {
    // no grabs for non-key bindings
    if (binding.isKeyValue())
        KeyUtil::grabKey(binding.value(), binding.mods());
    else {
        // remember it for later foreign grabs
        if (actionlist == 0)
            actionlist = &binding.actionList();

        if (!actionlist->empty()) {
            int levelmask = 0;
            ActionNode::ActionList::iterator it = actionlist->begin();
            ActionNode::ActionList::iterator it_end = actionlist->end();
            for (; it != it_end; ++it) 
                levelmask |= (*it)->getLevel();

            int i=1;
            for (; levelmask != 0; i <<= 1) {
                if (levelmask & i) {
                    m_levels.insert(std::make_pair(i, &binding));
                    levelmask ^= i;
                }
            }
        }
    }
}

void ActionHandler::ungrabBinding(ActionNode &binding) {
    if (!binding.isKeyValue())
        return;

    // TODO... not sure if we even need to or should ungrab
    // yep, need to for chaining...
}

void ActionHandler::grabActionMotion(Action &action, ActionContext &context) {
    // The appropriate active action must be zero, since otherwise
    // we wouldn't get here. context.binding must also exist
    Display *display = App::instance()->display();
    cerr<<"grabActionMotion!"<<endl;

    // key grab? (e.g. window cycling)
    if (context.binding->isKeyValue()) {
        m_active_keyaction = &action;
        m_active_keymods = context.binding->mods();

        XGrabKeyboard(display,
                      context.rootwin, True, 
                      GrabModeAsync, GrabModeAsync, CurrentTime);
    } else {
        // mouse grab (e.g. window moving/dragging)
        m_active_mouseaction = &action;
        m_active_mousebutton = context.binding->value();

        XGrabPointer(display, context.rootwin, False, PointerMotionMask |
                     ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                     context.rootwin, action.cursor(), CurrentTime);

    }
}

void ActionHandler::ungrabActionMotion(Action &action, ActionContext &context) {
    // The appropriate active action must be zero, since otherwise
    // we wouldn't get here. context.binding must also exist
    Display *display = App::instance()->display();

    // key grab? (e.g. window cycling)
    if (context.binding->isKeyValue()) {
        m_active_keyaction = 0;
        m_active_keymods = 0;

        XUngrabKeyboard(display, CurrentTime);
    } else {
        // mouse grab (e.g. window moving/dragging)
        m_active_mouseaction = 0;
        m_active_mousebutton = 0;
        XUngrabPointer(display, CurrentTime);

    }
}

bool ActionHandler::checkMotionStopped(ActionContext &context) const {
    // if key, check against key action

    if (context.binding->isKeyValue()) {
        /**
         * A key motion stops when all assocated modifiers
         * have been released.
         */
        unsigned int mods = context.binding->mods();

        // mask out the value of any released modifier
        mods &= ~KeyUtil::keycodeToModmask(context.binding->value());

        // strip out irrelevant mods, see if it is zero
        if ((mods & m_active_keymods) == 0)
            return true;
        else
            return false;
    } else {
        // mouse binding checking

        /**
         * A mouse motion stops when the primary button has
         * been released.
         */

        // mouse release actions have their button as the
        // active one, and their mask ON

        unsigned int buttonmask = Button1Mask <<
            (context.binding->value() - Button1);
        if (m_active_mousebutton == context.binding->value() &&
            (context.binding->mods() & buttonmask) != 0) {
            return true;
        } else
            return false;
    }
}

void ActionHandler::registerWindow(unsigned int level, FbWindow &win) {
    registerWindow(level, win.window(), win.screenNumber());
}

void ActionHandler::registerWindow(unsigned int level, Window win, int screen_num) {
    ActionLevels::iterator it = m_levels.find(level);
    ActionLevels::iterator it_end = m_levels.end();
    while (it != it_end && it->first == level) {
        ActionNode *node = it->second;
        if (!node->isKeyValue())
            KeyUtil::grabButton(node->value(), node->mods(), win, screen_num);

        ++it;
    }
}

void ActionHandler::triggerAction(Action *action, ActionContext &context, bool is_press) {
    if (action == 0)
        return; // this shouldn't happen

    // TODO: handle motion action setting etc
    if (is_press) {

        if (action->hasMotionAction() &&
            (context.binding->isKeyValue() && m_active_keyaction == 0 ||
             !context.binding->isKeyValue() && m_active_mouseaction == 0)) {

            // we have to start before the grab, to inform the action 
            // of what's going on, particularly so it can update
            // it's cursor if necessary
            if (!action->hasStartAction() || action->start(context))
                grabActionMotion(*action, context);

        } else {
            // no new motion action, check for existing one...
            if (action == m_active_mouseaction || 
                action == m_active_keyaction)
                action->motion(context);
            else if (action->hasStartAction())
                action->start(context);
        }
    } else {
        // release action
        // first check if this finishes a motion action
        if (action == m_active_mouseaction || 
            action == m_active_keyaction) {
            if (checkMotionStopped(context)) {
                ungrabActionMotion(*action, context);

                if (action->hasStopAction())
                    action->stop(context);
            }

        } else if (action->hasStopAction())
            action->stop(context);
    }

}

}; // end namespace FbTk
