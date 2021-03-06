// Texture.hh for Fluxbox Window Manager 
// Copyright (c) 2002 Henrik Kinnunen (fluxbox@linuxmail.org)
//
// from Image.hh for Blackbox - an X11 Window manager
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// $Id: Texture.hh,v 1.1 2002/07/23 16:23:15 fluxgen Exp $

#ifndef FBTK_TEXTURE_HH
#define FBTK_TEXTURE_HH

#include "Color.hh"

namespace FbTk  {

/**
	Holds texture type and info
*/
class Texture {
public:
	enum Bevel {
		FLAT =     0x00002,
		SUNKEN =   0x00004, 
		RAISED =   0x00008
	};
	enum Textures {
		NONE =     0x00000,
		SOLID =    0x00010,
		GRADIENT = 0x00020
	};
	enum Gradients {
		HORIZONTAL =     0x00040,
		VERTICAL =       0x00080,
		DIAGONAL =       0x00100,
		CROSSDIAGONAL =  0x00200,
		RECTANGLE =      0x00400,
		PYRAMID =        0x00800,
		PIPECROSS =      0x01000,
		ELLIPTIC =       0x02000		
	};
	
	enum {
		BEVEL1 =         0x04000, 
		BEVEL2 =         0x08000, // bevel types
		INVERT =        0x010000, //inverted image
		PARENTRELATIVE = 0x20000,
		INTERLACED =     0x40000
	};

	Texture():m_type(0) { }

	inline void setType(unsigned long t) { m_type = t; }
	inline void addType(unsigned long t) { m_type |= t; }
	
	inline Color &color() { return m_color; }
	inline Color &colorTo() { return m_color_to; }
	inline Color &hiColor() { return m_hicolor; }
	inline Color &loColor() { return m_locolor; }

	inline const Color &color() const { return m_color; }
	inline const Color &colorTo() const { return m_color_to; }
	inline const Color &hiColor() const { return m_hicolor; }
	inline const Color &loColor() const { return m_locolor; }
	
	inline unsigned long type() const { return m_type; }


private:
	FbTk::Color m_color, m_color_to, m_hicolor, m_locolor;
	unsigned long m_type;
};

}; // end namespace FbTk

#endif // FBTK_TEXTURE_HH
