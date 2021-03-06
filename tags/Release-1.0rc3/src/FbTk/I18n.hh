// i18n.hh for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// i18n.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#ifndef	 I18N_HH
#define	 I18N_HH

// TODO: FIXME
#include "../../nls/fluxbox-nls.hh"

#include "FbString.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H


#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef HAVE_NL_TYPES_H
// this is needed for linux libc5 systems
extern "C" {
#include <nl_types.h>
}
#elif defined(__CYGWIN__) || defined(__EMX__) || defined(__APPLE__)
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
typedef int nl_catd;
char *catgets(nl_catd cat, int set_number, int message_number, char *message);
nl_catd catopen(char *name, int flag);
void catclose(nl_catd cat);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HAVE_NL_TYPES_H

#include <string>

// Some defines to help out
#ifdef NLS
#define _FB_USES_NLS \
    FbTk::I18n &i18n = *FbTk::I18n::instance()

// ignore the description, it's for helping translators

// Text for X
#define _FB_XTEXT(msgset, msgid, default_text, description) \
    i18n.getMessage(FBNLS::msgset ## Set, FBNLS::msgset ## msgid, default_text, true)

// Text for console    
#define _FB_CONSOLETEXT(msgset, msgid, default_text, description) \
    i18n.getMessage(FBNLS::msgset ## Set, FBNLS::msgset ## msgid, default_text, false)

// This ensure that FbTk nls stuff is in a kind of namespace of its own
#define _FBTK_XTEXT( msgset, msgid, default_text, description) \
    i18n.getMessage(FBNLS::FbTk ## msgset ## Set, FBNLS::FbTk ## msgset ## msgid, default_text, true)

#define _FBTK_CONSOLETEXT( msgset, msgid, default_text, description) \
    i18n.getMessage(FBNLS::FbTk ## msgset ## Set, FBNLS::FbTk ## msgset ## msgid, default_text, false)

#else // no NLS

#define _FB_USES_NLS

#define _FB_XTEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _FB_CONSOLETEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _FBTK_XTEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _FBTK_CONSOLETEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#endif // defined NLS

namespace FbTk {

class I18n {
public:
    static I18n *instance();
    inline const char *getLocale() const { return m_locale.c_str(); }
    inline bool multibyte() const { return m_multibyte; }
    inline const nl_catd &getCatalogFd() const { return m_catalog_fd; }

    FbString getMessage(int set_number, int message_number, 
                           const char *default_messsage = 0, bool translate_fb = false) const;

    void openCatalog(const char *catalog);
private:
    I18n();
    ~I18n();
    std::string m_locale;
    bool m_multibyte, m_utf8_translate;
    nl_catd m_catalog_fd;


};

void NLSInit(const char *);

}; // end namespace FbTk

#endif // I18N_HH
