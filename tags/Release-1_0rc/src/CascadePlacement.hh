// CascadePlacement.hh
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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

#ifndef CASCADEPLACEMENT_HH
#define CASCADEPLACEMENT_HH

#include "PlacementStrategy.hh"
#include "FbTk/NotCopyable.hh"

class BScreen;

class CascadePlacement: public PlacementStrategy, 
                        private FbTk::NotCopyable {
public:
    explicit CascadePlacement(const BScreen &screen);
    ~CascadePlacement();
    bool placeWindow(const std::vector<FluxboxWindow *> &windowlist,
                     const FluxboxWindow &window,
                     int &place_x, int &place_y);
private:
    int *m_cascade_x; ///< need a cascade for each head (Xinerama)
    int *m_cascade_y; ///< need a cascade for each head (Xinerama)
};

#endif // CASCADEPLACEMENT_HH
