// ActionHandler.hh for fluxbox 
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

// $Id: ActionHandler.hh,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $

#ifndef FBTK_ACTIONHANDLER_HH
#define FBTK_ACTIONHANDLER_HH

#include "ActionNode.hh"
#include "EventHandler.hh"

#include <list>
#include <map>
#include <set>
#include <X11/Xlib.h>


namespace FbTk {


/**
 * NOTES:
 * 
 * - There should only need to be one instance of each type
 *   of action, this instance will be passed the ActionableObject
 *   static_ or dynamic_casting it to the expected type
 *   => static should be used in most, if not all cases (since 
 *      we should always assocation a certain action with a compatible
 *      ActionableObj).
 * - Objects as necessary will need to be a : public ActionableObj
 */

class Action;
class ActionContext;
class ActionBinding;

struct ltActionNode
{
  bool operator()(const ActionNode* s1, const ActionNode* s2) const
  {
      return (*s1) < (*s2);
  }
};


/**
 * This class is the main interface between X and the Application.
 * It receives all Key and Button, and instigates motion tracking etc.
 */
class ActionHandler : public EventHandler {
public:

    ActionHandler();

    // little dispatcher in case it's required
    void ActionHandler::handleEvent(XEvent &ev);

    // Handle events we get
    void buttonPressEvent(XButtonEvent &ev);
    void buttonReleaseEvent(XButtonEvent &ev);
    void motionNotifyEvent(XMotionEvent &ev);
    void keyPressEvent(XKeyEvent &ev);
    void keyReleaseEvent(XKeyEvent &ev);

    // reset action table (e.g. when re-reading keybinding file).
    void resetActions();

    // register the given action description
    // If any action bindings exist with the same keys etc, then they are merged.
    // The provided actionbinding is copied, the provider should consider it their
    // responsibility to delete or whatever it wants to do with it.
    void registerAction(ActionNode &binding);

private:
    typedef std::set<ActionNode *, ltActionNode> ActionNodes;

    // does most of the work
    void processButtonEvent(XButtonEvent &ev, bool is_press);
    void processKeyEvent(XKeyEvent &ev, bool is_press);

    void processEvent(ActionNode &event, ActionContext &context, bool is_press);

    void grabBinding(ActionNode &binding);
    void ungrabBinding(ActionNode &binding);

    // Do the necessary work to initiate a [un]grab for a motionable action
    void grabActionMotion(Action &action, ActionContext &context);
    void ungrabActionMotion(Action &action, ActionContext &context);

    // given this context, does it end the appropriate mouse/key action?
    bool checkMotionStopped(ActionContext &context) const;

    ActionNodes m_bindings;

    // for motion events, they get passed straight through
    Action *m_active_mouseaction;
    unsigned int m_active_mousebutton;

    // keyboard grab data
    Action *m_active_keyaction;
    unsigned int m_active_keymods; // what to wait for to stop grab

    ActionNode *m_currchain;

};

}; // end namespace FbTk


#endif // FBTK_ACTIONHANDLER_HH
