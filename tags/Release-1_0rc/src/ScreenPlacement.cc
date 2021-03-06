// ScreenPlacement.cc
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

#include "ScreenPlacement.hh"


#include "RowSmartPlacement.hh"
#include "UnderMousePlacement.hh"
#include "ColSmartPlacement.hh"
#include "CascadePlacement.hh"

#include "Screen.hh"
#include "Window.hh"

#include <iostream>
#include <exception>
using std::cerr;
using std::endl;

ScreenPlacement::ScreenPlacement(BScreen &screen):
    m_row_direction(screen.resourceManager(), LEFTRIGHT, 
                    screen.name()+".rowPlacementDirection", 
                    screen.altName()+".RowPlacementDirection"),
    m_col_direction(screen.resourceManager(), TOPBOTTOM, 
                    screen.name()+".colPlacementDirection", 
                    screen.altName()+".ColPlacementDirection"),
    m_placement_policy(screen.resourceManager(), ROWSMARTPLACEMENT, 
                       screen.name()+".windowPlacement", 
                       screen.altName()+".WindowPlacement"),
    m_old_policy(ROWSMARTPLACEMENT),
    m_strategy(0)
{
}

bool ScreenPlacement::placeWindow(const std::vector<FluxboxWindow *> &windowlist,
                                  const FluxboxWindow &win,
                                  int &place_x, int &place_y) {


    // check the resource placement and see if has changed
    // and if so update the strategy
    if (m_old_policy != *m_placement_policy || !m_strategy.get()) {
        m_old_policy = *m_placement_policy;
        switch (*m_placement_policy) {
        case ROWSMARTPLACEMENT:
            m_strategy.reset(new RowSmartPlacement());
            break;
        case COLSMARTPLACEMENT:
            m_strategy.reset(new ColSmartPlacement());
            break;
        case CASCADEPLACEMENT:
            m_strategy.reset(new CascadePlacement(win.screen()));
            break;
        case UNDERMOUSEPLACEMENT:
            m_strategy.reset(new UnderMousePlacement());
            break;
        }
    }

    // view (screen + head) constraints
    int head = (signed) win.screen().getCurrHead();
    int head_left = (signed) win.screen().maxLeft(head);
    int head_right = (signed) win.screen().maxRight(head);
    int head_top = (signed) win.screen().maxTop(head);
    int head_bot = (signed) win.screen().maxBottom(head);

    // start placement, top left corner
    place_x = head_left;
    place_y = head_top;

    bool placed = false;
    try {
        placed = m_strategy->placeWindow(windowlist,
                                         win,
                                         place_x, place_y);
    } catch (std::bad_cast cast) {
        // This should not happen. 
        // If for some reason we change the PlacementStrategy in Screen
        // from ScreenPlacement to something else then we might get 
        // bad_cast from some placement strategies.
        cerr<<"Failed to place window: "<<cast.what()<<endl;
    }

    if (!placed) {
        // Create fallback strategy, when we need it the first time
        // This strategy must succeed!
        if (m_fallback_strategy.get() == 0)
            m_fallback_strategy.reset(new CascadePlacement(win.screen()));

        m_fallback_strategy->placeWindow(windowlist,
                                         win,
                                         place_x, place_y);
    }



    int win_w = win.width() + win.fbWindow().borderWidth()*2 + win.widthOffset(),
        win_h = win.height() + win.fbWindow().borderWidth()*2 + win.heightOffset();

    // make sure the window is inside our screen(head) area
    if (place_x + win_w > head_right)
        place_x = (head_right - win_w) / 2 + win.xOffset();
    if (place_y + win_h > head_bot)
        place_y = (head_bot - win_h) / 2 + win.yOffset();

    return true;
}



////////////////////// Placement Resources
namespace FbTk {
    
template <>
void FbTk::Resource<ScreenPlacement::PlacementPolicy>::setFromString(const char *str) {
    if (strcasecmp("RowSmartPlacement", str) == 0)
        *(*this) = ScreenPlacement::ROWSMARTPLACEMENT;
    else if (strcasecmp("ColSmartPlacement", str) == 0)
        *(*this) = ScreenPlacement::COLSMARTPLACEMENT;
    else if (strcasecmp("UnderMousePlacement", str) == 0)
        *(*this) = ScreenPlacement::UNDERMOUSEPLACEMENT;
    else if (strcasecmp("CascadePlacement", str) == 0)
        *(*this) = ScreenPlacement::CASCADEPLACEMENT;
    else
        setDefaultValue();
}

template <>
std::string FbTk::Resource<ScreenPlacement::PlacementPolicy>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::ROWSMARTPLACEMENT:
        return "RowSmartPlacement";
    case ScreenPlacement::COLSMARTPLACEMENT:
        return "ColSmartPlacement";
    case ScreenPlacement::UNDERMOUSEPLACEMENT:
        return "UnderMousePlacement";
    case ScreenPlacement::CASCADEPLACEMENT:
        return "CascadePlacement";
    }

    return "RowSmartPlacement";
}

template <>
void FbTk::Resource<ScreenPlacement::RowDirection>::setFromString(const char *str) {
    if (strcasecmp("LeftToRight", str) == 0)
        *(*this) = ScreenPlacement::LEFTRIGHT;
    else if (strcasecmp("RightToLeft", str) == 0)
        *(*this) = ScreenPlacement::RIGHTLEFT;
    else
        setDefaultValue();
    
}

template <>
std::string FbTk::Resource<ScreenPlacement::RowDirection>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::LEFTRIGHT:
        return "LeftToRight";
    case ScreenPlacement::RIGHTLEFT:
        return "RightToLeft";
    }

    return "LeftToRight";
}


template <>
void FbTk::Resource<ScreenPlacement::ColumnDirection>::setFromString(const char *str) {
    if (strcasecmp("TopToBottom", str) == 0)
        *(*this) = ScreenPlacement::TOPBOTTOM;
    else if (strcasecmp("BottomToTop", str) == 0)
        *(*this) = ScreenPlacement::BOTTOMTOP;
    else
        setDefaultValue();
    
}

template <>
std::string FbTk::Resource<ScreenPlacement::ColumnDirection>::getString() const {
    switch (*(*this)) {
    case ScreenPlacement::TOPBOTTOM:
        return "TopToBottom";
    case ScreenPlacement::BOTTOMTOP:
        return "BottomToTop";
    }

    return "TopToBottom";
}
} // end namespace FbTk
