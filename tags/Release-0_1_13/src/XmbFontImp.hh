// XmbFontImp.hh for FbTk fluxbox toolkit
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

// $Id: XmbFontImp.hh,v 1.4 2002/10/19 13:58:58 fluxgen Exp $

#ifndef XMBFONTIMP_HH
#define XMBFONTIMP_HH

#include "FontImp.hh"

class XmbFontImp:public FbTk::FontImp {
public:
	XmbFontImp(const char *fontname, bool utf8);
	~XmbFontImp();
	bool load(const std::string &name);
	virtual void drawText(Drawable w, int screen, GC gc, const char *text, size_t len, int x, int y) const;
	unsigned int textWidth(const char * const text, unsigned int len) const;
	unsigned int height() const;
	int ascent() const { return m_setextents ? -m_setextents->max_ink_extent.y : 0; }
	int descent() const { return m_setextents ? m_setextents->max_ink_extent.height + m_setextents->max_ink_extent.y : 0; }
	bool loaded() const { return m_fontset != 0; }
private:
	XFontSet m_fontset;
	XFontSetExtents *m_setextents;
	const bool m_utf8mode;
};

#endif // XMBFONTIMP_HH
