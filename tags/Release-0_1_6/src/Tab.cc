// Tab.cc for Fluxbox Window Manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Tab.cc,v 1.14 2002/01/09 14:11:20 fluxgen Exp $

#include "Tab.hh"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "DrawUtil.hh"

#include <iostream>
using namespace std;

bool Tab::m_stoptabs = false;
Tab::t_tabplacementlist Tab::m_tabplacementlist[] = {
		{PTOP, "Top"},
		{PBOTTOM, "Bottom"},
		{PLEFT, "Left"},
		{PRIGHT, "Right"},
		{PNONE, "none"}
	};

Tab::t_tabplacementlist Tab::m_tabalignmentlist[] = {
		{ALEFT, "Left"},
		{ACENTER, "Center"},
		{ARIGHT, "Right"},
		{ARELATIVE, "Relative"},
		{ANONE, "none"}
	};

Tab::Tab(FluxboxWindow *win, Tab *prev, Tab *next) { 
	//set default values
	
	m_focus = m_moving = false;
	m_configured = true; // only set to false before Fluxbox::reconfigure
	m_move_x = m_move_y = 0;	
	m_prev = prev; m_next = next; 
	m_win = win;
	m_display = Fluxbox::instance()->getXDisplay();
	
	if ((m_win->getScreen()->getTabPlacement() == PLEFT ||
			m_win->getScreen()->getTabPlacement() == PRIGHT) &&
			m_win->getScreen()->isTabRotateVertical()) {
		m_size_w = m_win->getScreen()->getTabHeight();
		m_size_h = m_win->getScreen()->getTabWidth();
	} else {
		m_size_w = m_win->getScreen()->getTabWidth();
		m_size_h = m_win->getScreen()->getTabHeight();
	}

	createTabWindow();

	calcIncrease();
}

Tab::~Tab() {

	disconnect();	
	
	Fluxbox::instance()->removeTabSearch(m_tabwin);
	XDestroyWindow(m_display, m_tabwin);
}


//----------------  createTabWindow ---------------
// (private)
// Creates the Window for tab to be above the title window.
// This should only be called by the constructor.
//-------------------------------------------------
void Tab::createTabWindow() {
	unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
			      CWColormap | CWOverrideRedirect | CWEventMask;
	XSetWindowAttributes attrib;
	attrib.background_pixmap = None;
	attrib.background_pixel = attrib.border_pixel =
			m_win->getScreen()->getWindowStyle()->tab.border_color.getPixel();
	attrib.colormap = m_win->getScreen()->getColormap();
	attrib.override_redirect = True;
	attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
						ButtonMotionMask | ExposureMask | EnterWindowMask;
	//Notice that m_size_w gets the TOTAL width of tabs INCLUDING borders
	m_tabwin = XCreateWindow(m_display, m_win->getScreen()->getRootWindow(), 
							-30000, -30000, //TODO: So that it wont flicker or
											// appear before the window do
			m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
							m_win->getScreen()->getWindowStyle()->tab.border_width,
							m_win->getScreen()->getDepth(), InputOutput,
							m_win->getScreen()->getVisual(), attrib_mask, &attrib);
	//set grab
	XGrabButton(m_display, Button1, Mod1Mask, m_tabwin, True,
				ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
				GrabModeAsync, None, Fluxbox::instance()->getMoveCursor());

	//save to tabsearch
	Fluxbox::instance()->saveTabSearch(m_tabwin, this);	

	XMapSubwindows(m_display, m_tabwin);

	XMapWindow(m_display, m_tabwin);

	decorate();
}

//-------------- focus --------------------
// Called when the focus changes in m_win 
// updates pixmap or color and draws the tab
//-----------------------------------------
void Tab::focus() {

	if (m_win->isFocused()) {
		if (m_focus_pm)
			XSetWindowBackgroundPixmap(m_display, m_tabwin, m_focus_pm);
		else
			XSetWindowBackground(m_display, m_tabwin, m_focus_pixel);
	} else {
		if (m_unfocus_pm)
			XSetWindowBackgroundPixmap(m_display, m_tabwin, m_unfocus_pm);
		else
			XSetWindowBackground(m_display, m_tabwin, m_unfocus_pixel);
	}
	XClearWindow(m_display, m_tabwin);	
	draw(false);
}

//-------------- raise --------------------
// Raises the tabs in the tablist
//-----------------------------------------
void Tab::raise() {
	//get first tab
	Tab *first = 0;
	first = getFirst(this);
	//raise tabs
	for (; first!=0; first = first->m_next)
		m_win->getScreen()->raiseWindows(&first->m_tabwin, 1);
}

//-------------- loadTheme -----------------
// loads the texture with the correct
// width and height, this is necessary in
// vertical and relative tab modes
// TODO optimize this
//------------------------------------------
void Tab::loadTheme() {
	BImageControl *image_ctrl = m_win->getScreen()->getImageControl();
	Pixmap tmp = m_focus_pm;
	BTexture *texture = &(m_win->getScreen()->getWindowStyle()->tab.l_focus);

 	if (texture->getTexture() & BImage::PARENTRELATIVE ) {
		BTexture *pt = &(m_win->getScreen()->getWindowStyle()->tab.t_focus);
		if (pt->getTexture() == (BImage::FLAT | BImage::SOLID)) {
  		  m_focus_pm = None;
	    m_focus_pixel = pt->getColor()->getPixel();
  	} else
    	m_focus_pm =
	      image_ctrl->renderImage(m_size_w, m_size_h, pt);
  	if (tmp) image_ctrl->removeImage(tmp);
		
	} else {
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
  		  m_focus_pm = None;
	    m_focus_pixel = texture->getColor()->getPixel();
  	} else
    	m_focus_pm =
	      image_ctrl->renderImage(m_size_w, m_size_h, texture);
  	if (tmp) image_ctrl->removeImage(tmp);
	}
	
	tmp = m_unfocus_pm;
	texture = &(m_win->getScreen()->getWindowStyle()->tab.l_unfocus);
  
	if (texture->getTexture() & BImage::PARENTRELATIVE ) {
		BTexture *pt = &(m_win->getScreen()->getWindowStyle()->tab.t_unfocus);
		if (pt->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			m_unfocus_pm = None;
			m_unfocus_pixel = pt->getColor()->getPixel();
  		} else
			m_unfocus_pm =
			image_ctrl->renderImage(m_size_w, m_size_h, pt);
	} else {
		if (texture->getTexture() == (BImage::FLAT | BImage::SOLID)) {
			m_unfocus_pm = None;
			m_unfocus_pixel = texture->getColor()->getPixel();
  		} else
			m_unfocus_pm =
				image_ctrl->renderImage(m_size_w, m_size_h, texture);
	}
	
	if (tmp) image_ctrl->removeImage(tmp);
}

//-------------- decorate --------------------
// decorates the tab with current theme
//--------------------------------------------
void Tab::decorate() {
	loadTheme();

	XSetWindowBorderWidth(m_display, m_tabwin,
		m_win->getScreen()->getWindowStyle()->tab.border_width);
	XSetWindowBorder(m_display, m_tabwin,
		m_win->getScreen()->getWindowStyle()->tab.border_color.getPixel());
}

//-------------- deiconify -----------------
// Deiconifies the tab
// Used from FluxboxWindow to deiconify the tab when the window is deiconfied
//------------------------------------------
void Tab::deiconify() {
	XMapWindow(m_display, m_tabwin);
}

//------------- iconify --------------------
// Iconifies the tab.
// Used from FluxboxWindow to hide tab win when window is iconified
// disconnects itself from the list
//------------------------------------------
void Tab::iconify() {
	disconnect();
	withdraw();
}

//------------ withdraw --------------
// Unmaps the tab from display
//------------------------------------
void Tab::withdraw() {
	XUnmapWindow(m_display, m_tabwin);	
}

//------------ stick --------------------
// Set/reset the the sticky on all windows in the list
//---------------------------------------
void Tab::stick() {
 
	//get first tab
	Tab *first = 0;
	first = getFirst(this);
	
	//now do stick for all windows in the list
	for (; first!=0; first = first->m_next) {
		FluxboxWindow *win = first->m_win; //just for convenient
		if (win->isStuck()) {
			win->blackbox_attrib.flags ^= BaseDisplay::ATTRIB_OMNIPRESENT;
			win->blackbox_attrib.attrib ^= BaseDisplay::ATTRIB_OMNIPRESENT;
			win->stuck = false;
			if (!win->isIconic())
      	win->getScreen()->reassociateWindow(win, -1, true);
				    
		} else {
			win->stuck = true;
			win->blackbox_attrib.flags |= BaseDisplay::ATTRIB_OMNIPRESENT;
			win->blackbox_attrib.attrib |= BaseDisplay::ATTRIB_OMNIPRESENT;
		}
		
		win->setState(win->current_state);
	}	

}

//------------- resize -------------
// Resize the window's in the tablist
//----------------------------------
void Tab::resize() {
	Tab *first = getFirst(this);

	//now move and resize the windows in the list
	for (; first != 0; first = first->m_next) {
		if (first!=this) {
			first->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
								m_win->getWidth(), m_win->getHeight());
		}
	}

	// need to resize tabs if in relative mode
	if (m_win->getScreen()->getTabAlignment() == ARELATIVE) {
		calcIncrease();
		setPosition();
	}
}

//----------- shade --------------
// Shades the windows in the tablist
//--------------------------------
void Tab::shade() {
	for(Tab *first = getFirst(this); first != 0; first = first->m_next) {
		if (first==this)
			continue;
		first->m_win->shade();
	}

	if (m_win->getScreen()->getTabPlacement() == PLEFT ||
			m_win->getScreen()->getTabPlacement() == PRIGHT) { 
		resizeGroup();
		calcIncrease();
	}

	if (!(m_win->getScreen()->getTabPlacement() == PTOP))
		setPosition();
}

//------------ draw -----------------
// Draws the tab 
// if pressed = true then it draws the tab in pressed 
// mode else it draws it in normal mode
// TODO: the "draw in pressed mode" 
//-----------------------------------
void Tab::draw(bool pressed) {	
	unsigned int tabtext_w;

	GC gc = ((m_win->isFocused()) ? m_win->getScreen()->getWindowStyle()->tab.l_text_focus_gc :
		   m_win->getScreen()->getWindowStyle()->tab.l_text_unfocus_gc);

	// Different routines for drawing rotated text
	if ((m_win->getScreen()->getTabPlacement() == PLEFT ||
			m_win->getScreen()->getTabPlacement() == PRIGHT) &&
			(!m_win->isShaded() && m_win->getScreen()->isTabRotateVertical())) {

		tabtext_w = DrawUtil::XRotTextWidth(m_win->getScreen()->getWindowStyle()->tab.rot_font,
						m_win->client.title, m_win->client.title_len);
		tabtext_w += (m_win->frame.bevel_w * 4);

		DrawUtil::DrawRotString(m_display, m_tabwin, gc,
				m_win->getScreen()->getWindowStyle()->tab.rot_font,
				m_win->getScreen()->getWindowStyle()->tab.font.justify,
				tabtext_w, m_size_w, m_size_h,
				m_win->frame.bevel_w, m_win->client.title);

	} else {
		if (I18n::instance()->multibyte()) { // TODO: maybe move this out from here?
			XRectangle ink, logical;
			XmbTextExtents(m_win->getScreen()->getWindowStyle()->tab.font.set,
					m_win->client.title, m_win->client.title_len,
					&ink, &logical);
			tabtext_w = logical.width;
		} else {
			tabtext_w = XTextWidth(
					m_win->getScreen()->getWindowStyle()->tab.font.fontstruct,
					m_win->client.title, m_win->client.title_len);
		}
		tabtext_w += (m_win->frame.bevel_w * 4);

		DrawUtil::DrawString(m_display, m_tabwin, gc,
				&m_win->getScreen()->getWindowStyle()->tab.font,
				tabtext_w, m_size_w,
				m_win->frame.bevel_w, m_win->client.title);
	}
}

//------------- setPosition -----------------
// Position tab ( follow the m_win pos ).
// (and resize)
// Set new position of the other tabs in the chain
//-------------------------------------------
void Tab::setPosition() {	
	//don't do anything if the tablist is freezed
	if (m_stoptabs)
		return;

	Tab *first = 0;
	int pos_x = 0, pos_y = 0;
	
	m_stoptabs = true; //freeze tablist
	
	//and check for max tabs
	//TODO: optimize! There is many ways to implement this...		
	//posible movement to a static member function

	//Tab placement
	if (m_win->getScreen()->getTabPlacement() == PTOP) {
		pos_y = m_win->getYFrame() - m_size_h; 

	} else if (m_win->getScreen()->getTabPlacement() == PBOTTOM ||
				m_win->isShaded()) { 
		if (m_win->isShaded())
			pos_y = m_win->getYFrame() + m_win->getTitleHeight() +
					m_win->getScreen()->getBorderWidth2x();

		else
			pos_y = m_win->getYFrame() + m_win->getHeight() +
					m_win->getScreen()->getBorderWidth2x();

	} else if (m_win->getScreen()->getTabPlacement() == PLEFT) {
		pos_x = m_win->getXFrame() - m_size_w;
			
	} else if (m_win->getScreen()->getTabPlacement() == PRIGHT) {
		pos_x = m_win->getXFrame() + m_win->getWidth() +
				m_win->getScreen()->getBorderWidth2x();	
	}

	//Tab alignment
	if (m_win->getScreen()->getTabPlacement() == PTOP ||
			m_win->getScreen()->getTabPlacement() == PBOTTOM ||
			m_win->isShaded()) {
		switch(m_win->getScreen()->getTabAlignment()) {
			case ARELATIVE:
			case ALEFT:
				pos_x = m_win->getXFrame(); 
				break;
			case ACENTER:
				pos_x = calcCenterXPos(); 
				break;
			case ARIGHT:
				pos_x = m_win->getXFrame() + m_win->getWidth() +
						m_win->getScreen()->getBorderWidth2x() - m_size_w;
				break;
		}
	} else { //PLeft | PRight
		switch(m_win->getScreen()->getTabAlignment()) {
			case ALEFT:
				pos_y = m_win->getYFrame() - m_size_h + m_win->getHeight() +
						m_win->getScreen()->getBorderWidth2x();
				break;
			case ACENTER:
				pos_y = calcCenterYPos();
				break;
			case ARELATIVE:
			case ARIGHT:
				pos_y = m_win->getYFrame();
				break;
		}
	}
	
	for (first = getFirst(this);
			first!=0; 
			pos_x += first->m_inc_x, pos_y += first->m_inc_y,
			first = first->m_next){
		
		XMoveWindow(m_display, first->m_tabwin, pos_x, pos_y);
				
		//dont move fluxboxwindow if the itterator = this
		if (first!=this) {
			first->m_win->configure(m_win->getXFrame(), m_win->getYFrame(), 
					m_win->getWidth(), m_win->getHeight());
		}	
	}	

	m_stoptabs = false;
}

//------------- calcIncrease ----------------
// calculates m_inc_x and m_inc_y for tabs
// used for positioning the tabs.
//-------------------------------------------
void Tab::calcIncrease(void) {
	#ifdef DEBUG
	cerr << "Calculating tab increase" << endl;
	#endif // DEBUG

	Tab *first;
	int inc_x = 0, inc_y = 0;
	unsigned int tabs = 0, i = 0;

	if (m_win->getScreen()->getTabPlacement() == PTOP ||
			m_win->getScreen()->getTabPlacement() == PBOTTOM ||
			m_win->isShaded()) {
		inc_y = 0;

		switch(m_win->getScreen()->getTabAlignment()) {
			case ALEFT:
				inc_x = m_size_w;
				break;
			case ACENTER:
				inc_x = m_size_w;
				break;
			case ARIGHT:
				inc_x = -m_size_w;
				break;
			case ARELATIVE:
				inc_x = calcRelativeWidth();
				break;
		}
	} else if (m_win->getScreen()->getTabPlacement() == PLEFT ||
					m_win->getScreen()->getTabPlacement() == PRIGHT) {
		inc_x = 0;

		switch(m_win->getScreen()->getTabAlignment()) {
			case ALEFT:
				inc_y = -m_size_h;
				break;
			case ACENTER:
				inc_y = m_size_h;
				break;
			case ARIGHT:
				inc_y = m_size_h;
				break;
			case ARELATIVE:
				inc_y = calcRelativeHeight();
				break;
		}
	}

	for (first = getFirst(this); first!=0; first = first->m_next, tabs++);

	for (first = getFirst(this); first!=0; first = first->m_next, i++){

	//TODO: move this out from here?
		if ((m_win->getScreen()->getTabPlacement() == PTOP ||
				m_win->getScreen()->getTabPlacement() == PBOTTOM ||
				m_win->isShaded()) &&
				m_win->getScreen()->getTabAlignment() == ARELATIVE) {
			if (!((m_win->getWidth() +
					m_win->getScreen()->getBorderWidth2x()) % tabs) ||
					i >= ((m_win->getWidth() +
					m_win->getScreen()->getBorderWidth2x()) % tabs)) {
				first->setTabWidth(inc_x);
				first->m_inc_x = inc_x;
			} else { // adding 1 extra pixel to get tabs like win width
				first->setTabWidth(inc_x + 1);
				first->m_inc_x = inc_x + 1;
			}
			first->m_inc_y = inc_y;
		} else if (m_win->getScreen()->getTabAlignment() == ARELATIVE) {
			if (!((m_win->getHeight() +
				m_win->getScreen()->getBorderWidth2x()) % tabs) ||
				i >= ((m_win->getHeight() +
				m_win->getScreen()->getBorderWidth2x()) % tabs)) {

				first->setTabHeight(inc_y);
				first->m_inc_y = inc_y;
			} else {
				// adding 1 extra pixel to get tabs match window width
				first->setTabHeight(inc_y + 1);
				first->m_inc_y = inc_y + 1;
			}
			first->m_inc_x = inc_x;
		} else { // non relative modes
			first->m_inc_x = inc_x;
			first->m_inc_y = inc_y;
		}
	}
}

//------------- buttonPressEvent -----------
// Handle button press event here.
//------------------------------------------
void Tab::buttonPressEvent(XButtonEvent *be) {	
	//draw in pressed mode
	draw(true);
	
	//set window to titlewindow so we can take advatage of drag function
	be->window = m_win->frame.title;
	
	//call windows buttonpress eventhandler
	m_win->buttonPressEvent(be);
}

//----------- buttonReleaseEvent ----------
// Handle button release event here.
// If tab is dropped then it should try to find
// the window where the tab where dropped.
//-----------------------------------------
void Tab::buttonReleaseEvent(XButtonEvent *be) {		
	
	if (m_moving) {
		
		m_moving = false; 
		
		//erase tabmoving rectangle
		XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(),
			m_win->getScreen()->getOpGC(),
			m_move_x, m_move_y, 
			m_size_w, m_size_h);
		
		Fluxbox::instance()->ungrab();
		XUngrabPointer(m_display, CurrentTime);
		
		//storage of window and pos of window where we dropped the tab
		Window child;
		int dest_x = 0, dest_y = 0;
		
		//find window on coordinates of buttonReleaseEvent
		if (XTranslateCoordinates(m_display, m_win->getScreen()->getRootWindow(), 
							m_win->getScreen()->getRootWindow(),
							be->x_root, be->y_root, &dest_x, &dest_y, &child)) {
			
			Tab *tab = 0;
			FluxboxWindow *win = 0;
			//search tablist for a tabwindow
			if (((tab = Fluxbox::instance()->searchTab(child))!=0) ||
					(m_win->getScreen()->isSloppyWindowGrouping() &&
					((win = Fluxbox::instance()->searchWindow(child))!=0) &&
					(tab = win->getTab())!=0)) {

				if (tab == this) // inserting ourself to ourself causes a disconnect
					return;

				// do only attach a hole chain if we dropped the
				// first tab in the dropped chain...
				if (m_prev)
					disconnect();
				
				// attach this tabwindow chain to the tabwindow chain we found.
				tab->insert(this);

			} else {	
				disconnect();

				// convinience
				unsigned int placement = m_win->getScreen()->getTabPlacement();

				// (ab)using dest_x and dest_y
				dest_x = be->x_root;
				dest_y = be->y_root;

				if (placement == PTOP || placement == PBOTTOM || m_win->isShaded()) {
					if (placement == PBOTTOM && !m_win->isShaded())
						dest_y -= m_win->getHeight();
					else if (placement != PTOP && m_win->isShaded())
						dest_y -= m_win->getTitleHeight();
					else // PTOP
						dest_y += m_win->getTitleHeight();

					switch(m_win->getScreen()->getTabAlignment()) {
						case ACENTER:
							dest_x -= (m_win->getWidth() / 2) - (m_size_w / 2);
							break;
						case ARIGHT:
							dest_x -= m_win->getWidth() - m_size_w;
							break;
					}

				} else { // PLEFT & PRIGHT
					if (placement == PRIGHT)
						dest_x = be->x_root - m_win->getWidth();

					switch(m_win->getScreen()->getTabAlignment()) {
						case ACENTER:
							dest_y -= (m_win->getHeight() / 2) - (m_size_h / 2);
							break;
						case ALEFT:
							dest_y -= m_win->getHeight() - m_size_h;
							break;
					}
				}
				//TODO: this causes an calculate increase event, even if we
				// only are moving a window
				m_win->configure(dest_x, dest_y, m_win->getWidth(), m_win->getHeight());
			}
		}
	} else {
		
		//raise this tabwindow
		raise();
		
		//set window to title window soo we can use m_win handler for menu
		be->window = m_win->frame.title;
		
		//call windows buttonrelease event handler so it can popup a menu if needed
		m_win->buttonReleaseEvent(be);
	}
	
}

//------------- exposeEvent ------------
// Handle expose event here.
// Draws the tab unpressed
//--------------------------------------
void Tab::exposeEvent(XExposeEvent *ee) {
	draw(false);
}

//----------- motionNotifyEvent --------
// Handles motion event here
// Draws the rectangle of moving tab
//--------------------------------------
void Tab::motionNotifyEvent(XMotionEvent *me) {
	
	Fluxbox *fluxbox = Fluxbox::instance();
	
	//if mousebutton 2 is pressed
	if (me->state & Button2Mask) {
		if (!m_moving) {
			m_moving = true; 
			
			XGrabPointer(m_display, me->window, False, Button2MotionMask |
								ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
								None, fluxbox->getMoveCursor(), CurrentTime);

			fluxbox->grab();

			m_move_x = me->x_root - 1;
			m_move_y = me->y_root - 1;
				
			XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(), 
					m_win->getScreen()->getOpGC(),
					m_move_x, m_move_y,
					m_size_w, m_size_h);
	
		} else {
		
			int dx = me->x_root - 1, dy = me->y_root - 1;

			dx -= m_win->getScreen()->getBorderWidth();
			dy -= m_win->getScreen()->getBorderWidth();

			if (m_win->getScreen()->getEdgeSnapThreshold()) {
				int drx = m_win->getScreen()->getWidth() - (dx + 1);

				if (dx > 0 && dx < drx && dx < m_win->getScreen()->getEdgeSnapThreshold()) 
					dx = 0;
				else if (drx > 0 && drx < m_win->getScreen()->getEdgeSnapThreshold())
					dx = m_win->getScreen()->getWidth() - 1;

				int dtty, dbby, dty, dby;
		
				switch (m_win->getScreen()->getToolbarPlacement()) {
				case Toolbar::TOPLEFT:
				case Toolbar::TOPCENTER:
				case Toolbar::TOPRIGHT:
					dtty = m_win->getScreen()->getToolbar()->getExposedHeight() +
							m_win->getScreen()->getBorderWidth();
					dbby = m_win->getScreen()->getHeight();
					break;

				default:
					dtty = 0;
					dbby = m_win->getScreen()->getToolbar()->getY();
					break;
				}
		
				dty = dy - dtty;
				dby = dbby - (dy + 1);

				if (dy > 0 && dty < m_win->getScreen()->getEdgeSnapThreshold())
					dy = dtty;
				else if (dby > 0 && dby < m_win->getScreen()->getEdgeSnapThreshold())
					dy = dbby - 1;
		
			}
		
			//erase rectangle
			XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(),
					m_win->getScreen()->getOpGC(),
					m_move_x, m_move_y, 
					m_size_w, m_size_h);

			//redraw rectangle at new pos
			m_move_x = dx;
			m_move_y = dy;			
			XDrawRectangle(m_display, m_win->getScreen()->getRootWindow(), 
					m_win->getScreen()->getOpGC(),
					m_move_x, m_move_y,
					m_size_w, m_size_h);					
			
		}
	} 
}

//-------------- getFirst() ---------
// Returns the first Tab in the chain 
// of currentchain. 
//-----------------------------------
Tab *Tab::getFirst(Tab *current) {
	if (!current)
		return 0;
		
	Tab *i=current;
	
	for (; i->m_prev != 0; i = i->m_prev);
	return i;
}

//-------------- getLast() ---------
// Returns the last Tab in the chain 
// of currentchain. 
//-----------------------------------
Tab *Tab::getLast(Tab *current) {
	if (!current)
		return 0;
	Tab *i=current;
	
	for (; i->m_next != 0; i = i->m_next);
	return i;
}

//-------------- insert ------------
// (private)
// Inserts a tab in the chain
//----------------------------------
void Tab::insert(Tab *tab) {
	
	if (!tab || tab == this) //dont insert if the tab = 0 or the tab = this
		return;

	Tab *first = getFirst(this);
	
	//if the tab already in chain then disconnect it
	for (; first!=0; first = first->m_next) {
		if (first==tab) {
			#ifdef DEBUG
			cerr<<"Tab already in chain. Disconnecting!"<<endl;			
			#endif // DEBUG
			tab->disconnect();
			break;
		}
	}

	//get last tab in the chain to be inserted
	Tab *last = tab;
	for (; last->m_next!=0; last=last->m_next); 
	//do sticky before we connect it to the chain
	//sticky bit on window
	if (m_win->isStuck() && !tab->m_win->isStuck() ||
			!m_win->isStuck() && tab->m_win->isStuck())
			tab->m_win->stick(); //this will set all the m_wins in the list
	
	//connect the tab to this chain
	
	if (m_next)	
		m_next->m_prev = last;
	tab->m_prev = this; 
	last->m_next = m_next; 

	m_next = tab;	

	bool resize_tabs = false;

	//TODO: cleanup and optimize
	//move and resize all windows in the tablist we inserted
	//only from first tab of the inserted chain to the last
	for (; tab!=last->m_next; tab=tab->m_next) {
		if (m_win->isShaded() != tab->m_win->isShaded()) {
			tab->m_stoptabs = true; // we don't want any actions performed on the
															// tabs, just the tab windows!
			if (m_win->getScreen()->getTabPlacement() == PLEFT ||
					m_win->getScreen()->getTabPlacement() == PRIGHT)
				resize_tabs = true;

			// if the window we are grouping to, we need to shade the tab window
			// _after_ reconfigure
			if(m_win->isShaded()) {
					tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
							m_win->getWidth(), m_win->getHeight());
					tab->m_win->shade();
			} else {
					tab->m_win->shade(); // switch to correct shade state
					tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
						m_win->getWidth(), m_win->getHeight());
			}

			tab->m_stoptabs = false;

		// both window have the same shaded state and have different sizes,
		// checking this so that I'll only do shade on windows if configure did
		// anything.
		} else if ((m_win->getWidth() != tab->m_win->getWidth()) ||
				(m_win->getHeight() != tab->m_win->getHeight())) {

			tab->m_win->configure(m_win->getXFrame(), m_win->getYFrame(),
				m_win->getWidth(), m_win->getHeight());

			// need to shade the tab window as configure will mess it up
			if (m_win->isShaded())
				tab->m_win->shade();
		}
	}

	// resize if in relative mode or resize_tabs is true
	if(m_win->getScreen()->getTabAlignment() == ARELATIVE ||
			resize_tabs) {
		resizeGroup();
		calcIncrease();
	}
	// reposition tabs
	setPosition();
}

//---------- disconnect() --------------
// Disconnects the tab from any chain
//--------------------------------------
void Tab::disconnect() {
	Tab *tmp = 0;
	if (m_prev) {	//if this have a chain to "the left" (previous tab) then set it's next to this next
		m_prev->m_next = m_next;
		tmp = m_prev;
	} 
	if (m_next) {	//if this have a chain to "the right" (next tab) then set it's prev to this prev
		m_next->m_prev = m_prev;
		tmp = m_next;
	}

	//mark as no chain, previous and next.
	m_prev = 0;
	m_next = 0;
	
	//reposition the tabs
	if (tmp) {
		if (m_win->getScreen()->getTabAlignment() == ARELATIVE)
			tmp->calcIncrease();
		tmp->setPosition();
	}

	if (m_win->getScreen()->getTabAlignment() == ARELATIVE)
		calcIncrease();
	
	setPosition();
}

// ------------ setTabWidth --------------
// Sets Tab width _including_ borders
// ---------------------------------------
void Tab::setTabWidth(unsigned int w) {
	if (w > m_win->getScreen()->getWindowStyle()->tab.border_width_2x &&
			w != m_size_w) {
		m_size_w = w;
		XResizeWindow(m_display, m_tabwin,
			m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x);

		loadTheme(); // rerender themes to right size
		focus(); // redraw the window
	}
}

// ------------ setTabHeight ---------
// Sets Tab height _including_ borders
// ---------------------------------------
void Tab::setTabHeight(unsigned int h) {
	if (h > m_win->getScreen()->getWindowStyle()->tab.border_width_2x &&
			h != m_size_h) {
		m_size_h = h;
		XResizeWindow(m_display, m_tabwin, 
			m_size_w - m_win->getScreen()->getWindowStyle()->tab.border_width_2x,
			m_size_h - m_win->getScreen()->getWindowStyle()->tab.border_width_2x);

		loadTheme(); // rerender themes to right size
		focus(); // redraw the window
	} 
}

// ------------ resizeGroup --------------
// This function is used when (un)shading
// to get right size/width of tabs when
// PLeft || PRight && isTabRotateVertical
// ---------------------------------------
void Tab::resizeGroup(void) {
	#ifdef DEBUG
	cerr << "Resising group" << endl;
	#endif //DEBUG
	Tab *first;
	for (first = getFirst(this); first != 0; first = first->m_next) {
		if ((m_win->getScreen()->getTabPlacement() == PLEFT ||
				m_win->getScreen()->getTabPlacement() == PRIGHT) &&
				m_win->getScreen()->isTabRotateVertical() &&
				!m_win->isShaded()) {
			first->setTabWidth(m_win->getScreen()->getTabHeight());
			first->setTabHeight(m_win->getScreen()->getTabWidth());
		} else {
			first->setTabWidth(m_win->getScreen()->getTabWidth());
			first->setTabHeight(m_win->getScreen()->getTabHeight());
		}
		//TODO: do I have to set this all the time?
		first->m_configured = true; //used in Fluxbox::reconfigure()
	}
}

//------------- calcRelativeWidth --------
// Returns: Calculated width for relative 
// alignment
//----------------------------------------
unsigned int Tab::calcRelativeWidth() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return ((m_win->getWidth() + m_win->getScreen()->getBorderWidth2x())/num);
}

//------------- calcRelativeHeight -------
// Returns: Calculated height for relative 
// alignment
//----------------------------------------
unsigned int Tab::calcRelativeHeight() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return ((m_win->getHeight() + m_win->getScreen()->getBorderWidth2x())/num);
}

//------------- calcCenterXPos -----------
// Returns: Calculated x position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterXPos() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return (m_win->getXFrame() + ((m_win->getWidth() - (m_size_w * num)) / 2));
}

//------------- calcCenterYPos -----------
// Returns: Calculated y position for 
// centered alignment
//----------------------------------------
unsigned int Tab::calcCenterYPos() {
	unsigned int num=0;
	//calculate num objs in list (extract this to a function?)
	for (Tab *first=getFirst(this); first!=0; first=first->m_next, num++);	

	return (m_win->getYFrame() + ((m_win->getHeight() - (m_size_h * num)) / 2));
}


//------- getTabPlacementString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else 0.
//----------------------------------------
const char *Tab::getTabPlacementString(int placement) {	
	for (int i=0; i<(PNONE / 5); i++) {
		if (m_tabplacementlist[i] == placement)
			return m_tabplacementlist[i].string;
	}
	return 0;
}

//------- getTabPlacementNum -------------
// Returns the tabplacement number of the 
// tabplacement string on success else
// the type none on failure.
//----------------------------------------
int Tab::getTabPlacementNum(const char *string) {
	for (int i=0; i<(PNONE / 5); i ++) {
		if (m_tabplacementlist[i] == string) {
			return m_tabplacementlist[i].tp;
		}
	}
	return PNONE;
}

//------- getTabAlignmentString ----------
// Returns the tabplacement string of the 
// tabplacement number on success else 0.
//----------------------------------------
const char *Tab::getTabAlignmentString(int placement) {	
	for (int i=0; i<ANONE; i++) {
		if (m_tabalignmentlist[i] == placement)
			return m_tabalignmentlist[i].string;
	}
	return 0;
}

//------- getTabAlignmentNum -------------
// Returns the tabplacement number of the 
// tabplacement string on success else
// the type none on failure.
//----------------------------------------
int Tab::getTabAlignmentNum(const char *string) {
	for (int i=0; i<ANONE; i++) {
		if (m_tabalignmentlist[i] == string) {
			return m_tabalignmentlist[i].tp;
		}
	}
	return ANONE;
}
