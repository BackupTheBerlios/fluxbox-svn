// FbWinFrame.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2005 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

// $Id$

#include "FbWinFrame.hh"

#include "FbTk/ImageControl.hh"
#include "FbTk/EventManager.hh"
#include "FbTk/TextButton.hh"
#include "FbTk/App.hh"
#include "FbTk/SimpleCommand.hh"
#include "FbTk/Compose.hh"
#include "FbTk/Transparent.hh"
#include "CompareWindow.hh"
#include "FbWinFrameTheme.hh"
#include "fluxbox.hh"

#ifdef SHAPE
#include "Shape.hh"
#endif // SHAPE


#include <algorithm>
#include <iostream>
#include <X11/X.h>

using namespace std;

FbWinFrame::FbWinFrame(FbWinFrameTheme &theme, FbTk::ImageControl &imgctrl, 
                       int x, int y,
                       unsigned int width, unsigned int height):
    m_theme(theme),
    m_imagectrl(imgctrl),
    m_window(theme.screenNum(), x, y, width, height,  ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | EnterWindowMask, true),
    m_titlebar(m_window, 0, 0, 100, 16, 
               ButtonPressMask | ButtonReleaseMask |
               ButtonMotionMask | ExposureMask |
               EnterWindowMask | LeaveWindowMask),
    m_label(m_titlebar, 0, 0, 100, 16,
	    ButtonPressMask | ButtonReleaseMask |
            ButtonMotionMask | ExposureMask |
            EnterWindowMask | LeaveWindowMask),
    m_handle(m_window, 0, 0, 100, 5,
             ButtonPressMask | ButtonReleaseMask |
             ButtonMotionMask | ExposureMask |
             EnterWindowMask | LeaveWindowMask),
    m_grip_right(m_handle, 0, 0, 10, 4,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_grip_left(m_handle, 0, 0, 10, 4,
		ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | ExposureMask |
		EnterWindowMask | LeaveWindowMask),
    m_clientarea(m_window, 0, 0, 100, 100,
                 ButtonPressMask | ButtonReleaseMask |
                 ButtonMotionMask | ExposureMask |
                 EnterWindowMask | LeaveWindowMask),
    m_bevel(1),
    m_use_titlebar(true), 
    m_use_handle(true),
    m_focused(false),
    m_visible(false),
    m_button_pm(0),
    m_need_render(true),
    m_themelistener(*this),
    m_shape(new Shape(m_window, theme.shapePlace())) {
    m_theme.reconfigSig().attach(&m_themelistener);
    init();
}

FbWinFrame::~FbWinFrame() {
    removeEventHandler();
    removeAllButtons();
}

bool FbWinFrame::setOnClickTitlebar(FbTk::RefCount<FbTk::Command> &ref, int mousebutton_num, 
                            bool double_click, bool pressed) {
    // find mousebutton_num
    if (mousebutton_num < 1 || mousebutton_num > 5)
        return false;
    if (double_click)
        m_commands[mousebutton_num - 1].double_click = ref;
    else {
        if (pressed)
            m_commands[mousebutton_num - 1].click_pressed = ref;
        else
            m_commands[mousebutton_num - 1].click = ref;
    }

    return true;
}

void FbWinFrame::hide() {
    m_window.hide();
    m_visible = false;
}

void FbWinFrame::show() {
    m_visible = true;

    if (m_need_render) {
        renderAll();
        applyAll();
        clearAll();
    }

    m_window.showSubwindows();
    m_window.show();
}

/**
 Toggle shade state, and resize window
 */
void FbWinFrame::shade() {
    if (!m_use_titlebar)
        return;

    // toggle shade
    m_shaded = !m_shaded;
    if (m_shaded) { // i.e. should be shaded now
        m_width_before_shade = m_window.width();
        m_height_before_shade = m_window.height();
        m_window.resize(m_window.width(), m_titlebar.height());
    } else { // should be unshaded
        m_window.resize(m_width_before_shade, m_height_before_shade);
        reconfigure();
    }
}


void FbWinFrame::move(int x, int y, int win_gravity) {
    moveResize(x, y, 0, 0, true, false, win_gravity);
}

void FbWinFrame::resize(unsigned int width, unsigned int height, int win_gravity) {
    moveResize(0, 0, width, height, false, true, win_gravity);
}

// need an atomic moveresize where possible
void FbWinFrame::moveResizeForClient(int x, int y, unsigned int width, unsigned int height, bool move, bool resize, int win_gravity) {
    // total height for frame

    unsigned int total_height = height;

    if (resize) {
        // having a titlebar = 1 extra border + titlebar height
        if (m_use_titlebar)
            total_height += m_titlebar.height() + m_titlebar.borderWidth();
        // having a handle = 1 extra border + handle height
        if (m_use_handle)
            total_height += m_handle.height() + m_handle.borderWidth();
    }
    moveResize(x, y, width, total_height, move, resize, win_gravity);
}

void FbWinFrame::resizeForClient(unsigned int width, unsigned int height, int win_gravity) {
    moveResizeForClient(0, 0, width, height, false, true, win_gravity);
}

void FbWinFrame::moveResize(int x, int y, unsigned int width, unsigned int height, bool move, bool resize, int win_gravity) {
    if(win_gravity!=ForgetGravity) {
        gravityTranslate(x, y, width + m_window.borderWidth()*2, height + m_window.borderWidth()*2, win_gravity, false);
    }
    if (move && x == window().x() && y == window().y()) 
        move = false;

    if (resize && width == FbWinFrame::width() && height == FbWinFrame::height()) 
        resize = false;

    if (!move && !resize)
        return;

    if (resize && m_shaded) {
        // update unshaded size if  we're in shaded state and just resize width
        m_width_before_shade = width;
        m_height_before_shade = height;
        height = m_window.height();
    }

    if (move && resize) {
        m_window.moveResize(x, y, width, height);
        notifyMoved(false); // will reconfigure
    } else if (move) {
        m_window.move(x, y);
        // this stuff will be caught by reconfigure if resized
        notifyMoved(true);
    } else {
        m_window.resize(width, height);
    }

    if (resize)
        reconfigure();
}

void FbWinFrame::notifyMoved(bool clear) {
    // not important if no alpha...
    unsigned char alpha = (m_focused?theme().focusedAlpha():theme().unfocusedAlpha());
    if (alpha == 255) 
        return;

    if (m_use_titlebar) {
        m_titlebar.parentMoved();
        m_label.parentMoved();

        for_each(m_buttons_left.begin(),
                 m_buttons_left.end(),
                 mem_fun(&FbTk::Button::parentMoved));
        for_each(m_buttons_right.begin(),
                 m_buttons_right.end(),
                 mem_fun(&FbTk::Button::parentMoved));
        for_each(m_labelbuttons.begin(),
                 m_labelbuttons.end(),
                 mem_fun(&FbTk::Button::parentMoved));
    }

    if (m_use_handle) {
        m_handle.parentMoved();
        m_grip_left.parentMoved();
        m_grip_right.parentMoved();
    }

    if (clear && (m_use_handle || m_use_titlebar)) {
        clearAll();
    }
}

void FbWinFrame::clearAll() {

    if  (m_use_titlebar) {
        redrawTitlebar();

        for_each(m_buttons_left.begin(),
                 m_buttons_left.end(),
                 mem_fun(&FbTk::Button::clear));
        for_each(m_buttons_right.begin(),
                 m_buttons_right.end(),
                 mem_fun(&FbTk::Button::clear));
    }

    if (m_use_handle) {
        m_handle.clear();
        m_grip_left.clear();
        m_grip_right.clear();
    }
}

void FbWinFrame::setFocus(bool newvalue) {
    if (m_focused == newvalue)
        return;

    m_focused = newvalue;

    if (FbTk::Transparent::haveRender() && theme().focusedAlpha() != theme().unfocusedAlpha()) {
        unsigned char alpha = (m_focused?theme().focusedAlpha():theme().unfocusedAlpha());
        if (FbTk::Transparent::haveComposite()) {
            m_window.setOpaque(alpha);
        } else {
            LabelList::iterator btn_it = m_labelbuttons.begin();
            LabelList::iterator btn_it_end = m_labelbuttons.end();        
            for (; btn_it != btn_it_end; ++btn_it) {
                (*btn_it)->setAlpha(alpha);
                if (m_current_label != (*btn_it))
                    (*btn_it)->updateBackground(false);
            }
        }
    }

    if (currentLabel()) {
        if (newvalue) // focused
            applyFocusLabel(*m_current_label);       
        else // unfocused
            applyActiveLabel(*m_current_label);
    }

    applyTitlebar();
    applyHandles();
    clearAll();
}

void FbWinFrame::setDoubleClickTime(unsigned int time) {
    m_double_click_time = time;
}

void FbWinFrame::addLeftButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    applyButton(*btn); // setup theme and other stuff
    
    m_buttons_left.push_back(btn);
}

void FbWinFrame::addRightButton(FbTk::Button *btn) {
    if (btn == 0) // valid button?
        return;

    applyButton(*btn); // setup theme and other stuff

    m_buttons_right.push_back(btn);
}

void FbWinFrame::removeAllButtons() {
    // destroy left side
    while (!m_buttons_left.empty()) {
        delete m_buttons_left.back();
        m_buttons_left.pop_back();
    }
    // destroy right side
    while (!m_buttons_right.empty()) {
        delete m_buttons_right.back();
        m_buttons_right.pop_back();
    }
}

FbWinFrame::ButtonId FbWinFrame::createTab(const std::string &title, FbTk::Command *command) {
    FbTk::TextButton *button = new FbTk::TextButton(label(),
                                                    theme().font(),
                                                    title);
    button->show();
    button->setEventMask(ExposureMask | ButtonPressMask |
                          ButtonReleaseMask | ButtonMotionMask |
                         EnterWindowMask);
    
    FbTk::RefCount<FbTk::Command> refcmd(command);
    button->setOnClick(refcmd);

    button->setTextPadding(Fluxbox::instance()->getTabsPadding());
    button->setJustify(theme().justify());

    m_labelbuttons.push_back(button);

    if (currentLabel() == 0)
        setLabelButtonFocus(*button);

    return button;
}

void FbWinFrame::removeTab(ButtonId btn) {
    LabelList::iterator erase_it = remove(m_labelbuttons.begin(),
                                          m_labelbuttons.end(),
                                          btn);
    if (erase_it == m_labelbuttons.end())
        return;

    if (btn == m_current_label)
        m_current_label = 0;

    m_labelbuttons.erase(erase_it);
    delete btn;
}


void FbWinFrame::moveLabelButtonLeft(const FbTk::TextButton &btn) {
    LabelList::iterator it = find(m_labelbuttons.begin(),
                                  m_labelbuttons.end(),
                                  &btn);
    // make sure we found it and we're not at the begining
    if (it == m_labelbuttons.end() || it == m_labelbuttons.begin())
        return;

    LabelList::iterator new_pos = it;
    new_pos--;
    FbTk::TextButton *item = *it;
    // remove from list
    m_labelbuttons.erase(it); 
    // insert on the new place
    m_labelbuttons.insert(new_pos, item);
    // update titlebar
    redrawTitlebar();
}

void FbWinFrame::moveLabelButtonRight(const FbTk::TextButton &btn) {
    LabelList::iterator it = find(m_labelbuttons.begin(),
                                  m_labelbuttons.end(),
                                  &btn);
    // make sure we found it and we're not at the last item
    if (it == m_labelbuttons.end() || *it == m_labelbuttons.back())
        return;

    FbTk::TextButton *item = *it;
    // remove from list
    LabelList::iterator new_pos = m_labelbuttons.erase(it); 
    new_pos++;
    // insert on the new place
    m_labelbuttons.insert(new_pos, item);
    // update titlebar
    redrawTitlebar();
}

void FbWinFrame::moveLabelButtonTo(FbTk::TextButton &btn, int x, int y) {
    Window parent_return=0,
        root_return=75,
        *children_return = NULL;

    unsigned int nchildren_return;

    // get the root window
    if (!XQueryTree(window().display(), window().window(),
                    &root_return, &parent_return, &children_return, &nchildren_return))
        parent_return = parent_return;

    if (children_return != NULL)
        XFree(children_return);

    int dest_x = 0, dest_y = 0;
    Window labelbutton = 0;
    if (!XTranslateCoordinates(window().display(),
                               root_return, label().window(),
                               x, y, &dest_x, &dest_y,
                               &labelbutton))
        return;

    LabelList::iterator it = find_if(m_labelbuttons.begin(),
                                     m_labelbuttons.end(),
                                     CompareWindow(&FbTk::Button::window,
                                                   labelbutton));
    // label button not found
    if (it == m_labelbuttons.end())
        return;

    Window child_return = 0;
    //make x and y relative to our labelbutton
    if (!XTranslateCoordinates(window().display(),
                               label().window(),labelbutton,
                               dest_x, dest_y, &x, &y,
                               &child_return))
        return;

    if (x > (*it)->width() / 2)
        moveLabelButtonRightOf(btn,**it);
    else
        moveLabelButtonLeftOf(btn,**it);

}



void FbWinFrame::moveLabelButtonLeftOf(const FbTk::TextButton &btn, const FbTk::TextButton &dest) {
    LabelList::iterator it = find(m_labelbuttons.begin(),
                                  m_labelbuttons.end(),
                                  &btn);
    LabelList::iterator new_pos = find(m_labelbuttons.begin(),
				       m_labelbuttons.end(),
				       &dest);

    // make sure we found them
    if (it == m_labelbuttons.end() || new_pos == m_labelbuttons.end())
        return;

    // moving a button to the left of itself results in no change
    if (new_pos == it)
        return;

    FbTk::TextButton *item = *it;
    // remove from list
    m_labelbuttons.erase(it);
    // insert on the new place
    m_labelbuttons.insert(new_pos, item);
    // update titlebar
    redrawTitlebar();
}

void FbWinFrame::moveLabelButtonRightOf(const FbTk::TextButton &btn, const FbTk::TextButton &dest) {
    LabelList::iterator it = find(m_labelbuttons.begin(),
                                  m_labelbuttons.end(),
                                  &btn);
    LabelList::iterator new_pos = find(m_labelbuttons.begin(),
				       m_labelbuttons.end(),
				       &dest);

    // make sure we found them
    if (it == m_labelbuttons.end() || new_pos == m_labelbuttons.end())
        return;

    //moving a button to the right of itself results in no change
    if (new_pos == it)
        return;

    FbTk::TextButton *item = *it;
    // remove from list
    m_labelbuttons.erase(it);
    // need to insert into the next position
    new_pos++;
    // insert on the new place
    if (new_pos == m_labelbuttons.end())
        m_labelbuttons.push_back(item);
    else
        m_labelbuttons.insert(new_pos, item);
    //update titlebar
    redrawTitlebar();
}

void FbWinFrame::setLabelButtonFocus(FbTk::TextButton &btn) {
    if (&btn == currentLabel() || btn.parent() != &label())
        return;

    // render label buttons
    if (currentLabel() != 0)
        applyUnfocusLabel(*m_current_label);

    m_current_label = &btn; // current focused button

    if (m_focused)
        applyFocusLabel(*m_current_label);
    else
        applyActiveLabel(*m_current_label);
}

void FbWinFrame::setClientWindow(FbTk::FbWindow &win) {

    win.setBorderWidth(0);

    XChangeSaveSet(win.display(), win.window(), SetModeInsert);

    m_window.setEventMask(NoEventMask);

    // we need to mask this so we don't get unmap event
    win.setEventMask(NoEventMask);
    win.reparent(m_window, 0, clientArea().y());
    // remask window so we get events
    win.setEventMask(PropertyChangeMask | StructureNotifyMask | 
                     FocusChangeMask);

    m_window.setEventMask(ButtonPressMask | ButtonReleaseMask |
                          ButtonMotionMask | EnterWindowMask | SubstructureRedirectMask);

    XFlush(win.display());

    XSetWindowAttributes attrib_set;
    attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask;
    attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | 
        ButtonMotionMask;

    XChangeWindowAttributes(win.display(), win.window(), CWEventMask|CWDontPropagate, &attrib_set);

    m_clientarea.raise();
    win.raise();
    m_window.showSubwindows();

}

bool FbWinFrame::hideTitlebar() {
    if (!m_use_titlebar)
        return false;

    m_titlebar.hide();
    m_use_titlebar = false;
    if (static_cast<signed int>(m_window.height() - m_titlebar.height() -
                                m_titlebar.borderWidth()) <= 0) {
        m_window.resize(m_window.width(), 1);
    } else {
        // only take away one borderwidth (as the other border is still the "top" border)
        m_window.resize(m_window.width(), m_window.height() - m_titlebar.height() -
                        m_titlebar.borderWidth());
    }

    return true;
}

bool FbWinFrame::showTitlebar() {
    if (m_use_titlebar)
        return false;

    m_titlebar.show();
    m_use_titlebar = true;

    // only add one borderwidth (as the other border is still the "top" border)
    m_window.resize(m_window.width(), m_window.height() + m_titlebar.height() +
                    m_titlebar.borderWidth());
    return true;

}

bool FbWinFrame::hideHandle() {
    if (!m_use_handle)
        return false;
    m_handle.hide();
    m_grip_left.hide();
    m_grip_right.hide();
    m_use_handle = false;

    if (static_cast<signed int>(m_window.height() - m_handle.height() -
                                m_handle.borderWidth()) <= 0) {
        m_window.resize(m_window.width(), 1);
    } else {
        // only take away one borderwidth (as the other border is still the "top" border)
        m_window.resize(m_window.width(), m_window.height() - m_handle.height() -
                        m_handle.borderWidth());
    }

    return true;

}

bool FbWinFrame::showHandle() {
    if (m_use_handle || theme().handleWidth() == 0)
        return false;

    // weren't previously rendered...
    renderHandles();

    m_use_handle = true;

    m_handle.show();
    m_handle.showSubwindows(); // shows grips

    m_window.resize(m_window.width(), m_window.height() + m_handle.height() +
                    m_handle.borderWidth());
    return true;
}

bool FbWinFrame::hideAllDecorations() {
    bool changed = false;
    changed |= hideHandle();
    changed |= hideTitlebar();
    // resize done by hide*
    reconfigure();

    return changed;
}

bool FbWinFrame::showAllDecorations() {
    bool changed = false;
    if (!m_use_handle)
        changed |= showHandle();
    if (!m_use_titlebar)
        changed |= showTitlebar();
    // resize shouldn't be necessary
    reconfigure();

    return changed;
}

/**
   Set new event handler for the frame's windows
*/
void FbWinFrame::setEventHandler(FbTk::EventHandler &evh) {

    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.add(evh, m_label);
    evm.add(evh, m_titlebar);
    evm.add(evh, m_handle);
    evm.add(evh, m_grip_right);
    evm.add(evh, m_grip_left);
    evm.add(evh, m_window);
    evm.add(evh, m_clientarea);
}

/**
   remove event handler from windows
*/
void FbWinFrame::removeEventHandler() {
    FbTk::EventManager &evm = *FbTk::EventManager::instance();
    evm.remove(m_label);
    evm.remove(m_titlebar);
    evm.remove(m_handle);
    evm.remove(m_grip_right);
    evm.remove(m_grip_left);
    evm.remove(m_window);
    evm.remove(m_clientarea);
}

void FbWinFrame::buttonPressEvent(XButtonEvent &event) {
    // we can ignore which window the event was generated for
    LabelList::iterator btn_it = m_labelbuttons.begin();
    LabelList::iterator btn_it_end = m_labelbuttons.end();
    for (; btn_it != btn_it_end; ++btn_it) {
        if ((*btn_it)->window() == event.window) {
            (*btn_it)->buttonPressEvent(event);
            break;
        }
    }
    if (event.window == m_grip_right.window() ||
        event.window == m_grip_left.window() ||
        event.window == m_clientarea.window() ||
        event.window == m_window.window())
        return;
    // we handle only buttons 0 to 5
    if (event.button > 5 || event.button < 1)
        return;

    if (*m_commands[event.button - 1].click_pressed)
        m_commands[event.button - 1].click_pressed->execute();
}

void FbWinFrame::buttonReleaseEvent(XButtonEvent &event) {
    // we can ignore which window the event was generated for
    
    LabelList::iterator button_it = find_if(m_labelbuttons.begin(),
                                            m_labelbuttons.end(),
                                            CompareWindow(&FbTk::Button::window,
                                                          event.window));
    if (button_it != m_labelbuttons.end())
        (*button_it)->buttonReleaseEvent(event);

    if (event.window == m_grip_right.window() ||
        event.window == m_grip_left.window() ||
        event.window == m_clientarea.window() ||
        event.window == m_handle.window() ||
        event.window == m_window.window())
        return;

    if (event.button < 1 || event.button > 5)
        return;

    static int last_release_time = 0;
    bool double_click = (event.time - last_release_time <= m_double_click_time);
    last_release_time = event.time;
    int real_button = event.button - 1;
    
    if (double_click && *m_commands[real_button].double_click)
        m_commands[real_button].double_click->execute();
    else if (*m_commands[real_button].click)
        m_commands[real_button].click->execute();

}

void FbWinFrame::exposeEvent(XExposeEvent &event) {
    if (m_titlebar == event.window) {
        m_titlebar.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_label == event.window) {
        m_label.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_handle == event.window) {
        m_handle.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_grip_left == event.window) {
        m_grip_left.clearArea(event.x, event.y, event.width, event.height);
    } else if (m_grip_right == event.window) {
        m_grip_right.clearArea(event.x, event.y, event.width, event.height);
    } else {
        // create compare function
        // that we should use with find_if        
        FbTk::CompareEqual_base<FbTk::FbWindow, Window> compare(&FbTk::FbWindow::window,
                                                                event.window);

        LabelList::iterator btn_it = find_if(m_labelbuttons.begin(),
                                             m_labelbuttons.end(),
                                             compare);
        if (btn_it != m_labelbuttons.end()) {
            (*btn_it)->exposeEvent(event);
            return;
        }

        ButtonList::iterator it = find_if(m_buttons_left.begin(),
                                          m_buttons_left.end(),
                                          compare);
        if (it != m_buttons_left.end()) {
            (*it)->exposeEvent(event);
            return;
        }

        it = find_if(m_buttons_right.begin(),
                     m_buttons_right.end(),
                     compare);

        if (it != m_buttons_right.end())
            (*it)->exposeEvent(event);
    }

}

void FbWinFrame::handleEvent(XEvent &event) {
    if (event.type == ConfigureNotify && event.xconfigure.window == window().window())
        configureNotifyEvent(event.xconfigure);
}

void FbWinFrame::configureNotifyEvent(XConfigureEvent &event) {
    resize(event.width, event.height);
}

void FbWinFrame::reconfigure() {
    if (m_labelbuttons.empty())
        return;

    m_bevel = theme().bevelWidth();
    // reconfigure can't set borderwidth, as it doesn't know
    // if it's meant to be borderless or not

    unsigned int orig_handle_h = handle().height();
    if (m_use_handle && orig_handle_h != theme().handleWidth())
        m_window.resize(m_window.width(), m_window.height() -
                        orig_handle_h + theme().handleWidth());

    handle().resize(handle().width(), 
                    theme().handleWidth());
    gripLeft().resize(buttonHeight(), 
                      theme().handleWidth());
    gripRight().resize(gripLeft().width(), 
                       gripLeft().height());

    // align titlebar and render it
    if (m_use_titlebar) {
        reconfigureTitlebar();
        m_titlebar.raise();
    } else 
        m_titlebar.lower();


    // leave client+grips alone if we're shaded (it'll get fixed when we unshade)
    if (!m_shaded) {
        int client_top = 0;
        int client_height = m_window.height();
        if (m_use_titlebar) {
            // only one borderwidth as titlebar is really at -borderwidth
            int titlebar_height = m_titlebar.height() + m_titlebar.borderWidth();
            client_top += titlebar_height;
            client_height -= titlebar_height;
        }
        
        // align handle and grips
        const int grip_height = m_handle.height();
        const int grip_width = 20; //TODO
        const int handle_bw = static_cast<signed>(m_handle.borderWidth());

        int ypos = m_window.height();

        // if the handle isn't on, it's actually below the window
        if (m_use_handle)
            ypos -= grip_height + handle_bw;

        // we do handle settings whether on or not so that if they get toggled
        // then things are ok...
        m_handle.moveResize(-handle_bw, ypos,
                            m_window.width(), grip_height);

        m_grip_left.moveResize(-handle_bw, -handle_bw,
                               grip_width, grip_height);

        m_grip_right.moveResize(m_handle.width() - grip_width - handle_bw, -handle_bw,
                                grip_width, grip_height);

        if (m_use_handle) {
            m_handle.raise();
            client_height -= m_handle.height() + m_handle.borderWidth();
        } else {
            m_handle.lower();
        }

        m_clientarea.moveResize(0, client_top,
                                m_window.width(), client_height);
    }


    // render the theme
    if (isVisible()) {
        renderAll();
        applyAll();
        clearAll();
    } else {
        m_need_render = true;
    }

    if (m_shape.get() && theme().shapePlace() == Shape::NONE  || m_disable_shape)
        m_shape.reset(0);
    else if (m_shape.get() == 0 && theme().shapePlace() != Shape::NONE)
        m_shape.reset(new Shape(window(), theme().shapePlace()));
    else if (m_shape.get())
        m_shape->setPlaces(theme().shapePlace());

    if (m_shape.get())
        m_shape->update();

    // titlebar stuff rendered already by reconftitlebar
}

void FbWinFrame::setUseShape(bool value) {
    m_disable_shape = !value;

    if (m_shape.get() && m_disable_shape)
        m_shape.reset(0);
    else if (m_shape.get() == 0 && !m_disable_shape)
        m_shape.reset(new Shape(window(), theme().shapePlace()));
}

unsigned int FbWinFrame::buttonHeight() const {
    return m_titlebar.height() - m_bevel*2;
}

//--------------------- private area

/**
   aligns and redraws title
*/
void FbWinFrame::redrawTitlebar() {
    if (!m_use_titlebar || m_labelbuttons.empty())
        return;

    int focus_button_min_percent = Fluxbox::instance()->getFocusedTabMinWidth();
    int button_count = m_labelbuttons.size();
    int label_width = label().width();

    /* force sane value */
    if (focus_button_min_percent > 90) 
        focus_button_min_percent = 90;
    if (focus_button_min_percent < 1) 
        focus_button_min_percent = 1;

    int focus_button_width, unfocus_button_width;
    if (100 < (focus_button_min_percent * button_count)) {
        focus_button_width = label_width * focus_button_min_percent / 100;
        if (button_count > 1) {
            unfocus_button_width = label_width *
                (100 - focus_button_min_percent) / (100 * (button_count - 1));
        } else {
            /* should never happen */
            unfocus_button_width = 0;
        }
    } else {
        focus_button_width = label_width / button_count;
        unfocus_button_width = focus_button_width;
    }

    int rounding_error = label_width - focus_button_width -
        ((button_count - 1) * unfocus_button_width);

    //!! TODO: bevel
    //int border_width = m_labelbuttons.front()->window().borderWidth();
    int border_width =  m_labelbuttons.empty() ? 0 : m_labelbuttons.front()->borderWidth();

    LabelList::iterator btn_it = m_labelbuttons.begin();
    LabelList::iterator btn_it_end = m_labelbuttons.end();
    int extra = 0, dx = 0;
    for (unsigned int last_x = 0;
         btn_it != btn_it_end; 
         ++btn_it, last_x += dx) {

        if (rounding_error != 0) {
            extra = 1;
            --rounding_error;
        } else
            extra = 0;

        if (currentLabel() == *btn_it) {
            dx = focus_button_width;
        } else {
            dx = unfocus_button_width;
        }
        dx += border_width + extra;

        (*btn_it)->moveResize(last_x - border_width, - border_width,
                              dx - border_width, 
                              label().height() + border_width);

    }

    if (isVisible()) {
        for_each(m_labelbuttons.begin(),
                 m_labelbuttons.end(),
                 mem_fun(&FbTk::TextButton::clear));

        m_label.clear();
        m_titlebar.clear();
    }
}

/**
   Align buttons with title text window
*/
void FbWinFrame::reconfigureTitlebar() {
    if (!m_use_titlebar)
        return;

    int orig_height = m_titlebar.height();
    // resize titlebar to window size with font height
    int title_height = m_theme.font().height() == 0 ? 16 : 
        m_theme.font().height() + m_bevel*2 + 2;
    if (m_theme.titleHeight() != 0)
        title_height = m_theme.titleHeight();

    // if the titlebar grows in size, make sure the whole window does too
    if (orig_height != title_height) 
        m_window.resize(m_window.width(), m_window.height()-orig_height+title_height);
    m_titlebar.moveResize(-m_titlebar.borderWidth(), -m_titlebar.borderWidth(),
                          m_window.width(), title_height);

    // draw left buttons first
    unsigned int next_x = m_bevel; 
    unsigned int button_size = buttonHeight();
    m_button_size = button_size;
    for (size_t i=0; i < m_buttons_left.size(); i++, next_x += button_size + m_bevel) {
        m_buttons_left[i]->moveResize(next_x, m_bevel, 
                                      button_size, button_size);
    }
    
    next_x += m_bevel;

    // space left on titlebar between left and right buttons
    unsigned int space_left = m_titlebar.width() - next_x;
    if (!m_buttons_right.empty()) 
        space_left -= m_buttons_right.size() * (button_size + m_bevel);

    space_left -= m_bevel;
	
    m_label.moveResize(next_x, m_bevel,
                       space_left, button_size);

    next_x += m_label.width() + m_bevel;;

    // finaly set new buttons to the right
    for (size_t i=0; i < m_buttons_right.size(); 
         ++i, next_x += button_size + m_bevel) {
        m_buttons_right[i]->moveResize(next_x, m_bevel,
                                       button_size, button_size);
    }

    m_titlebar.raise(); // always on top
}

void FbWinFrame::renderAll() {
    m_need_render = false;

    renderTitlebar();
    renderHandles();
    renderLabelButtons();
}

void FbWinFrame::applyAll() {
    applyTitlebar();
    applyHandles();
    applyLabelButtons();
}

void FbWinFrame::renderTitlebar() {
    if (!m_use_titlebar)
        return;

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    // render pixmaps
    render(m_theme.titleFocusTexture(), m_title_focused_color, 
           m_title_focused_pm,
           m_titlebar.width(), m_titlebar.height());

    render(m_theme.titleUnfocusTexture(), m_title_unfocused_color, 
           m_title_unfocused_pm,
           m_titlebar.width(), m_titlebar.height());

    render(m_theme.labelFocusTexture(), m_label_focused_color, 
           m_label_focused_pm,
           m_label.width(), m_label.height());
		

    render(m_theme.labelUnfocusTexture(), m_label_unfocused_color, 
           m_label_unfocused_pm,
           m_label.width(), m_label.height());

    renderButtons();
}

void FbWinFrame::applyTitlebar() {

    // set up pixmaps for titlebar windows
    Pixmap label_pm = None;
    Pixmap title_pm = None;    
    FbTk::Color label_color;
    FbTk::Color title_color;
    getCurrentFocusPixmap(label_pm, title_pm,
                          label_color, title_color);

    unsigned char alpha = (m_focused?theme().focusedAlpha():theme().unfocusedAlpha());
    m_titlebar.setAlpha(alpha);
    m_label.setAlpha(alpha);

    if (label_pm != 0)
        m_label.setBackgroundPixmap(label_pm);
    else            
        m_label.setBackgroundColor(label_color);
    
    if (title_pm != 0)
        m_titlebar.setBackgroundPixmap(title_pm);
    else
        m_titlebar.setBackgroundColor(title_color);

    applyButtons();
}


void FbWinFrame::renderHandles() {
    if (!m_use_handle)
        return;

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    render(m_theme.handleFocusTexture(), m_handle_focused_color, 
           m_handle_focused_pm,
           m_handle.width(), m_handle.height());
	
    render(m_theme.handleUnfocusTexture(), m_handle_unfocused_color, 
           m_handle_unfocused_pm,
           m_handle.width(), m_handle.height());

    render(m_theme.gripFocusTexture(), m_grip_focused_color, m_grip_focused_pm,
           m_grip_left.width(), m_grip_left.height());

    render(m_theme.gripUnfocusTexture(), m_grip_unfocused_color, 
           m_grip_unfocused_pm,
           m_grip_left.width(), m_grip_left.height());

}

void FbWinFrame::applyHandles() {

    unsigned char alpha = (m_focused?theme().focusedAlpha():theme().unfocusedAlpha());
    m_handle.setAlpha(alpha);
    m_grip_left.setAlpha(alpha);
    m_grip_right.setAlpha(alpha);

    if (m_focused) {

        if (m_handle_focused_pm) {
            m_handle.setBackgroundPixmap(m_handle_focused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_focused_color);
        }

        if (m_grip_focused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_focused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_focused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_focused_color);
            m_grip_right.setBackgroundColor(m_grip_focused_color);
        }                

    } else {

        if (m_handle_unfocused_pm) {
            m_handle.setBackgroundPixmap(m_handle_unfocused_pm);
        } else {
            m_handle.setBackgroundColor(m_handle_unfocused_color);
        }                

        if (m_grip_unfocused_pm) {
            m_grip_left.setBackgroundPixmap(m_grip_unfocused_pm);
            m_grip_right.setBackgroundPixmap(m_grip_unfocused_pm);
        } else {
            m_grip_left.setBackgroundColor(m_grip_unfocused_color);
            m_grip_right.setBackgroundColor(m_grip_unfocused_color);
        }                
    }

}

void FbWinFrame::renderButtons() {

    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    render(m_theme.buttonFocusTexture(), m_button_color, 
           m_button_pm,
           m_button_size, m_button_size);

    render(m_theme.buttonUnfocusTexture(), m_button_unfocused_color, 
           m_button_unfocused_pm,
           m_button_size, m_button_size);
		
    render(m_theme.buttonPressedTexture(), m_button_pressed_color, 
           m_button_pressed_pm,
           m_button_size, m_button_size);
}

void FbWinFrame::applyButtons() {
    // setup left and right buttons
    for (size_t i=0; i < m_buttons_left.size(); ++i)
        applyButton(*m_buttons_left[i]);

    for (size_t i=0; i < m_buttons_right.size(); ++i)
        applyButton(*m_buttons_right[i]);
}

void FbWinFrame::init() {

    if (FbTk::Transparent::haveComposite()) {
        if (m_focused)
            m_window.setOpaque(theme().focusedAlpha());
        else
            m_window.setOpaque(theme().unfocusedAlpha());
    }

    if (theme().handleWidth() == 0)
        m_use_handle = false;

    m_disable_shape = false;

    m_current_label = 0; // no focused button at first

    m_handle.showSubwindows();

    // clear pixmaps
    m_title_focused_pm = m_title_unfocused_pm = 0;
    m_label_focused_pm = m_label_unfocused_pm = 0;
    m_labelbutton_focused_pm = m_labelbutton_unfocused_pm = m_labelbutton_active_pm = 0;
    m_handle_focused_pm = m_handle_unfocused_pm = 0;
    m_button_pm = m_button_unfocused_pm = m_button_pressed_pm = 0;
    m_grip_unfocused_pm = m_grip_focused_pm = 0;

    m_double_click_time = 200;
    m_button_size = 26;

    m_clientarea.setBorderWidth(0);
    m_shaded = false;
    m_label.show();

    showHandle();
    showTitlebar();

    // Note: we don't show clientarea yet

    setEventHandler(*this);
}

/**
   Setups upp background, pressed pixmap/color of the button to current theme
*/
void FbWinFrame::applyButton(FbTk::Button &btn) {
    if (m_button_pressed_pm)
        btn.setPressedPixmap(m_button_pressed_pm);
    else
        btn.setPressedColor(m_button_pressed_color);

    if (focused()) { // focused
        btn.setAlpha(theme().focusedAlpha());

        btn.setGC(m_theme.buttonPicFocusGC());
        if (m_button_pm)
            btn.setBackgroundPixmap(m_button_pm);
        else
            btn.setBackgroundColor(m_button_color);
    } else { // unfocused
        btn.setAlpha(theme().unfocusedAlpha());

        btn.setGC(m_theme.buttonPicUnfocusGC());
        if (m_button_unfocused_pm)
            btn.setBackgroundPixmap(m_button_unfocused_pm);
        else
            btn.setBackgroundColor(m_button_unfocused_color);
    }

}

void FbWinFrame::render(const FbTk::Texture &tex, FbTk::Color &col, Pixmap &pm,
                        unsigned int w, unsigned int h) {

    Pixmap tmp = pm;
    if (!tex.usePixmap()) {
        pm = None;
        col = tex.color();
    } else {
        pm = m_imagectrl.renderImage(w, h, tex);
    }

    if (tmp) 
        m_imagectrl.removeImage(tmp);

}

void FbWinFrame::getCurrentFocusPixmap(Pixmap &label_pm, Pixmap &title_pm,
                                       FbTk::Color &label_color, FbTk::Color &title_color) {
    if (m_focused) {
        if (m_labelbutton_focused_pm != 0)
            label_pm = m_labelbutton_focused_pm;
        else
            label_color = m_labelbutton_focused_color;
    
        if (m_title_focused_pm != 0)
            title_pm = m_title_focused_pm;
        else
            title_color = m_title_focused_color;
     
    } else {
        getActiveLabelPixmap(label_pm, title_pm,
                             label_color, title_color);
    }

}

// only called if not focused
void FbWinFrame::getActiveLabelPixmap(Pixmap &label_pm, Pixmap &title_pm,
                                  FbTk::Color &label_color, 
                                  FbTk::Color &title_color) {

    if (m_labelbutton_active_pm != 0)
        label_pm = m_labelbutton_active_pm;            
    else
        label_color = m_labelbutton_active_color;

    if (m_title_unfocused_pm != 0)
        title_pm  = m_title_unfocused_pm;
    else
        title_color = m_title_unfocused_color;
}

void FbWinFrame::renderLabelButtons() {
    if (!isVisible()) {
        m_need_render = true;
        return;
    }

    render(m_theme.labelFocusTexture(), m_labelbutton_focused_color, 
           m_labelbutton_focused_pm,
           m_label.width(), m_label.height());
		

    render(m_theme.labelUnfocusTexture(), m_labelbutton_unfocused_color, 
           m_labelbutton_unfocused_pm,
           m_label.width(), m_label.height());

    render(m_theme.labelActiveTexture(), m_labelbutton_active_color, 
           m_labelbutton_active_pm,
           m_label.width(), m_label.height());

}

void FbWinFrame::applyLabelButtons() {

    LabelList::iterator btn_it = m_labelbuttons.begin();
    LabelList::iterator btn_it_end = m_labelbuttons.end();        
    for (; btn_it != btn_it_end; ++btn_it) {
        if (*btn_it == m_current_label) {
            if (m_focused)
                applyFocusLabel(**btn_it);
            else
                applyActiveLabel(**btn_it);
        } else
            applyUnfocusLabel(**btn_it);
    }
}

void FbWinFrame::setBorderWidth(unsigned int border_width) {
    int bw_changes = 0;
    // we need to change the size of the window 
    // if the border width changes...
    if (m_use_titlebar) 
        bw_changes += static_cast<signed>(border_width - titlebar().borderWidth());
    if (m_use_handle) 
        bw_changes += static_cast<signed>(border_width - handle().borderWidth());

    window().setBorderWidth(border_width);
    window().setBorderColor(theme().border().color());

    titlebar().setBorderWidth(border_width);
    titlebar().setBorderColor(theme().border().color());

    handle().setBorderWidth(border_width);
    handle().setBorderColor(theme().border().color());

    gripLeft().setBorderWidth(border_width);
    gripLeft().setBorderColor(theme().border().color());

    gripRight().setBorderWidth(border_width);
    gripRight().setBorderColor(theme().border().color());

    if (bw_changes != 0)
        resize(width(), height() + bw_changes);
}

void FbWinFrame::applyFocusLabel(FbTk::TextButton &button) {

    button.setGC(theme().labelTextFocusGC());
    button.setJustify(theme().justify());
    button.setBorderWidth(1);
    button.setAlpha(m_focused?theme().focusedAlpha():theme().unfocusedAlpha());

    if (m_labelbutton_focused_pm != 0) {
        button.setBackgroundPixmap(m_labelbutton_focused_pm);
    } else
        button.setBackgroundColor(m_labelbutton_focused_color);

}

void FbWinFrame::applyActiveLabel(FbTk::TextButton &button) {

    button.setGC(theme().labelTextActiveGC());
    button.setJustify(theme().justify());
    button.setBorderWidth(1);
    button.setAlpha(m_focused?theme().focusedAlpha():theme().unfocusedAlpha());

    if (m_labelbutton_active_pm != 0) {
            button.setBackgroundPixmap(m_labelbutton_active_pm);
    } else
        button.setBackgroundColor(m_labelbutton_active_color);

}

void FbWinFrame::applyUnfocusLabel(FbTk::TextButton &button) {

    button.setGC(theme().labelTextUnfocusGC());
    button.setJustify(theme().justify());
    button.setBorderWidth(1);
    button.setAlpha(m_focused?theme().focusedAlpha():theme().unfocusedAlpha());

    if (m_labelbutton_unfocused_pm != 0) {
        button.setBackgroundPixmap(m_labelbutton_unfocused_pm);
    } else
        button.setBackgroundColor(m_labelbutton_unfocused_color);

}

namespace {
class IgnoreEvent {
public:
    typedef void result_type;
    typedef Window argument_type;
    explicit IgnoreEvent(long eventmask):
        m_display(FbTk::App::instance()->display()),
        m_event_mask(eventmask) {
    }

    void operator()(Window win) const {
        static XEvent event;
        while (XCheckWindowEvent(m_display, win, m_event_mask, &event))
            continue;
    }
private:
    Display *m_display;
    long m_event_mask;
};
}

// this function translates its arguments according to win_gravity
// if win_gravity is negative, it does an inverse translation
// This function should be used when a window is mapped/unmapped/pos configured
void FbWinFrame::gravityTranslate(int &x, int &y, int win_gravity, bool move_frame) {
    gravityTranslate(x, y, m_window.width(), m_window.height(), win_gravity, move_frame);
}
//use width and height given instead of the real values, allows figuring out where to place a window before doing a moveResize
void FbWinFrame::gravityTranslate(int &x, int &y, unsigned int width, unsigned int height, int win_gravity, bool move_frame) {
    bool invert = false;
    if (win_gravity < 0) {
        invert = true;
        win_gravity = -win_gravity; // make +ve
    }

    /* Ok, so, gravity says which point of the frame is put where the 
     * corresponding bit of window would have been
     * Thus, x,y always refers to where top left of the WINDOW would be placed
     * but given that we're wrapping it in a frame, we actually place
     * it so that the given reference point is in the same spot as the
     * window's reference point would have been.
     * i.e. east gravity says that the centre of the right hand side of the 
     * frame is placed where the centre of the rhs of the window would
     * have been if there was no frame.
     * Hope that makes enough sense.
     *
     * If you get confused with the calculations, draw a picture.
     *
     */

    // We calculate offsets based on the gravity and frame aspects
    // and at the end apply those offsets +ve or -ve depending on 'invert'

    int x_offset = 0;
    int y_offset = 0;

    /* win_gravity:    placed at the reference point
     * StaticGravity    the left top corner of the client window
     * NorthWestGravity    the left top corner of the frame window
     * NorthGravity    the center of the frame window's top side
     * NorthEastGravity    the right top corner of the frame window
     * EastGravity    the center of the frame window's right side
     * SouthEastGravity    the right bottom corner of the frame window
     * SouthGravity    the center of the frame window's bottom side
     * SouthWestGravity    the left bottom corner of the frame window
     * WestGravity    the center of the frame window's left side
     * CenterGravity    the center of the frame window
     */ 
    

    //vertical offset
    //North Gravities don't require a vertical offset
    //South Gravities
    if (win_gravity==SouthWestGravity || win_gravity==SouthGravity ||
        win_gravity==SouthEastGravity) {
        //We start on the frame so going the full height would take us one pixel past the edge of the frame
        y_offset-=height-1;
    }

    //vertical centering
    if (win_gravity==WestGravity || win_gravity==CenterGravity ||
        win_gravity==EastGravity) {
        y_offset-=height/2;
    }

    //horizontal offset
    //West Gravities don't require a horizontal offset
    //East Gravities
    if (win_gravity==NorthEastGravity || win_gravity==EastGravity ||
        win_gravity==SouthEastGravity ) {
        //Starting on the frame so offset of one width would end up one pixel beyond the border
        x_offset-=width-1;
    }
    //horizontal centering
    if (win_gravity==NorthGravity || win_gravity==CenterGravity ||
        win_gravity==SouthGravity ) {
        x_offset-=width/2;
    }

    if( win_gravity==StaticGravity ) {
        if (m_use_titlebar)
            y_offset -= m_titlebar.height() + m_titlebar.borderWidth();
        x_offset -= m_window.borderWidth();
        y_offset -= m_window.borderWidth();
    }


    if (invert) {
        x_offset = -x_offset;
        y_offset = -y_offset;
    }

    x += x_offset;
    y += y_offset;
    if (move_frame && (x_offset != 0 || y_offset != 0)) {
        move(x, y);
    }
}
