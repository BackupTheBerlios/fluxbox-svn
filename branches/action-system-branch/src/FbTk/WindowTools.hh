// WindowTools.hh for fluxbox 
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

// $Id: WindowTools.hh,v 1.1.2.1 2003/10/28 21:34:52 rathnor Exp $


#ifndef FBTK_WINDOWTOOLS_HH
#define FBTK_WINDOWTOOLS_HH

#include <X11/Xlib.h>

namespace FbTk {

class FbWindow;

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
 */

class WindowTransform {
public:
    WindowTransform();
    virtual ~WindowTransform();

    void useOutline(bool outline);

    void setDrawable(Drawable d, GC gc);
    void resize(unsigned int new_width, unsigned int new_height);
    void move(int new_x, int new_y);
    void moveResize(int new_x, int new_y, unsigned int new_width, unsigned int new_height);

    // These functions can be overridden to allow special resize
    // methods. By default it just calls resize/move on the fbwindow.
    // These aren't used for outline operations.
    virtual void resizeWindow(unsigned int width, unsigned int height);
    virtual void moveWindow(int x, int y);
    virtual void moveResizeWindow(int x, int y, unsigned int width, unsigned int height);

    // saves doing setDrawable
    void setWindow(FbWindow *win);

    // The outline operation specifically requires the rectangle
    // to be cleared when windows around it might be changed.
    // Thus, if external changes are necessary while moving, safePause
    // must be used, or there could be rectangle left floating.
    void safePause();
    void resume();

    // makes the window active/inactive
    void activate();

    // operation finished, finalise everything
    void commit();

    // stop and revert
    void cancel();

    inline int currX() const { return x; }
    inline int currY() const { return y; }
    inline unsigned int currWidth() const { return width; }
    inline unsigned int currHeight() const { return height; }
    inline unsigned int origWidth() const { return orig_width; }
    inline unsigned int origHeight() const { return orig_height; }


protected:
    inline FbWindow *getWindow() { return m_window; }
    inline const FbWindow *getWindow() const { return m_window; }

private:

    void invertOutline();

    int x, y;
    unsigned int width, height;

    unsigned int borderWidth;

    int orig_x, orig_y;
    unsigned int orig_width, orig_height;
    bool orig_pos, orig_size;

    // a window can be paused and/or active
    // (but must be active to be paused)

    bool paused, active, use_outline;
    Drawable m_drawable;
    GC m_gc;

    FbWindow *m_window;
};

}; // end namespace FbTk


#endif // FBTK_WINDOWTOOLS_HH
