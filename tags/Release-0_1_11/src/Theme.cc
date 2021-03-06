// Theme.cc for fluxbox 
// Copyright (c) 2001-2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

//  A lot of the code base is taken from Screen.cc in Blackbox 0.61.1
//  and Brad Hughes (bhuges@tcac.net) should get alot of credit for it
//  Screen.cc - Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
// 
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

// $Id: Theme.cc,v 1.26 2002/08/24 19:06:06 fluxgen Exp $

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif //HAVE_CONFIG_H_

#include "Theme.hh"

#include "i18n.hh"
#include "Basemenu.hh"
#include "StringUtil.hh"

#include <X11/Xresource.h>

#ifdef		HAVE_CTYPE_H
#	include <ctype.h>
#endif // HAVE_CTYPE_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <iostream>
using namespace std;

Theme::Theme(Display *display, Window rootwindow, Colormap colormap, 
	int screennum, BImageControl *ic, const char *filename, const char *rootcommand):
m_menustyle(display),
m_imagecontrol(ic),
m_display(display),
m_colormap(colormap),
m_screennum(screennum),
m_rootcommand(rootcommand==0 ? "" : rootcommand) //we dont want to send 0-pointer to std::string	
{
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): Creating."<<endl;
	#endif //DEBUG
	//default settings	

	m_windowstyle.font.set = m_toolbarstyle.font.set = m_windowstyle.tab.font.set =  0;
	
	m_toolbarstyle.font.fontstruct = m_windowstyle.font.fontstruct = m_windowstyle.tab.font.fontstruct = 0;
	m_windowstyle.tab.rot_font = 0;
		
	load(filename);
	//-------- create gc for the styles ------------
	
	
	XGCValues gcv;
	unsigned long gc_value_mask = GCForeground;

	if (! I18n::instance()->multibyte())
		gc_value_mask |= GCFont;

	gcv.foreground = WhitePixel(m_display, screennum)^BlackPixel(m_display, screennum);
	gcv.function = GXxor;
	gcv.subwindow_mode = IncludeInferiors;
	m_opgc = XCreateGC(m_display, rootwindow,
		GCForeground | GCFunction | GCSubwindowMode, &gcv);

	gcv.foreground = m_windowstyle.l_text_focus.pixel();
	if (m_windowstyle.font.fontstruct)
		gcv.font = m_windowstyle.font.fontstruct->fid;
		
	m_windowstyle.l_text_focus_gc =
		XCreateGC(m_display, rootwindow,
			gc_value_mask, &gcv);
	
	gcv.foreground = m_windowstyle.l_text_unfocus.pixel();
	if (m_windowstyle.font.fontstruct)
		gcv.font = m_windowstyle.font.fontstruct->fid;
	m_windowstyle.l_text_unfocus_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);
	
	//---- Tab 
	gcv.foreground = m_windowstyle.tab.l_text_focus.pixel();
	if (m_windowstyle.tab.font.fontstruct)
		gcv.font = m_windowstyle.tab.font.fontstruct->fid;
	
	m_windowstyle.tab.l_text_focus_gc =
		XCreateGC(m_display, rootwindow,
			gc_value_mask, &gcv);
	
	gcv.foreground = m_windowstyle.tab.l_text_unfocus.pixel();
	m_windowstyle.tab.l_text_unfocus_gc =
		XCreateGC(m_display, rootwindow,
			gc_value_mask, &gcv);
	
	//---end Tab			
	
				
	gcv.foreground = m_windowstyle.b_pic_focus.pixel();
	m_windowstyle.b_pic_focus_gc =
		XCreateGC(m_display, rootwindow,
				GCForeground, &gcv);

	gcv.foreground = m_windowstyle.b_pic_unfocus.pixel();
	m_windowstyle.b_pic_unfocus_gc =
		XCreateGC(m_display, rootwindow,
				GCForeground, &gcv);

	gcv.foreground = m_menustyle.t_text.pixel();
	if (m_menustyle.titlefont.fontStruct())
		gcv.font = m_menustyle.titlefont.fontStruct()->fid;
	m_menustyle.t_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.f_text.pixel();
	if (m_menustyle.framefont.fontStruct())
		gcv.font = m_menustyle.framefont.fontStruct()->fid;

	m_menustyle.f_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.h_text.pixel();
	m_menustyle.h_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.d_text.pixel();
	m_menustyle.d_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.hilite.color().pixel();
	m_menustyle.hilite_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.l_text.pixel();
	if (m_toolbarstyle.font.fontstruct)
		gcv.font = m_toolbarstyle.font.fontstruct->fid;
	m_toolbarstyle.l_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.w_text.pixel();
	m_toolbarstyle.w_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.c_text.pixel();
	m_toolbarstyle.c_text_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.b_pic.pixel();
	m_toolbarstyle.b_pic_gc =
		XCreateGC(m_display, rootwindow,
				gc_value_mask, &gcv);
	
}

Theme::~Theme() {

	freeMenuStyle();
	freeWindowStyle();
	freeToolbarStyle();
	freeTabStyle();
}

//----- freeMenuStyle -----
// free memory allocated for m_menustyle
// should only be called from ~Theme
//--------------------
void Theme::freeMenuStyle() {
		
	XFreeGC(m_display, m_menustyle.t_text_gc);
	XFreeGC(m_display, m_menustyle.f_text_gc);
	XFreeGC(m_display, m_menustyle.h_text_gc);
	XFreeGC(m_display, m_menustyle.d_text_gc);
	XFreeGC(m_display, m_menustyle.hilite_gc);
}

//----- freeWindowStyle -----
// free memory allocated for m_windowstyle
// should only be called from ~Theme
//--------------------
void Theme::freeWindowStyle() {
	if (m_windowstyle.font.set)
		XFreeFontSet(m_display, m_windowstyle.font.set);
	
	if (m_windowstyle.font.fontstruct)
		XFreeFont(m_display, m_windowstyle.font.fontstruct);	
		
	XFreeGC(m_display, m_windowstyle.l_text_focus_gc);
	XFreeGC(m_display, m_windowstyle.l_text_unfocus_gc);
	XFreeGC(m_display, m_windowstyle.b_pic_focus_gc);
	XFreeGC(m_display, m_windowstyle.b_pic_unfocus_gc);
}

//----- freeTabStyle -----
// free memory allocated for m_windowstyle.tab
// should only be called from ~Theme
//--------------------
void Theme::freeTabStyle() {	
	
	if (m_windowstyle.tab.font.set)
		XFreeFontSet(m_display, m_windowstyle.tab.font.set);
		
	if (m_windowstyle.tab.font.fontstruct)
		XFreeFont(m_display, m_windowstyle.tab.font.fontstruct);

	if (m_windowstyle.tab.rot_font)
		DrawUtil::XRotUnloadFont(m_display, m_windowstyle.tab.rot_font);

	
	XFreeGC(m_display, m_windowstyle.tab.l_text_focus_gc);
	XFreeGC(m_display, m_windowstyle.tab.l_text_unfocus_gc);			
}

//----- freeToolbarStyle -----
// free memory allocated for m_toolbarstyle
// should only be called from ~Theme
//--------------------
void Theme::freeToolbarStyle() {
	
	if (m_toolbarstyle.font.set)
		XFreeFontSet(m_display, m_toolbarstyle.font.set);

	if (m_toolbarstyle.font.fontstruct)
		XFreeFont(m_display, m_toolbarstyle.font.fontstruct);

	XFreeGC(m_display, m_toolbarstyle.l_text_gc);
	XFreeGC(m_display, m_toolbarstyle.w_text_gc);
	XFreeGC(m_display, m_toolbarstyle.c_text_gc);
	XFreeGC(m_display, m_toolbarstyle.b_pic_gc);
	
}

//---------- load ------------
// Loads a theme from a file
//----------------------------
void Theme::load(const char *filename){
	m_database = XrmGetFileDatabase(filename);
	if (!m_database)
		m_database = XrmGetFileDatabase(DEFAULTSTYLE);

	loadMenuStyle();
	loadToolbarStyle();	
	loadWindowStyle();
	loadTabStyle();
	loadRootCommand();
	loadMisc();
	
	XrmDestroyDatabase(m_database);
}

void Theme::loadMenuStyle() {

	readDatabaseTexture("menu.title", "Menu.Title",
		&m_menustyle.title,
		WhitePixel(m_display, m_screennum));
	readDatabaseTexture("menu.frame", "Menu.Frame",
		&m_menustyle.frame,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("menu.hilite", "Menu.Hilite",
		&m_menustyle.hilite,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor",
		&m_menustyle.t_text,
		BlackPixel(m_display, m_screennum));
	readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor",
		&m_menustyle.f_text,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("menu.frame.disableColor", "Menu.Frame.DisableColor",
		&m_menustyle.d_text,
		BlackPixel(m_display, m_screennum));
	readDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor",
		&m_menustyle.h_text,
		BlackPixel(m_display, m_screennum));

	XrmValue value;
	char *value_type=0;
	
	if (XrmGetResource(m_database, "menu.title.justify",
			"Menu.Title.Justify", &value_type, &value)) {
				 
		if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
			m_menustyle.titlefont_justify = DrawUtil::Font::RIGHT;
		else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
			m_menustyle.titlefont_justify = DrawUtil::Font::CENTER;
		else
			m_menustyle.titlefont_justify = DrawUtil::Font::LEFT;
		
	} else
		m_menustyle.titlefont_justify = DrawUtil::Font::LEFT;

	if (XrmGetResource(m_database, "menu.frame.justify",
				 "Menu.Frame.Justify", &value_type, &value)) {
		
		if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
			m_menustyle.framefont_justify = DrawUtil::Font::RIGHT;
		else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
			m_menustyle.framefont_justify = DrawUtil::Font::CENTER;
		else
			m_menustyle.framefont_justify = DrawUtil::Font::LEFT;
			
	} else
		m_menustyle.framefont_justify = DrawUtil::Font::LEFT;

	if (XrmGetResource(m_database, "menu.bullet", "Menu.Bullet",
										 &value_type, &value)) {
		
		if (! strncasecmp(value.addr, "empty", value.size))
			m_menustyle.bullet = Basemenu::EMPTY;
		else if (! strncasecmp(value.addr, "square", value.size))
			m_menustyle.bullet = Basemenu::SQUARE;
		else if (! strncasecmp(value.addr, "diamond", value.size))
			m_menustyle.bullet = Basemenu::DIAMOND;
		else
			m_menustyle.bullet = Basemenu::TRIANGLE;
			
	} else
		m_menustyle.bullet = Basemenu::TRIANGLE;

	if (XrmGetResource(m_database, "menu.bullet.position",
										 "Menu.Bullet.Position", &value_type, &value)) {

		if (! strncasecmp(value.addr, "right", value.size))
			m_menustyle.bullet_pos = Basemenu::RIGHT;
		else
			m_menustyle.bullet_pos = Basemenu::LEFT;
			
	} else
		m_menustyle.bullet_pos = Basemenu::LEFT;

	//---------- font
	m_menustyle.framefont.loadFromDatabase(m_database, "menu.frame.font", "Menu.Frame.Font");
	m_menustyle.titlefont.loadFromDatabase(m_database, "menu.title.font", "Menu.Title.Font");
	
}

void Theme::loadWindowStyle() {
	
	//read textures
	
	readDatabaseTexture("window.title.focus", "Window.Title.Focus",
		&m_windowstyle.t_focus,
		WhitePixel(m_display, m_screennum));
	if ( (m_windowstyle.t_focus.type() & FbTk::Texture::PARENTRELATIVE) ) {
		cerr<<"Warning: window.title.focus type is ParentRelative. Using flat solid!"<<endl;
		m_windowstyle.t_focus.setType(FbTk::Texture::FLAT|FbTk::Texture::SOLID);
	}
	readDatabaseTexture("window.title.unfocus", "Window.Title.Unfocus",
		&m_windowstyle.t_unfocus,
		BlackPixel(m_display, m_screennum));
	if ( (m_windowstyle.t_unfocus.type() & FbTk::Texture::PARENTRELATIVE) ) {
		cerr<<"Warning: window.title.unfocus type is ParentRelative. Using flat solid!"<<endl;
		m_windowstyle.t_unfocus.setType(FbTk::Texture::FLAT|FbTk::Texture::SOLID);
	}
	
	readDatabaseTexture("window.label.focus", "Window.Label.Focus",
		&m_windowstyle.l_focus,
		WhitePixel(m_display, m_screennum));
	readDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus",
		&m_windowstyle.l_unfocus,
		BlackPixel(m_display, m_screennum));	
	
	
	readDatabaseTexture("window.handle.focus", "Window.Handle.Focus",
		&m_windowstyle.h_focus,
		WhitePixel(m_display, m_screennum));
	if ( (m_windowstyle.h_focus.type() & FbTk::Texture::PARENTRELATIVE) ) {
		cerr<<"Warning: window.handle.focus is ParentReleative. Using flat solid instead."<<endl;
		m_windowstyle.h_focus.setType(FbTk::Texture::FLAT|FbTk::Texture::SOLID);
	}
	readDatabaseTexture("window.handle.unfocus", "Window.Handle.Unfocus",
		&m_windowstyle.h_unfocus,
		BlackPixel(m_display, m_screennum));
	if ( (m_windowstyle.h_unfocus.type() & FbTk::Texture::PARENTRELATIVE) ) {
		cerr<<"Warning: window.handle.unfocus is ParentReleative. Using flat solid instead."<<endl;
		m_windowstyle.h_unfocus.setType(FbTk::Texture::FLAT|FbTk::Texture::SOLID);
	}

	readDatabaseTexture("window.grip.focus", "Window.Grip.Focus",
		&m_windowstyle.g_focus,
		WhitePixel(m_display, m_screennum));
	readDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus",
		&m_windowstyle.g_unfocus,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("window.button.focus", "Window.Button.Focus",
		&m_windowstyle.b_focus,
		WhitePixel(m_display, m_screennum));
	readDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus",
		&m_windowstyle.b_unfocus,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("window.button.pressed", "Window.Button.Pressed",
		&m_windowstyle.b_pressed,
		BlackPixel(m_display, m_screennum));
	
	// read colors
	
	readDatabaseColor("window.frame.focusColor",
		"Window.Frame.FocusColor",
		&m_windowstyle.f_focus,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("window.frame.unfocusColor",
		"Window.Frame.UnfocusColor",
		&m_windowstyle.f_unfocus,
		BlackPixel(m_display, m_screennum));
	readDatabaseColor("window.label.focus.textColor",
		"Window.Label.Focus.TextColor",
		&m_windowstyle.l_text_focus,
		BlackPixel(m_display, m_screennum));
	readDatabaseColor("window.label.unfocus.textColor",
		"Window.Label.Unfocus.TextColor",
		&m_windowstyle.l_text_unfocus,
		WhitePixel(m_display, m_screennum));	
	readDatabaseColor("window.button.focus.picColor",
		"Window.Button.Focus.PicColor",
		&m_windowstyle.b_pic_focus,
		BlackPixel(m_display, m_screennum));
	readDatabaseColor("window.button.unfocus.picColor",
		"Window.Button.Unfocus.PicColor",
		&m_windowstyle.b_pic_unfocus,
		WhitePixel(m_display, m_screennum));

	//----- font
	
	if (I18n::instance()->multibyte()) {
		readDatabaseFontSet("window.font", "Window.Font",
			&m_windowstyle.font.set);	
			
		m_windowstyle.font.set_extents =
			XExtentsOfFontSet(m_windowstyle.font.set);
	} else {
		readDatabaseFont("window.font", "Window.Font",
			&m_windowstyle.font.fontstruct);
	}
	
	XrmValue value;
	char *value_type;
	
	if (XrmGetResource(m_database, "window.justify", "Window.Justify",
			&value_type, &value)) {
		if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
			m_windowstyle.font.justify = DrawUtil::Font::RIGHT;
		else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
			m_windowstyle.font.justify = DrawUtil::Font::CENTER;
		else
			m_windowstyle.font.justify = DrawUtil::Font::LEFT;
	} else
		m_windowstyle.font.justify = DrawUtil::Font::LEFT;
		
}

void Theme::loadTabStyle() {

	if (!readDatabaseTexture("window.tab.title.focus", "Window.Tab.Title.Focus",
			&m_windowstyle.tab.t_focus,
			WhitePixel(m_display, m_screennum)))
		m_windowstyle.tab.t_focus = m_windowstyle.t_focus;
		
	if (!readDatabaseTexture("window.tab.title.unfocus", "Window.Tab.Title.Unfocus",
			&m_windowstyle.tab.t_unfocus,
			BlackPixel(m_display, m_screennum)))
		m_windowstyle.tab.t_unfocus = m_windowstyle.t_unfocus;
	
	if (!readDatabaseTexture("window.tab.label.focus", "Window.Tab.Label.Focus",
			&m_windowstyle.tab.l_focus,
			WhitePixel(m_display, m_screennum)))
		m_windowstyle.tab.l_focus = m_windowstyle.l_focus;
		
	if (!readDatabaseTexture("window.tab.label.unfocus", "Window.Tab.Label.Unfocus",
			&m_windowstyle.tab.l_unfocus,
			BlackPixel(m_display, m_screennum)))
		m_windowstyle.tab.l_unfocus = m_windowstyle.l_unfocus;

	if (!readDatabaseColor("window.tab.label.focus.textColor",
			"Window.Tab.Label.Focus.TextColor",
			&m_windowstyle.tab.l_text_focus,
			BlackPixel(m_display, m_screennum)))
		m_windowstyle.tab.l_text_focus = m_windowstyle.l_text_focus;
		
	if (!readDatabaseColor("window.tab.label.unfocus.textColor",
			"Window.Tab.Label.Unfocus.TextColor",
			&m_windowstyle.tab.l_text_unfocus,
			WhitePixel(m_display, m_screennum)))
		m_windowstyle.tab.l_text_unfocus = m_windowstyle.l_text_unfocus;
	
	readDatabaseColor("window.tab.borderColor", "Window.Tab.BorderColor", 
			&m_windowstyle.tab.border_color,	
			BlackPixel(m_display, m_screennum));

	XrmValue value;
	char *value_type;

	if (XrmGetResource(m_database, "window.tab.borderWidth", "Window.Tab.BorderWidth",
			&value_type, &value)) {
		if (sscanf(value.addr, "%u", &m_windowstyle.tab.border_width) != 1)
			m_windowstyle.tab.border_width = 1;
	} else
		m_windowstyle.tab.border_width = 1;

	m_windowstyle.tab.border_width_2x = m_windowstyle.tab.border_width*2;
	
	//---------- font
	
	if (I18n::instance()->multibyte()) {
		readDatabaseFontSet("window.tab.font", "Window.Tab.Font",
			&m_windowstyle.tab.font.set);
		
		m_windowstyle.tab.font.set_extents =
			XExtentsOfFontSet(m_windowstyle.tab.font.set);
	} else {
		readDatabaseFont("window.tab.font", "Window.Tab.Font",
			&m_windowstyle.tab.font.fontstruct);
	}
	
	//--------- rotated font for left and right tabs
	// TODO: add extra checking
	if (XrmGetResource(m_database, "window.tab.font", "Window.Tab.Font",
			&value_type, &value)) {		
		if (! (m_windowstyle.tab.rot_font = DrawUtil::XRotLoadFont(m_display, value.addr, 90.0)) )
			m_windowstyle.tab.rot_font = DrawUtil::XRotLoadFont(m_display, "fixed", 90);
	} else
		m_windowstyle.tab.rot_font = DrawUtil::XRotLoadFont(m_display, "fixed", 90);

	if (XrmGetResource(m_database, "window.tab.justify", "Window.Tab.Justify",
			 &value_type, &value)) {
		if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
			m_windowstyle.tab.font.justify = DrawUtil::Font::RIGHT;
		else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
			m_windowstyle.tab.font.justify = DrawUtil::Font::CENTER;
		else
			m_windowstyle.tab.font.justify = DrawUtil::Font::LEFT;
	} else
		m_windowstyle.tab.font.justify = DrawUtil::Font::LEFT;
		
}

void Theme::loadToolbarStyle() {

	readDatabaseTexture("toolbar", "Toolbar",
		&m_toolbarstyle.toolbar,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("toolbar.label", "Toolbar.Label",
		&m_toolbarstyle.label,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("toolbar.windowLabel", "Toolbar.WindowLabel",
		&m_toolbarstyle.window,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("toolbar.button", "Toolbar.Button",
		&m_toolbarstyle.button,
		WhitePixel(m_display, m_screennum));
	readDatabaseTexture("toolbar.button.pressed", "Toolbar.Button.Pressed",
		&m_toolbarstyle.pressed,
		BlackPixel(m_display, m_screennum));
	readDatabaseTexture("toolbar.clock", "Toolbar.Clock",
		&m_toolbarstyle.clock,
		BlackPixel(m_display, m_screennum));

	readDatabaseColor("toolbar.label.textColor", "Toolbar.Label.TextColor",
		&m_toolbarstyle.l_text,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("toolbar.windowLabel.textColor",
		"Toolbar.WindowLabel.TextColor",
		&m_toolbarstyle.w_text,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("toolbar.clock.textColor", "Toolbar.Clock.TextColor",
		&m_toolbarstyle.c_text,
		WhitePixel(m_display, m_screennum));
	readDatabaseColor("toolbar.button.picColor", "Toolbar.Button.PicColor",
		&m_toolbarstyle.b_pic,
		BlackPixel(m_display, m_screennum));

	
	// ----------- load font
	
	if (I18n::instance()->multibyte()) {
		readDatabaseFontSet("toolbar.font", "Toolbar.Font",
			&m_toolbarstyle.font.set);
				
		m_toolbarstyle.font.set_extents =
			XExtentsOfFontSet(m_toolbarstyle.font.set);
	} else {
		readDatabaseFont("toolbar.font", "Toolbar.Font",
			&m_toolbarstyle.font.fontstruct);
	}
	XrmValue value;
	char *value_type;

	if (XrmGetResource(m_database, "toolbar.justify",
			"Toolbar.Justify", &value_type, &value)) {
		if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
			m_toolbarstyle.font.justify = DrawUtil::Font::RIGHT;
		else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
			m_toolbarstyle.font.justify = DrawUtil::Font::CENTER;
		else
			m_toolbarstyle.font.justify = DrawUtil::Font::LEFT;
	} else
		m_toolbarstyle.font.justify = DrawUtil::Font::LEFT;

}

void Theme::loadRootCommand() {
	XrmValue value;
	char *value_type;
	
	if (m_rootcommand.size()) {
		#ifndef  __EMX__		
		char tmpstring[256]; //to hold m_screennum 
		tmpstring[0]=0;
		sprintf(tmpstring, "%d", m_screennum);
		string displaystring("DISPLAY=");
		displaystring.append(DisplayString(m_display));
		displaystring.append(tmpstring); // append m_screennum				
		
		bexec(m_rootcommand.c_str(), const_cast<char *>(displaystring.c_str()));

		#else //         __EMX__
		spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", m_rootcommand.c_str(), 0);  
		#endif // !__EMX__     

	} else if (XrmGetResource(m_database, "rootCommand", "RootCommand",
			&value_type, &value)) {
		#ifndef		__EMX__
		char tmpstring[256]; //to hold m_screennum
		tmpstring[0]=0;
		sprintf(tmpstring, "%d", m_screennum);
		string displaystring("DISPLAY=");
		displaystring.append(DisplayString(m_display));
		displaystring.append(tmpstring); // append m_screennum				
		
		bexec(value.addr, const_cast<char *>(displaystring.c_str()));
		#else //	 __EMX__
	
		spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", value.addr, 0);
	
		#endif // !__EMX__
		
	}
	#ifdef DEBUG
	else
		cerr<<__FILE__<<"("<<__LINE__<<"): Didn't find rootCommand."<<endl;
	#endif	

}

void Theme::loadMisc() {
	unsigned int screen_width_div2 = WidthOfScreen(ScreenOfDisplay(m_display, m_screennum)) / 2;
	XrmValue value;
	char *value_type=0;
	
	if (XrmGetResource(m_database, "bevelWidth", "BevelWidth",
										 &value_type, &value)) {
		if (sscanf(value.addr, "%u", &m_bevel_width) != 1 ||
				m_bevel_width >  screen_width_div2 || 
				m_bevel_width == 0)
			m_bevel_width = 3;
	} else
		m_bevel_width = 3;

	if (XrmGetResource(m_database, "handleWidth", "HandleWidth",
										 &value_type, &value)) {
		if (sscanf(value.addr, "%u", &m_handle_width) != 1 ||
			m_handle_width > screen_width_div2 || m_handle_width == 0)
			m_handle_width = 6;
	} else
		m_handle_width = 6;

	if (XrmGetResource(m_database, "borderWidth", "BorderWidth",
			&value_type, &value)) {
		if (sscanf(value.addr, "%u", &m_border_width) != 1)
			m_border_width = 1;
	} else
		m_border_width = 1;
	
	
	if (XrmGetResource(m_database, "frameWidth", "FrameWidth",
			&value_type, &value)) {
		if (sscanf(value.addr, "%u", &m_frame_width) != 1 ||
				m_frame_width > screen_width_div2)
			m_frame_width = m_bevel_width;
	} else
		m_frame_width = m_bevel_width;

	readDatabaseColor("borderColor", "BorderColor", &m_border_color,
		BlackPixel(m_display, m_screennum));		

	// load slit style, if it wasn't found fall back to toolbarstyle
	if (!readDatabaseTexture("slit", "Slit",
		&m_slit_texture, 
		BlackPixel(m_display, m_screennum)) ) {
		m_slit_texture = m_toolbarstyle.toolbar;
	}

}


bool Theme::readDatabaseTexture(char *rname, char *rclass,
	FbTk::Texture *texture,
	unsigned long default_pixel)
{
	XrmValue value;
	char *value_type;
	bool retval = true;//return true as default
	
	if (XrmGetResource(m_database, rname, rclass, &value_type,
			&value))
		m_imagecontrol->parseTexture(texture, value.addr);
	else
		texture->setType(FbTk::Texture::SOLID | FbTk::Texture::FLAT);

	if (texture->type() & FbTk::Texture::SOLID) {
		int clen = strlen(rclass) + 32, nlen = strlen(rname) + 32;

		char *colorclass = new char[clen], *colorname = new char[nlen];

		sprintf(colorclass, "%s.Color", rclass);
		sprintf(colorname,	"%s.color", rname);
		
		if (!readDatabaseColor(colorname, colorclass, &texture->color(),
				default_pixel))
			retval = false;

#ifdef		INTERLACE
		sprintf(colorclass, "%s.ColorTo", rclass);
		sprintf(colorname,	"%s.colorTo", rname);

		readDatabaseColor(colorname, colorclass, &texture->colorTo(), default_pixel);
#endif // INTERLACE

		delete [] colorclass;
		delete [] colorname;

		if ((! texture->color().isAllocated()) ||
				(texture->type() & FbTk::Texture::FLAT))
			return retval;

		XColor xcol;

		xcol.red = (unsigned int) (texture->color().red() +
			(texture->color().red() >> 1));
		if (xcol.red >= 0xff) xcol.red = 0xffff;
		else xcol.red *= 0xff;
		xcol.green = (unsigned int) (texture->color().green() +
			(texture->color().green() >> 1));
		if (xcol.green >= 0xff) xcol.green = 0xffff;
		else xcol.green *= 0xff;
		xcol.blue = (unsigned int) (texture->color().blue() +
			(texture->color().blue() >> 1));
		if (xcol.blue >= 0xff) xcol.blue = 0xffff;
		else xcol.blue *= 0xff;

		if (! XAllocColor(m_display, m_colormap, &xcol))
			xcol.pixel = 0;

		texture->hiColor().setPixel(xcol.pixel);

		xcol.red =
			(unsigned int) ((texture->color().red() >> 2) +
			(texture->color().red() >> 1)) * 0xff;
		xcol.green =
			(unsigned int) ((texture->color().green() >> 2) +
			(texture->color().green() >> 1)) * 0xff;
		xcol.blue =
			(unsigned int) ((texture->color().blue() >> 2) +
			(texture->color().blue() >> 1)) * 0xff;

		if (! XAllocColor(m_display, m_colormap, &xcol))
			xcol.pixel = 0;

		texture->loColor().setPixel(xcol.pixel);
	} else if (texture->type() & FbTk::Texture::GRADIENT) {
		int clen = strlen(rclass) + 10, nlen = strlen(rname) + 10;

		char *colorclass = new char[clen], *colorname = new char[nlen],
			*colortoclass = new char[clen], *colortoname = new char[nlen];

		sprintf(colorclass, "%s.Color", rclass);
		sprintf(colorname,	"%s.color", rname);

		sprintf(colortoclass, "%s.ColorTo", rclass);
		sprintf(colortoname,	"%s.colorTo", rname);

		if (!readDatabaseColor(colorname, colorclass, &texture->color(),
				default_pixel))
			retval = false;	//report failure in loading
				
		readDatabaseColor(colortoname, colortoclass, &texture->colorTo(), default_pixel);

		delete [] colorclass;
		delete [] colorname;
		delete [] colortoclass;
		delete [] colortoname;
	}

	if (!retval)
		cerr<<"Failed to load texture for: "<<rname<<endl;
	
	return retval;
}


bool Theme::readDatabaseColor(char *rname, char *rclass, FbTk::Color *color,
				unsigned long default_pixel)
{
	XrmValue value;
	char *value_type;

	if (XrmGetResource(m_database, rname, rclass, &value_type,
			&value)) {
		m_imagecontrol->parseColor(color, value.addr);
	} else {
		// parsing with no color string just deallocates the color, if it has
		// been previously allocated
		m_imagecontrol->parseColor(color);
		color->setPixel(default_pixel);
		return false; 
	}

	return true;
}


void Theme::readDatabaseFontSet(char *rname, char *rclass, XFontSet *fontset) {
	if (! fontset) return;

	static char *defaultFont = "fixed";

	bool load_default = false;
	XrmValue value;
	char *value_type;

	if (*fontset)
		XFreeFontSet(m_display, *fontset);

	if (XrmGetResource(m_database, rname, rclass, &value_type, &value)) {
		char *fontname = value.addr;
		if (! (*fontset = createFontSet(fontname)))
			load_default = true;
	} else
		load_default = true;

	if (load_default) {
		*fontset = createFontSet(defaultFont);

		if (! *fontset) {
			fprintf(stderr,
				I18n::instance()->
				getMessage(
				 FBNLS::ScreenSet, FBNLS::ScreenDefaultFontLoadFail,
				 "BScreen::LoadStyle(): couldn't load default font.\n"));
			throw 2;
		}
	}
}



void Theme::readDatabaseFont(char *rname, char *rclass, XFontStruct **font) {
	if (! font) return;

	static char *defaultFont = "fixed";

	bool load_default = false;
	XrmValue value;
	char *value_type;

	if (*font)
		XFreeFont(m_display, *font);

	if (XrmGetResource(m_database, rname, rclass, &value_type, &value)) {
		#ifdef DEBUG
		cerr<<__FILE__<<"("<<__LINE__<<"): Load font:"<<value.addr<<endl;
		#endif
		if ((*font = XLoadQueryFont(m_display, value.addr)) == 0) {
			fprintf(stderr,
				I18n::instance()->
				getMessage(
					FBNLS::ScreenSet, FBNLS::ScreenFontLoadFail,
					"BScreen::LoadStyle(): couldn't load font '%s'\n"),
				value.addr);

			load_default = true;
		}
	} else
		load_default = true;

	if (load_default) {
		if ((*font = XLoadQueryFont(m_display, defaultFont)) == 0) {
			fprintf(stderr,
				I18n::instance()->
				getMessage(
					 FBNLS::ScreenSet, FBNLS::ScreenDefaultFontLoadFail,
					 "BScreen::LoadStyle(): couldn't load default font.\n"));
			throw 2; //fatal!
		}
	}
}

void Theme::reconfigure() {

	XGCValues gcv;
	unsigned long gc_value_mask = GCForeground;
	if (! I18n::instance()->multibyte())
		gc_value_mask |= GCFont;
	
	XChangeGC(m_display, m_opgc,
		GCForeground | GCFunction | GCSubwindowMode, &gcv);
		
	gcv.foreground = WhitePixel(m_display, m_screennum);
	gcv.function = GXinvert;
	gcv.subwindow_mode = IncludeInferiors;
	XChangeGC(m_display, m_opgc,
		GCForeground | GCFunction | GCSubwindowMode, &gcv);

	gcv.foreground = m_windowstyle.l_text_focus.pixel();
	if (m_windowstyle.font.fontstruct)
		gcv.font = m_windowstyle.font.fontstruct->fid;
		
	XChangeGC(m_display, m_windowstyle.l_text_focus_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_windowstyle.l_text_unfocus.pixel();
	XChangeGC(m_display, m_windowstyle.l_text_unfocus_gc,
		gc_value_mask, &gcv);

	//---- Tab 
	gcv.foreground = m_windowstyle.tab.l_text_focus.pixel();
	if (m_windowstyle.tab.font.fontstruct)
		gcv.font = m_windowstyle.tab.font.fontstruct->fid;
	
	XChangeGC(m_display, m_windowstyle.tab.l_text_focus_gc,
		gc_value_mask, &gcv);
	
	gcv.foreground = m_windowstyle.tab.l_text_unfocus.pixel();
	XChangeGC(m_display, m_windowstyle.tab.l_text_unfocus_gc,
		gc_value_mask, &gcv);
	
	//--- end tab
		
	gcv.foreground = m_windowstyle.b_pic_focus.pixel();
	XChangeGC(m_display, m_windowstyle.b_pic_focus_gc,
		GCForeground, &gcv);

	gcv.foreground = m_windowstyle.b_pic_unfocus.pixel();
	XChangeGC(m_display, m_windowstyle.b_pic_unfocus_gc,
		GCForeground, &gcv);

	gcv.foreground = m_menustyle.t_text.pixel();
	if (m_menustyle.titlefont.fontStruct())
		gcv.font = m_menustyle.titlefont.fontStruct()->fid;
	XChangeGC(m_display, m_menustyle.t_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.f_text.pixel();	
	if (m_menustyle.framefont.fontStruct())
		gcv.font = m_menustyle.framefont.fontStruct()->fid;
		
	XChangeGC(m_display, m_menustyle.f_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.h_text.pixel();
	XChangeGC(m_display, m_menustyle.h_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.d_text.pixel();
	XChangeGC(m_display, m_menustyle.d_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_menustyle.hilite.color().pixel();
	XChangeGC(m_display, m_menustyle.hilite_gc,
			gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.l_text.pixel();
	if (m_toolbarstyle.font.fontstruct)
		gcv.font = m_toolbarstyle.font.fontstruct->fid;
		
	XChangeGC(m_display, m_toolbarstyle.l_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.w_text.pixel();
	XChangeGC(m_display, m_toolbarstyle.w_text_gc,
		gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.c_text.pixel();
	XChangeGC(m_display, m_toolbarstyle.c_text_gc,
			gc_value_mask, &gcv);

	gcv.foreground = m_toolbarstyle.b_pic.pixel();
	XChangeGC(m_display, m_toolbarstyle.b_pic_gc,
			gc_value_mask, &gcv);
			
}

XFontSet Theme::createFontSet(char *fontname) {
	XFontSet fs;
	const int FONT_ELEMENT_SIZE=50;
	char **missing, *def = "-";
	int nmissing, pixel_size = 0, buf_size = 0;
	char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

	fs = XCreateFontSet(m_display,
					fontname, &missing, &nmissing, &def);
	if (fs && (! nmissing)) return fs;

#ifdef		HAVE_SETLOCALE
	if (! fs) {
		if (nmissing) XFreeStringList(missing);

		setlocale(LC_CTYPE, "C");
		fs = XCreateFontSet(m_display, fontname,
			&missing, &nmissing, &def);
		setlocale(LC_CTYPE, "");
	}
#endif // HAVE_SETLOCALE

	if (fs) {
		XFontStruct **fontstructs;
		char **fontnames;
		XFontsOfFontSet(fs, &fontstructs, &fontnames);
		fontname = fontnames[0];
	}

	getFontElement(fontname, weight, FONT_ELEMENT_SIZE,
		 "-medium-", "-bold-", "-demibold-", "-regular-", 0);
	getFontElement(fontname, slant, FONT_ELEMENT_SIZE,
		 "-r-", "-i-", "-o-", "-ri-", "-ro-", 0);
	getFontSize(fontname, &pixel_size);

	if (! strcmp(weight, "*")) 
		strncpy(weight, "medium", FONT_ELEMENT_SIZE);
	if (! strcmp(slant, "*")) 
		strncpy(slant, "r", FONT_ELEMENT_SIZE);
	if (pixel_size < 3) 
		pixel_size = 3;
	else if (pixel_size > 97) 
		pixel_size = 97;

	buf_size = strlen(fontname) + (FONT_ELEMENT_SIZE * 2) + 64;
	char *pattern2 = new char[buf_size];
	snprintf(pattern2, buf_size - 1,
		 "%s,"
		 "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
		 "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
		 fontname, weight, slant, pixel_size, pixel_size);
	fontname = pattern2;

	if (nmissing)
		XFreeStringList(missing);
	if (fs)
		XFreeFontSet(m_display, fs);

	fs = XCreateFontSet(m_display, fontname,
					&missing, &nmissing, &def);
	delete [] pattern2;

	return fs;
}

const char *Theme::getFontSize(const char *pattern, int *size) {
	const char *p;
	const char *p2=0;
	int n=0;

	for (p=pattern; 1; p++) {
		if (!*p) {
			if (p2!=0 && n>1 && n<72) {
				*size = n; return p2+1;
			} else {
				*size = 16; return 0;
			}
		} else if (*p=='-') {
			if (n>1 && n<72 && p2!=0) {
				*size = n;
				return p2+1;
			}
			p2=p; n=0;
		} else if (*p>='0' && *p<='9' && p2!=0) {
			n *= 10;
			n += *p-'0';
		} else {
			p2=0; n=0;
		}
	}
}

const char *Theme::getFontElement(const char *pattern, char *buf, int bufsiz, ...) {
	const char *p, *v;
	char *p2;
	va_list va;

	va_start(va, bufsiz);
	buf[bufsiz-1] = 0;
	buf[bufsiz-2] = '*';
	while((v = va_arg(va, char *)) != 0) {
		p = StringUtil::strcasestr(pattern, v);
		if (p) {
			strncpy(buf, p+1, bufsiz-2);
			p2 = strchr(buf, '-');
			if (p2) *p2=0;
			va_end(va);
			return p;
		}
	}
	va_end(va);
	strncpy(buf, "*", bufsiz);
	return 0;
}
