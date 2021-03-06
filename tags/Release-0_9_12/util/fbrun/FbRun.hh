// FbRun.hh
// Copyright (c) 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id$

#ifndef FBRUN_HH
#define FBRUN_HH

#include "FbTk/EventHandler.hh"
#include "FbTk/Font.hh"
#include "FbTk/FbWindow.hh"
#include "FbTk/TextBox.hh"
#include "FbTk/GContext.hh"
#include "FbTk/FbPixmap.hh"

#include <string>
#include <vector>

/**
   Creates and managed a run window
*/
class FbRun: public FbTk::TextBox {
public:
    FbRun(int x = 0, int y = 0, size_t width = 200);
    ~FbRun();
    void handleEvent(XEvent * const ev);
    void setTitle(const std::string &title);
    void resize(unsigned int width, unsigned int height);

    /// load and reconfigure for new font
    bool loadFont(const std::string &fontname);
    void setForegroundColor(const FbTk::Color &color);
    void setAntialias(bool val) { m_font.setAntialias(val); }
    const FbTk::Font &font() const { return m_font; }
    /// execute command and exit
    void run(const std::string &execstring);
    /// is this application done?
    bool end() const { return m_end; }
    /**
       loads history file.
       @return true on success, else false
    */
    bool loadHistory(const char *filename);
    /**
       @name events
    */
    ///@{
    void keyPressEvent(XKeyEvent &ev);
    ///@}

private:
    void nextHistoryItem();
    void prevHistoryItem();
    void drawString(int x, int y, const char *text, size_t len);
    void getSize(size_t &width, size_t &height);
    void createWindow(int x, int y, size_t width, size_t height);
    void redrawLabel();
    /// set no maximizable for this window
    void setNoMaximize();

    void insertCharacter(char key);
    void adjustStartPos();
    void adjustEndPos();
    void firstHistoryItem();
    void lastHistoryItem();
    void tabCompleteHistory();
    void tabCompleteApps();

    FbTk::Font m_font; ///< font used to draw command text
    Display *m_display;  ///< display connection
    int m_bevel;
    FbTk::GContext m_gc; ///< graphic context
    bool m_end; ///< marks when this object is done
    std::vector<std::string> m_history; ///< history list of commands
    std::string m_history_file; ///< holds filename for command history file
    size_t m_current_history_item; ///< holds current position in command history
    
    typedef std::vector<std::string> AppsContainer;
    typedef AppsContainer::iterator AppsContainerIt;
    AppsContainer m_apps; ///< holds all apps in $PATH
    size_t m_current_apps_item; ///< holds current position in apps-history
    
    Cursor m_cursor;

    FbTk::FbPixmap m_pixmap;
};

#endif // FBRUN_HH
