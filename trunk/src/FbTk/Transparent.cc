// Transparent.cc for FbTk - Fluxbox Toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen(at)users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Transparent.cc,v 1.1 2003/04/20 13:27:16 fluxgen Exp $

#include "Transparent.hh"
#include "App.hh"

// #ifdef HAVE_XRENDER
#include <X11/extensions/Xrender.h>
// #endif // HAVE_XRENDER

#include <iostream>
using namespace std;

namespace {

Picture createAlphaPic(Window drawable, unsigned char alpha) {
    Display *disp = FbTk::App::instance()->display();

    // try to find a specific render format
    XRenderPictFormat pic_format;
    pic_format.type  = PictTypeDirect;
    pic_format.depth = 8; // alpha with bit depth 8
    pic_format.direct.alphaMask = 0xff;    
    XRenderPictFormat *format = XRenderFindFormat(disp, PictFormatType |
                                                  PictFormatDepth | PictFormatAlphaMask,
                                                  &pic_format, 0);
    if (format == 0) {
        cerr<<"Warning! FbTk::Transparent:  Failed to find valid format for alpha."<<endl;
        return 0;
    }

    // create one pixel pixmap with depth 8 for alpha
    Pixmap alpha_pm = XCreatePixmap(disp, drawable,
                                    1, 1, 8);
    if (alpha_pm == 0) {
        cerr<<"Warning! FbTk::Transparent: Failed to create alpha pixmap."<<endl;
        return 0;
    }

    // create picture with alpha_pm as repeated background
    XRenderPictureAttributes attr;
    attr.repeat = True; // small bitmap repeated
    Picture alpha_pic = XRenderCreatePicture(disp, alpha_pm,
                                             format, CPRepeat, &attr);
    if (alpha_pic == 0) {
        XFreePixmap(disp, alpha_pm);
        cerr<<"Warning! FbTk::Transparent: Failed to create alpha picture."<<endl;
        return 0;
    }

    // finaly set alpha and fill with it
    XRenderColor color;
    // calculate alpha percent and then scale it to short
    color.red = 0xFF;
    color.blue = 0xFF;
    color.green = 0xFF;
    color.alpha = ((unsigned short) (255 * alpha) << 8);

    XRenderFillRectangle(disp, PictOpSrc, alpha_pic, &color,
                         0, 0, 1, 1);

    XFreePixmap(disp, alpha_pm);

    return alpha_pic;
}

};

namespace FbTk {


Transparent::Transparent(Drawable src, Drawable dest, unsigned char alpha, int screen_num):
    m_alpha_pic(0), m_src_pic(0), m_dest_pic(0),
    m_source(src), m_dest(dest), m_alpha(alpha) {

    allocAlpha(m_alpha);

    Display *disp = FbTk::App::instance()->display();

    XRenderPictFormat *format = 
        XRenderFindVisualFormat(disp, 
                                DefaultVisual(disp, screen_num));


    if (src != 0 && format != 0) {
        m_src_pic = XRenderCreatePicture(disp, src, format, 
                                         0, 0);
    }

    if (dest != 0 && format != 0) {
        m_dest_pic = XRenderCreatePicture(disp, dest, format,
                                          0, 0);
    }

}

Transparent::~Transparent() {
    if (m_alpha_pic != 0)
        freeAlpha();

    Display *disp = FbTk::App::instance()->display();

    if (m_dest_pic != 0)
        XRenderFreePicture(disp, m_dest_pic);

    if (m_src_pic != 0)
        XRenderFreePicture(disp, m_src_pic);
}

void Transparent::setAlpha(unsigned char alpha) {
    if (m_source == 0)
        return;

    freeAlpha();
    allocAlpha(alpha);
}

void Transparent::setDest(Drawable dest, int screen_num) {
    if (m_dest == dest)
        return;

    Display *disp = FbTk::App::instance()->display();

    if (m_dest_pic != 0) {
        XRenderFreePicture(disp, m_dest_pic);
        m_dest_pic = 0;
    }
    // create new dest pic if we have a valid dest drawable
    if (dest != 0) {

        XRenderPictFormat *format = 
            XRenderFindVisualFormat(disp, 
                                    DefaultVisual(disp, screen_num));
        if (format == 0)
            cerr<<"Warning! FbTk::Transparent: Failed to find format for screen("<<screen_num<<")"<<endl;
        m_dest_pic = XRenderCreatePicture(disp, dest, format, 0, 0);
       

    }
    m_dest = dest;

}

void Transparent::setSource(Drawable source, int screen_num) {
    if (m_source == source)
        return;

    if (m_alpha_pic != 0)
        freeAlpha();

    Display *disp = FbTk::App::instance()->display();   

    if (m_src_pic != 0) {
        XRenderFreePicture(disp, m_src_pic);
        m_src_pic = 0;
    }

    m_source = source;
    allocAlpha(m_alpha);

    // create new source pic if we have a valid source drawable
    if (m_source != 0) {

        XRenderPictFormat *format = 
            XRenderFindVisualFormat(disp, 
                                    DefaultVisual(disp, screen_num));
        if (format == 0)
            cerr<<"Warning! FbTk::Transparent: Failed to find format for screen("<<screen_num<<")"<<endl;
        m_src_pic = XRenderCreatePicture(disp, m_source, format, 
                                         0, 0);    
    }
}

void Transparent::render(int src_x, int src_y,
                         int dest_x, int dest_y,
                         unsigned int width, unsigned int height) const {
    if (m_src_pic == 0 || m_dest_pic == 0 ||
        m_alpha_pic  == 0)
        return;
    // render src+alpha to dest picture
    XRenderComposite(FbTk::App::instance()->display(), 
                     PictOpOver, 
                     m_src_pic,
                     m_alpha_pic, 
                     m_dest_pic, 
                     src_x, src_y,
                     0, 0, 
                     dest_x, dest_y,
                     width, height);


}

void Transparent::allocAlpha(unsigned char alpha) {
    if (m_source == 0)
        return;
    if (m_alpha_pic != 0)
        freeAlpha();

    m_alpha_pic = createAlphaPic(m_source, alpha);
    m_alpha = alpha;
}

void Transparent::freeAlpha() {
    XRenderFreePicture(FbTk::App::instance()->display(), m_alpha_pic);
    m_alpha_pic = 0;
    m_alpha = 255;
}

}; // end namespace FbTk


 
