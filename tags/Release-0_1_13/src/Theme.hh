// Theme.hh for fluxbox 
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
// 
// A lot of the base code is taken from Screen.hh in Blackbox 0.61.1
// Screen.cc - Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Theme.hh,v 1.17 2002/10/29 16:06:23 fluxgen Exp $

#ifndef THEME_HH
#define THEME_HH

#include "Image.hh"
#include "DrawUtil.hh"
#include "Font.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <string>
/**
	Main theme class, holds themes for 
	Window, toolbar, slit, menu and the rootCommand, for a specific screen.
*/
class Theme {
public:	
		
	Theme(Display *display, Window rootwindow, Colormap colormap, 
		int screennum, BImageControl *ic, const char *filename, const char *rootcommand);
	~Theme();	
	
	
	typedef struct MenuStyle {
		MenuStyle():titlefont("fixed"),
			framefont("fixed") { }
		FbTk::Color t_text, f_text, h_text, d_text;
		FbTk::Texture title, frame, hilite;
		GC t_text_gc, f_text_gc, h_text_gc, d_text_gc, hilite_gc;
		FbTk::Font titlefont, framefont;
		DrawUtil::Font::FontJustify framefont_justify;
		DrawUtil::Font::FontJustify titlefont_justify;
		int bullet, bullet_pos;
	} MenuStyle;
	
	typedef struct LabelStyle {
		LabelStyle(const char *fontname="fixed"):font(fontname) { }

		FbTk::Texture l_focus, l_unfocus,
			t_focus, t_unfocus;	
		GC l_text_focus_gc, l_text_unfocus_gc;
		FbTk::Font font;
		DrawUtil::Font::FontJustify justify;
		FbTk::Color l_text_focus, l_text_unfocus;
	} LabelStyle;

	
	typedef struct WindowStyle:public LabelStyle {
		FbTk::Color f_focus, f_unfocus, b_pic_focus,
			b_pic_unfocus;
		FbTk::Texture h_focus, h_unfocus,
			b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;
		GC b_pic_focus_gc, b_pic_unfocus_gc;

		struct t_tab:public LabelStyle {
			FbTk::Color border_color;
			unsigned int border_width;
			unsigned int border_width_2x;
			DrawUtil::XRotFontStruct *rot_font;
		} tab;
	
	} WindowStyle;

	
	typedef struct ToolbarStyle {
		ToolbarStyle():font("fixed") { } // default font 'fixed'
		FbTk::Color l_text, w_text, c_text, b_pic;
		FbTk::Texture toolbar, label, window, button, pressed, clock;
		GC l_text_gc, w_text_gc, c_text_gc, b_pic_gc;
		FbTk::Font font;
		DrawUtil::Font::FontJustify justify;
	} ToolbarStyle;	
		
	inline WindowStyle &getWindowStyle() { return m_windowstyle; }
	inline MenuStyle &getMenuStyle() { return m_menustyle; }
	inline ToolbarStyle &getToolbarStyle() { return m_toolbarstyle; }
	inline const FbTk::Texture &getSlitTexture() const { return m_slit_texture; }
	inline unsigned int getBevelWidth() const { return m_bevel_width; }
	inline unsigned int getBorderWidth() const { return m_border_width; }
	inline unsigned int getHandleWidth() const { return m_handle_width; }
	inline unsigned int getFrameWidth() const { return m_frame_width; }
	inline GC getOpGC() const { return m_opgc; }
	inline const FbTk::Color &getBorderColor() const { return m_border_color; }
	void load(const char *filename);
	void reconfigure(bool antialias = false);
	
	inline void setRootCommand(const std::string &command) { m_rootcommand = command; }
	
	
	
private:
	
	void loadMenuStyle();
	void loadWindowStyle();
	void loadTabStyle();
	void loadToolbarStyle();	
	void loadRootCommand();
	void loadMisc();
	void freeMenuStyle();
	void freeWindowStyle();
	void freeTabStyle();
	void freeToolbarStyle();
	void loadFontFromDatabase(FbTk::Font &dest, const char *name, const char *altname);
	bool readDatabaseTexture(char *, char *, FbTk::Texture *, unsigned long);
	bool readDatabaseColor(char *, char *, FbTk::Color *, unsigned long);

	void readDatabaseFontSet(char *, char *, XFontSet *);
	XFontSet createFontSet(char *);
	void readDatabaseFont(char *, char *, XFontStruct **);

	static const char *getFontElement(const char *pattern, char *buf, int bufsiz, ...);
	static const char *getFontSize(const char *pattern, int *size);


	WindowStyle m_windowstyle;
	MenuStyle m_menustyle;
	ToolbarStyle	m_toolbarstyle;
	unsigned int m_bevel_width, m_border_width, m_handle_width, m_frame_width;
	FbTk::Color m_border_color;
	GC m_opgc;
	BImageControl *m_imagecontrol;
	Display *m_display;	 
	XrmDatabase m_database;
	Colormap m_colormap;
	int m_screennum;
	std::string m_rootcommand;
	FbTk::Texture m_slit_texture;
};


#endif  //_THEME_HH_
