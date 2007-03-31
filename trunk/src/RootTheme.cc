// RootTheme.cc
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#include "RootTheme.hh"

#include "FbRootWindow.hh"
#include "FbCommands.hh"
#include "Screen.hh"

#include "FbTk/App.hh"
#include "FbTk/Font.hh"
#include "FbTk/ImageControl.hh"
#include "FbTk/Resource.hh"
#include "FbTk/FileUtil.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/TextureRender.hh"
#include "FbTk/I18n.hh"

#include <X11/Xatom.h>
#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>

using std::cerr;
using std::endl;
using std::string;

class BackgroundItem: public FbTk::ThemeItem<FbTk::Texture> {
public:
    BackgroundItem(FbTk::Theme &tm, const std::string &name, const std::string &altname):
        FbTk::ThemeItem<FbTk::Texture>(tm, name, altname),
        m_changed(false), m_loaded(false) {
        
    }

    void load(const std::string *o_name = 0, const std::string *o_altname = 0) {
        const string &m_name = (o_name == 0) ? name() : *o_name;
        const string &m_altname = (o_altname == 0) ? altName() : *o_altname;

        // if we got this far, then the background was loaded
        m_loaded = true;

        // create subnames
        string color_name(FbTk::ThemeManager::instance().
                          resourceValue(m_name + ".color", m_altname + ".Color"));
        string colorto_name(FbTk::ThemeManager::instance().
                            resourceValue(m_name + ".colorTo", m_altname + ".ColorTo"));
        string pixmap_name(FbTk::ThemeManager::instance().
                           resourceValue(m_name + ".pixmap", m_altname + ".Pixmap"));
        string mod_x(FbTk::ThemeManager::instance().
                     resourceValue(m_name + ".modX", m_altname + ".ModX"));
        string mod_y(FbTk::ThemeManager::instance().
                     resourceValue(m_name + ".modY", m_altname + ".ModY"));

        // validate mod_x and mod_y
        if (mod_x.length() > 2)
            mod_x.erase(2,mod_x.length()); // shouldn't be longer than 2 digits
        if (mod_y.length() > 2)
            mod_y.erase(2,mod_y.length()); // ditto
        // should be integers
        if (!mod_x.length() || mod_x[0] < '0' || mod_x[0] > '9' ||
            (mod_x.length() == 2 && (mod_x[1] < '0' || mod_x[1] > '9')))
            mod_x = "1";
        if (!mod_y.length() || mod_y[0] < '0' || mod_y[0] > '9' ||
            (mod_y.length() == 2 && (mod_y[1] < '0' || mod_y[1] > '9')))
            mod_y = "1";

        // remove whitespace from filename
        FbTk::StringUtil::removeFirstWhitespace(pixmap_name);
        FbTk::StringUtil::removeTrailingWhitespace(pixmap_name);

        // check if the background has been changed
        if (mod_x != m_mod_x || mod_y != m_mod_y || pixmap_name != m_filename ||
                color_name != m_color || colorto_name != m_color_to) {
            m_changed = true;
            m_mod_x = mod_x;
            m_mod_y = mod_y;
            m_filename = pixmap_name;

            // these aren't quite right because of defaults set below
            m_color = color_name;
            m_color_to = colorto_name;
        }

        // set default value if we failed to load colors
        if (!(*this)->color().setFromString(color_name.c_str(),
                                            theme().screenNum()))
            (*this)->color().setFromString("darkgray", theme().screenNum());

        if (!(*this)->colorTo().setFromString(colorto_name.c_str(),
                                              theme().screenNum()))
            (*this)->colorTo().setFromString("white", theme().screenNum());


        if (((*this)->type() & FbTk::Texture::SOLID) != 0 && ((*this)->type() & FbTk::Texture::FLAT) == 0)
            (*this)->calcHiLoColors(theme().screenNum());

        // we dont load any pixmap, using external command to set background pixmap
        (*this)->pixmap() = 0;
    }

    void setFromString(const char *str) {
        m_options = str; // save option string
        FbTk::ThemeItem<FbTk::Texture>::setFromString(str);
    }
    const std::string &filename() const { return m_filename; }
    const std::string &options() const { return m_options; }
    const std::string &colorString() const { return m_color; }
    const std::string &colorToString() const { return m_color_to; }
    const std::string &modX() const { return m_mod_x; }
    const std::string &modY() const { return m_mod_y; }
    bool changed() const { return m_changed; }
    bool loaded() const { return m_loaded; }
    void setApplied() { m_changed = false; }
    void unsetLoaded() { m_loaded = false; }
private:
    std::string m_filename, m_options;
    std::string m_color, m_color_to;
    std::string m_mod_x, m_mod_y;
    bool m_changed, m_loaded;
};


RootTheme::RootTheme(FbTk::ImageControl &image_control):
    FbTk::Theme(image_control.screenNumber()),
    m_background(new BackgroundItem(*this, "background", "Background")),
    m_opgc(RootWindow(FbTk::App::instance()->display(), image_control.screenNumber())),
    m_image_ctrl(image_control) {

    Display *disp = FbTk::App::instance()->display();
    m_opgc.setForeground(WhitePixel(disp, screenNum())^BlackPixel(disp, screenNum()));
    m_opgc.setFunction(GXxor);
    m_opgc.setSubwindowMode(IncludeInferiors);
    m_opgc.setLineAttributes(1, LineSolid, CapNotLast, JoinMiter);
}

RootTheme::~RootTheme() {
    delete m_background;
}

bool RootTheme::fallback(FbTk::ThemeItem_base &item) {
    // if background theme item was not found in the
    // style then mark background as not loaded so
    // we can deal with it in reconfigureTheme()
    if (item.name() == "background") {
        // mark no background loaded
        m_background->unsetLoaded();
        return true;
    }
    return false;
}

void RootTheme::reconfigTheme() {
    if (!m_background->loaded())
        return;

    if (!m_background->changed())
        return;

    // style doesn't wish to change the background
    if (strstr(m_background->options().c_str(), "none") != 0)
        return;

    //
    // Else parse background from style 
    //

    m_background->setApplied();

    // handle background option in style
    std::string filename = m_background->filename();
    FbTk::StringUtil::removeTrailingWhitespace(filename);
    FbTk::StringUtil::removeFirstWhitespace(filename);
    // if background argument is a file then
    // parse image options and call image setting 
    // command specified in the resources
    filename = FbTk::StringUtil::expandFilename(filename);
    if (FbTk::FileUtil::isRegularFile(filename.c_str())) {
        // parse options
        std::string options = "-F ";
        if (strstr(m_background->options().c_str(), "tiled") != 0)
            options = "-T ";
        if (strstr(m_background->options().c_str(), "centered") != 0)
            options = "-C ";
        if (strstr(m_background->options().c_str(), "aspect") != 0)
            options = "-A ";
            
        // compose wallpaper application "fbsetbg" with argumetns
        std::string commandargs = "fbsetbg " + options + filename;

        // call command with options
        FbCommands::ExecuteCmd exec(commandargs, screenNum());
        exec.execute();

    } else if (FbTk::FileUtil::isDirectory(filename.c_str()) &&
            strstr(m_background->options().c_str(), "random") != 0) {
        std::string commandargs = "fbsetbg -R " + filename;
        FbCommands::ExecuteCmd exec(commandargs, screenNum());
        exec.execute();
    } else {
        // render normal texture with fbsetroot


        // Make sure the color strings are valid, 
        // so we dont pass any `commands` that can be executed
        bool color_valid = 
            FbTk::Color::validColorString(m_background->colorString().c_str(), 
                                          screenNum());
        bool color_to_valid = 
            FbTk::Color::validColorString(m_background->colorToString().c_str(),
                                          screenNum());

        std::string options;
        if (color_valid)
            options += "-foreground '" + m_background->colorString() + "' ";
        if (color_to_valid)
            options += "-background '" + m_background->colorToString() + "' ";

        if (strstr(m_background->options().c_str(), "mod") != 0)
            options += "-mod " + m_background->modX() + " " + m_background->modY();
        else if ((*m_background)->type() & FbTk::Texture::SOLID && color_valid)
            options += "-solid '" + m_background->colorString() + "' ";

        else if ((*m_background)->type() & FbTk::Texture::GRADIENT) {
            options += "-gradient '" + m_background->options() + "'";
        }

        std::string commandargs = "fbsetroot " + options;

        FbCommands::ExecuteCmd exec(commandargs, screenNum());
        exec.execute();
    }

}
