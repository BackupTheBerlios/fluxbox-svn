// IconBar.cc for Fluxbox Window Manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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

// $Id: IconBar.cc,v 1.10 2002/02/07 14:45:02 fluxgen Exp $

#include "IconBar.hh"
#include "i18n.hh"
#include "Screen.hh"

#include <algorithm>

IconBarObj::IconBarObj(FluxboxWindow *fluxboxwin, Window iconwin)
{
	m_fluxboxwin = fluxboxwin;
	m_iconwin = iconwin;
}

IconBarObj::~IconBarObj() {
	
}

IconBar::IconBar(BScreen *scrn, Window parent):
m_screen(scrn),
m_parent(parent)
{
	m_display = scrn->getBaseDisplay()->getXDisplay();
}

IconBar::~IconBar() {
}

//------------ addIcon -----------------------
// Adds icon to iconbar and repostions the 
// icons.
// returns window to iconobj
//--------------------------------------------
Window IconBar::addIcon(FluxboxWindow *fluxboxwin) {
	
	Window iconwin = createIconWindow(fluxboxwin, m_parent);
	decorate(iconwin);	
	//add window object to list	
	m_iconlist.push_back(new IconBarObj(fluxboxwin, iconwin));
	//reposition all icons to fit windowbar
	repositionIcons();
	
	XMapSubwindows(m_display, iconwin);
	XMapWindow(m_display, iconwin);	
	
	return iconwin;
}

//----------- delIcon -------------------
// Removes icon from list and
// repositions the rest of the icons
// Return X Window of the removed iconobj
// returns None if no window was found
//---------------------------------------
Window IconBar::delIcon(FluxboxWindow *fluxboxwin) {
	Window retwin = None;
	IconBarObj *obj = findIcon(fluxboxwin);
	if (obj) {
		IconList::iterator it =
			std::find(m_iconlist.begin(), m_iconlist.end(), obj);
		if (it != m_iconlist.end()) {
			m_iconlist.erase(it);								
			retwin = obj->getIconWin();		
			delete obj;				
			XDestroyWindow(m_display, retwin);
			repositionIcons();		
		}
	}		
	return retwin;
}

//------------ loadTheme ---------------
// renders theme to m_focus_pm
// with the size width * height
//--------------------------------------
void IconBar::loadTheme(unsigned int width, unsigned int height) {
	BImageControl *image_ctrl = m_screen->getImageControl();
	Pixmap tmp = m_focus_pm;
	BTexture *texture = &(m_screen->getWindowStyle()->tab.l_focus);
	
	//If we are working on a PARENTRELATIVE, change to right focus value
	if (texture->getTexture() & BImage::PARENTRELATIVE ) {
		texture = &(m_screen->getWindowStyle()->tab.t_focus);
	}
		
	if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
  		m_focus_pm = None;
	    	m_focus_pixel = texture->getColor()->getPixel();
  	} else {
		m_focus_pm =
		  image_ctrl->renderImage(width, height, texture);	 
	}

	if (tmp) image_ctrl->removeImage(tmp);
}

//------------ decorate ------------------
// sets the background pixmap/color, 
// border, border width of the window
//----------------------------------------
void IconBar::decorate(Window win) {

	XSetWindowBorderWidth(m_display, win, m_screen->getWindowStyle()->tab.border_width);
	XSetWindowBorder(m_display, win, m_screen->getWindowStyle()->tab.border_color.getPixel());
	if (m_focus_pm)
		XSetWindowBackgroundPixmap(m_display, win, m_focus_pm);
	else
		XSetWindowBackground(m_display, win, m_focus_pixel);
}


//------------ reconfigure ---------------
// Reconfigures the icons 
// theme, pos and width
//----------------------------------------
void IconBar::reconfigure() {	
	repositionIcons();
	
}
//---------- exposeEvent -----------------
// Handles the expose event
// just redraws all the icons
//----------------------------------------
void IconBar::exposeEvent(XExposeEvent *ee) {
	IconBarObj *obj=0;	
	IconList::iterator it = m_iconlist.begin();
	IconList::iterator it_end = m_iconlist.end();
	for (; it != it_end; ++it) {
		if ((*it)->getIconWin() == ee->window) {
			obj = (*it);
			break;
		}
	}	

	if (obj) {

		Window root;
		unsigned int width, height;
		unsigned int border_width, depth;	//not used
		int x, y;
		XGetGeometry(m_display, m_parent, &root, &x, &y, &width, &height,
					&border_width, &depth);

		//max width on every icon
		unsigned int icon_width = width / m_iconlist.size();
	
		//load right size of theme
		loadTheme(icon_width, height);
				
		draw(obj, icon_width);
		
	}	
}

//------------ repositionIcons ------------
// Calculates and moves/resizes the icons
//-----------------------------------------
void IconBar::repositionIcons(void) {
	if (m_iconlist.size() == 0)
		return;
		
	Window root;
	unsigned int width, height;
	unsigned int border_width, depth;	//not used
	int x, y;
	XGetGeometry(m_display, m_parent, &root, &x, &y, &width, &height,
		&border_width, &depth);
	
	//max width on every icon
	unsigned int icon_width = width / m_iconlist.size();
	
	//load right size of theme
	loadTheme(icon_width, height);

	IconList::iterator it = m_iconlist.begin();
	IconList::iterator it_end = m_iconlist.end();
	for (x = 0; it != it_end; ++it, x += icon_width) {
		Window iconwin = (*it)->getIconWin();
		XMoveResizeWindow(m_display, iconwin,
			 x, 0,
			 icon_width, height);	
		draw((*it), icon_width);
		decorate(iconwin);
	}
		
}

//------------ createIconWindow ----------------
// creates the X Window of icon
// returns the created X Window
//----------------------------------------------
Window IconBar::createIconWindow(FluxboxWindow *fluxboxwin, Window parent) {
	unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
			CWColormap | CWOverrideRedirect | CWEventMask;
	XSetWindowAttributes attrib;
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
		fluxboxwin->getScreen()->getWindowStyle()->tab.border_color.getPixel();
	attrib.colormap = fluxboxwin->getScreen()->getColormap();
	attrib.override_redirect = True;
	attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
		ButtonMotionMask | ExposureMask | EnterWindowMask;
											
	//create iconwindow
	Window iconwin = XCreateWindow(m_display, parent, 0, 0, 1, 1, 0,
		fluxboxwin->getScreen()->getDepth(), InputOutput, fluxboxwin->getScreen()->getVisual(),
		attrib_mask, &attrib);
		
	return iconwin;
}

//------------ draw ------------------
// Draws theme and string to Window w
//------------------------------------
void IconBar::draw(IconBarObj *obj, int width) {
	if (!obj)
		return;
	
	FluxboxWindow *fluxboxwin = obj->getFluxboxWin();
	Window iconwin = obj->getIconWin();
	char *title = *fluxboxwin->getIconTitle();
	unsigned int title_len = strlen(title);
	unsigned int title_text_w;

	const int multibyte = I18n::instance()->multibyte();

	if (multibyte) {
		XRectangle ink, logical;
		XmbTextExtents(m_screen->getWindowStyle()->font.set,
				title, title_len, &ink, &logical);
		title_text_w = logical.width;
	} else {
		title_text_w = XTextWidth(m_screen->getWindowStyle()->font.fontstruct,
				title, title_len);
	}
	
	int l = title_text_w;
	int dlen=title_len;
	unsigned int bevel_w = m_screen->getBevelWidth();
	int dx=bevel_w*2;
		
	for (; dlen >= 0; dlen--) {
		if (multibyte) {
		XRectangle ink, logical;
			XmbTextExtents(m_screen->getWindowStyle()->tab.font.set, 
										title, dlen,
										&ink, &logical);
			l = logical.width;
		} else
			l = XTextWidth(m_screen->getWindowStyle()->tab.font.fontstruct, 
								title, dlen);
			l += (bevel_w * 4);

		if (l < width)
			break;
	}

	switch (m_screen->getWindowStyle()->tab.font.justify) {
	case DrawUtil::Font::RIGHT:
		dx += width - l;
		break;
	case DrawUtil::Font::CENTER:
		dx += (width - l) / 2;
		break;
	default:
		break;
	}

	//Draw title to m_iconwin

	XClearWindow(m_display, iconwin);		
	
	if (multibyte) {
		XmbDrawString(m_display, iconwin,
			m_screen->getWindowStyle()->tab.font.set,
			m_screen->getWindowStyle()->tab.l_text_focus_gc, dx, 
			 1 - m_screen->getWindowStyle()->tab.font.set_extents->max_ink_extent.y,
			title, dlen);
	} else {
		XDrawString(m_display, iconwin,
			m_screen->getWindowStyle()->tab.l_text_focus_gc, dx,
			m_screen->getWindowStyle()->tab.font.fontstruct->ascent + 1, 
			title, dlen);
	}	

}

//------------ findWindow ----------
// Tries to find the FluxboxWindow of the X Window
// in iconbar
// returns the fluxboxwindow on success else
// 0 on failure
//----------------------------------
FluxboxWindow *IconBar::findWindow(Window w) {

	IconList::iterator it = m_iconlist.begin();
	IconList::iterator it_end = m_iconlist.end();
	for (; it != it_end; ++it) {
		IconBarObj *tmp = (*it);
		if (tmp)
			if (tmp->getIconWin() == w)
				return tmp->getFluxboxWin();			
	}
	
	return 0;
}

//----------- findIcon ---------------
// Tries to find a fluxboxwin icon in the iconlist
// returns pointer to IconBarObj on success else 
// 0 on failure
//------------------------------------

IconBarObj *IconBar::findIcon(FluxboxWindow *fluxboxwin) {

	IconList::iterator it = m_iconlist.begin();
	IconList::iterator it_end = m_iconlist.end();
	for (; it != it_end; ++it) {
		IconBarObj *tmp = (*it);
		if (tmp)
			if (tmp->getFluxboxWin() == fluxboxwin)
				return tmp;			
	}
	
	return 0;
}

//---------- getIconWidth ------------
// will return the width of an icon
// window
//------------------------------------
unsigned int IconBarObj::getWidth() {
	Window root;

	unsigned int width, height;
	unsigned int border_width, depth;	//not used
	int x, y; //not used

	Display *m_display = Fluxbox::instance()->getXDisplay();

	XGetGeometry(m_display, m_iconwin, &root, &x, &y, 
					&width, &height, &border_width, &depth);

	return width;
}
