// Basemenu.hh for Fluxbox Window manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// Basemenu.hh for Blackbox - an X11 Window manager
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

// $Id: Basemenu.hh,v 1.14 2002/05/19 12:57:47 fluxgen Exp $

#ifndef	 BASEMENU_HH
#define	 BASEMENU_HH

#include <X11/Xlib.h>
#include <vector>
#include <string>

class Basemenu;
class BasemenuItem;
class Fluxbox;
class BImageControl;
class BScreen;

class Basemenu {
public:
	enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
	enum { RIGHT = 1, LEFT };
	enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };
	
	explicit Basemenu(BScreen *screen);
	virtual ~Basemenu();

	//manipulators
	int insert(const char *label, int function= 0, const char *exec = 0, int pos = -1);
	int insert(const char *label, Basemenu *submenu, int pos= -1);
	int remove(unsigned int item);
	inline void setInternalMenu() { internal_menu = true; }
	inline void setAlignment(Alignment a) { m_alignment = a; }
	inline void setTorn() { torn = true; }
	inline void removeParent() { if (internal_menu) m_parent = 0; }
	void buttonPressEvent(XButtonEvent *);
	void buttonReleaseEvent(XButtonEvent *);
	void motionNotifyEvent(XMotionEvent *);
	void enterNotifyEvent(XCrossingEvent *);
	void leaveNotifyEvent(XCrossingEvent *);
	void exposeEvent(XExposeEvent *);
	void reconfigure();
	void setLabel(const char *labelstr);
	void move(int x, int y);
	void update();
	void setItemSelected(unsigned int index, bool val);
	void setItemEnabled(unsigned int index, bool val);
	virtual void drawSubmenu(unsigned int index);
	virtual void show();
	virtual void hide();
	
	//accessors
	inline	bool isTorn() const { return torn; }
	inline	bool isVisible() const { return visible; }
	inline BScreen *screen() const { return m_screen; }
	inline Window windowID() const { return menu.window; }
	inline const std::string &label() const { return menu.label; }	
	inline int x() const { return menu.x; }
	inline int y() const { return menu.y; }
	inline unsigned int width() const { return menu.width; }
	inline unsigned int height() const { return menu.height; }
	inline unsigned int numberOfItems() const { return menuitems.size(); }
	inline int currentSubmenu() const { return which_sub; }	
	inline unsigned int titleHeight() const { return menu.title_h; }
	bool hasSubmenu(unsigned int index) const;
	bool isItemSelected(unsigned int index) const;
	bool isItemEnabled(unsigned int index) const;

private:
	typedef std::vector<BasemenuItem *> Menuitems;
	Fluxbox *m_fluxbox;
	BScreen *m_screen;
	Display *m_display;
	Basemenu *m_parent;
	BImageControl *m_image_ctrl;
	Menuitems menuitems;
	
	bool moving, visible, movable, torn, internal_menu, title_vis, shifted,
		hide_tree;
	
	int which_sub, which_press, which_sbl;
	Alignment m_alignment;

	struct _menu {
		Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
		Window window, frame, title;

		std::string label;
		int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
			grab_x, grab_y;
		unsigned int width, height, title_h, frame_h, item_w, item_h, bevel_w,
			bevel_h;
	} menu;


protected:
	inline BasemenuItem *find(unsigned int index) const { return menuitems[index]; }
	inline void setTitleVisibility(bool b) { title_vis = b; }
	inline void setMovable(bool b) { movable = b; }
	inline void setHideTree(bool h) { hide_tree = h; }
	inline void setMinimumSublevels(int m) { menu.minsub = m; }

	virtual void itemSelected(int button, unsigned int index) = 0;
	virtual void drawItem(unsigned int index, bool highlight= false, bool clear= false,
			int x= -1, int y= -1, unsigned int width= 0, unsigned int height= 0);
	virtual void redrawTitle();
	virtual void internal_hide();
};

class BasemenuItem {
public:
	BasemenuItem(
			const char *label,
			int function,
			const char *exec = (const char *) 0)
		: m_label(label ? label : "")
		, m_exec(exec ? exec : "")
		, m_submenu(0)
		, m_function(function)
		, m_enabled(true)
		, m_selected(false)
	{ }

	BasemenuItem(const char *label, Basemenu *submenu)
		: m_label(label ? label : "")
		, m_exec("")
		, m_submenu(submenu)
		, m_function(0)
		, m_enabled(true)
		, m_selected(false)
	{ }

	inline const std::string &exec() const { return m_exec; }
	inline const std::string &label() const { return m_label; }
	inline int function() const { return m_function; }
	inline Basemenu *submenu() const { return m_submenu; }

	inline bool isEnabled() const { return m_enabled; }
	inline void setEnabled(bool enabled) { m_enabled = enabled; }
	inline bool isSelected() const { return m_selected; }
	inline void setSelected(bool selected) { m_selected = selected; }

private:
	std::string m_label, m_exec;
	Basemenu *m_submenu;
	int m_function;
	bool m_enabled, m_selected;

	friend class Basemenu;
};


#endif // _BASEMENU_HH_
