// XrmDatabaseHelper.hh
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

// $Id: XrmDatabaseHelper.hh,v 1.1 2003/05/18 22:06:59 fluxgen Exp $

// This is a helper for XrmDatabase
// when database goes out of scope
// the XrmDatabase variable will be destroyed.

#ifndef XRMDATABASEHELPER_HH
#define XRMDATABASEHELPER_HH

#include <X11/Xlib.h>
#include <X11/Xresource.h>

/**
	Helper class for XrmDatabase.
*/
class XrmDatabaseHelper
{
public:
    XrmDatabaseHelper(char const * filename=0)
	: m_database(0)
    { if (filename != 0) load(filename); }
	
    ~XrmDatabaseHelper() {
        close();
    }

    /// assignment operator
    XrmDatabaseHelper& operator=(const XrmDatabase& database) {
        if (m_database!=0)
            XrmDestroyDatabase(m_database);
        m_database = database; 
        return *this;
    }
    bool load(const char *filename) {
        if (filename == 0)
            return false;
        XrmDatabase db = XrmGetFileDatabase(filename);
        if (db == 0)
            return false;
        close(); // close old database 
        m_database = db; // set new and return true
        return true;
    }
    void close() {
        if (m_database != 0) {
            XrmDestroyDatabase(m_database);
            m_database = 0;
        }
    }
    bool operator == (const XrmDatabase& database) { return m_database == database; }
    XrmDatabase & operator*(void) {	return m_database; }

private:
    XrmDatabase m_database;	
};

#endif //_XRMDATABASEHELPER_HH_
