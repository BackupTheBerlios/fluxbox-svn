// ClockTool.cc
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

// $Id: ClockTool.cc,v 1.5 2003/08/15 15:30:18 fluxgen Exp $

#include "ClockTool.hh"

#include "ToolTheme.hh"
#include "Screen.hh"

#include "FbTk/SimpleCommand.hh"
#include "FbTk/ImageControl.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <ctime>

ClockTool::ClockTool(const FbTk::FbWindow &parent,
                     ToolTheme &theme, BScreen &screen):
    ToolbarItem(ToolbarItem::FIXED),
    m_button(parent, theme.font(), ""),
    m_theme(theme),
    m_screen(screen),
    m_pixmap(0),
    m_timeformat(screen.resourceManager(), std::string("%k:%M"), 
                 screen.name() + ".strftimeFormat", screen.altName() + ".StrftimeFormat") {
    // attach signals
    theme.reconfigSig().attach(this);

    // setup timer to update the graphics each second
    timeval delay;
    delay.tv_sec = 1;
    delay.tv_usec = 0;
    m_timer.setTimeout(delay);
    FbTk::RefCount<FbTk::Command> update_graphic(new FbTk::SimpleCommand<ClockTool>(*this, 
                                                                                    &ClockTool::updateTime));
    m_timer.setCommand(update_graphic);
    m_timer.start();

    m_button.setGC(m_theme.textGC());

    update(0);
}

ClockTool::~ClockTool() {
    // remove cached pixmap
    if (m_pixmap)
        m_screen.imageControl().removeImage(m_pixmap);
}

void ClockTool::move(int x, int y) {
    m_button.move(x, y);
}

void ClockTool::resize(unsigned int width, unsigned int height) {
    m_button.resize(width, height);
    renderTheme();
}

void ClockTool::moveResize(int x, int y,
                      unsigned int width, unsigned int height) {
    m_button.moveResize(x, y, width, height);
    renderTheme();
}

void ClockTool::show() {
    m_button.show();
}

void ClockTool::hide() {
    m_button.hide();
}

void ClockTool::update(FbTk::Subject *subj) {
    updateTime();

    // + 2 to make the entire text fit inside
    std::string text;
    for (size_t i=0; i<m_button.text().size() + 2; ++i)
        text += '0';

    resize(m_theme.font().textWidth(text.c_str(), text.size()), m_button.height());
}

unsigned int ClockTool::borderWidth() const { 
    return m_button.borderWidth();
}

unsigned int ClockTool::width() const {
    return m_button.width();
}

unsigned int ClockTool::height() const {
    return m_button.height();
}

void ClockTool::updateTime() {

    // update clock
    time_t the_time = time(0);

    if (the_time != -1) {
        char time_string[255];
        struct tm *time_type = localtime(&the_time);
        if (time_type == 0)
            return;

#ifdef HAVE_STRFTIME
        if (!strftime(time_string, 255, m_timeformat->c_str(), time_type))
            return;
        m_button.setText(time_string);
#else // dont have strftime so we have to set it to hour:minut
        //        sprintf(time_string, "%d:%d", );
#endif // HAVE_STRFTIME
    }

    m_button.clear();
}

void ClockTool::renderTheme() {
    Pixmap old_pm = m_pixmap;
    if (m_theme.texture().type() == (FbTk::Texture::FLAT | FbTk::Texture::SOLID)) {
        m_pixmap = 0;
        m_button.setBackgroundColor(m_theme.texture().color());
    } else {
        m_pixmap = m_screen.imageControl().renderImage(m_button.width(), m_button.height(), m_theme.texture());
        m_button.setBackgroundPixmap(m_pixmap);
    }

    if (old_pm)
        m_screen.imageControl().removeImage(old_pm);

    m_button.setJustify(m_theme.justify());
    m_button.setBorderWidth(m_theme.border().width());
    m_button.setBorderColor(m_theme.border().color());
    m_button.clear();
}
