// WindowTools.cc for fluxbox 
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

// $Id: WindowTools.cc,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $


#include "WindowTools.hh"
#include "FbWindow.hh"
#include "App.hh"

namespace FbTk {

/**
 * Class for providing a window outline, or movement
 * on some drawable. The change method can be set to
 * either opaque, or outline.
 * Opaque means the whole window is constantly moved/resized
 * Outline means a rectangular outline of the window is
 * drawn, and only moved on a commit.
 * The operation can be cancelled, which will revert
 * to the original values. When completed, it must be committed.
 *
 * The outline mode also grabs the server to provide safe 
 * inversion of rectangles. Thus other applications will be put on hold
 * until the operation is committed.
 *
 * This API doesn't currently support moveResize operations, because
 * it doesn't need to yet - existing ops all have the user manipulate
 * either size OR position at any one time.
 */

WindowTransform::WindowTransform():
    x(0), y(0), width(0), height(0),
    paused(false), active(false),
    m_drawable(0),
    m_window(0)
{}

WindowTransform::~WindowTransform() {
    if (active)
        cancel();
}

void WindowTransform::useOutline(bool outline) {
    if (active && outline != use_outline) {
        invertOutline();
    }
    use_outline = outline;
}

void WindowTransform::setDrawable(Drawable d, GC gc) {
    if (active)
        cancel();
    m_drawable = d;
    m_gc = gc;
}

void WindowTransform::move(int new_x, int new_y) {
    if (use_outline && !paused)
        invertOutline(); // clear

    x = new_x;
    y = new_y;

    bool was_active = active;
    if (!active)
        activate();

    if (!paused) {
        if (use_outline) {
            if (was_active)
                invertOutline(); // redraw new
        } else
            moveWindow(x, y);
    }
}

void WindowTransform::resize(unsigned int new_width, unsigned int new_height) {
    if (use_outline && !paused)
        invertOutline(); // clear

    width = new_width;
    height = new_height;

    bool was_active = active;
    if (!active)
        activate();

    if (!paused) {
        if (use_outline) {
            if (was_active)
                invertOutline(); // redraw new
        } else
            resizeWindow(width, height);
    }
}

void WindowTransform::moveResize(int new_x, int new_y, unsigned int new_width, unsigned int new_height) {

    if (use_outline && !paused)
        invertOutline(); // clear

    x = new_x;
    y = new_y;
    width = new_width;
    height = new_height;

    bool was_active = active;
    if (!active)
        activate();

    if (!paused) {
        if (use_outline) {
            if (was_active)
                invertOutline(); // redraw new
        } else
            moveResizeWindow(x, y, width, height);
    }
}


// These functions can be overridden to allow special resize
// methods. By default it just calls resize/move on the fbwindow.
// These aren't used for outline operations.
void WindowTransform::resizeWindow(unsigned int width, unsigned int height) {
    if (m_window == 0)
        return;
    else
        m_window->resize(width, height);
}

void WindowTransform::moveWindow(int x, int y) {
    if (m_window == 0)
        return;
    else
        m_window->move(x, y);
}

void WindowTransform::moveResizeWindow(int x, int y, 
                                       unsigned int width, 
                                       unsigned int height) {
    if (m_window == 0)
        return;
    else
        m_window->moveResize(x, y, width, height);
}

void WindowTransform::setWindow(FbWindow *win) {
    if (win == m_window)
        return;

    bool was_active = active;
    if (was_active)
        cancel();

    m_window = win;
    if (m_window) {

        x = orig_x = m_window->x();
        y = orig_y = m_window->y();
        width = orig_width = m_window->width();
        height = orig_height = m_window->height();
        borderWidth = m_window->borderWidth();
        orig_pos = true;
        orig_size = true;

        if (was_active)
            activate();
    } else {
        orig_pos = false;
        orig_size = false;
    }
}

// The outline operation specifically requires the rectangle
// to be cleared when windows around it might be changed.
// Thus, if external changes are necessary while moving, safePause
// must be used, or there could be rectangle left floating.
void WindowTransform::safePause() {
    if (active && !paused && use_outline)
        invertOutline(); // clear

    paused = true;
}

void WindowTransform::resume() {
    // make sure there's nothing going to get in our way
    paused = false;
    if (active && use_outline) {
        XSync(FbTk::App::instance()->display(), false);
        invertOutline(); // redraw new
    }
}

void WindowTransform::activate() {
    if (active)
        return;

    active = true;
    orig_x = x;
    orig_y = y;
    orig_width = width;
    orig_height = height;
    
    orig_pos = true;
    orig_size = true;

    if (use_outline)
        invertOutline();
}

void WindowTransform::commit() {
    if (!active)
        return;

    if (use_outline && !paused)
        invertOutline(); // clear

    moveResizeWindow(x, y, width, height);

    orig_pos = false;
    orig_size = false;

    active = false;
    paused = false;
}

void WindowTransform::cancel() {
    if (!active)
        return;

    if (orig_pos && orig_size) 
        moveResizeWindow(orig_x, orig_y,
                         orig_width, orig_height);
    else if (orig_pos)
        moveWindow(orig_x, orig_y);
    else if (orig_size)
        resizeWindow(orig_width, orig_height);

    active = false;
    orig_pos = false;
    orig_size = false;
    paused = false;
}

void WindowTransform::invertOutline() {
    if (active && !paused) {
        XDrawRectangle(FbTk::App::instance()->display(),
                       m_drawable,
                       m_gc,
                       x, y,
                       width + 2*borderWidth - 1,
                       height + 2*borderWidth - 1);
    }
}


}; // end namespace FbTk


