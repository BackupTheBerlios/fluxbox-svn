// MultLayers.cc for FbTk - fluxbox toolkit
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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

// $Id: MultLayers.cc,v 1.4 2003/02/03 13:46:13 fluxgen Exp $

#include "MultLayers.hh"
#include "XLayer.hh"
#include "XLayerItem.hh"
#include "App.hh"

#include <iostream>
using namespace std;

using namespace FbTk;

MultLayers::MultLayers(int numlayers) {
    for (int i=0; i < numlayers; ++i) 
        m_layers.push_back(new XLayer(*this, i));
}

MultLayers::~MultLayers() {
    while (!m_layers.empty()) {
        delete m_layers.back();
        m_layers.pop_back();
    }
}


XLayerItem *MultLayers::getLowestItemAboveLayer(int layernum) {
    if (layernum >= m_layers.size() || layernum <= 0) 
        return 0;

    layernum--; // next one up
    XLayerItem *item = 0;
    while (layernum >= 0 && (item = m_layers[layernum]->getLowestItem()) == 0)
        layernum--;
    return item;

}    

XLayerItem *MultLayers::getItemBelow(XLayerItem &item) {
    XLayer &curr_layer = item.getLayer();

    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer.getItemBelow(item);

    if (ret == 0) {
        int num = curr_layer.getLayerNum()-1;
        while (num >= 0 && !ret) {
            ret = m_layers[num]->getItemBelow(item);
            num--;
        }
    }

    return ret;
}    

XLayerItem *MultLayers::getItemAbove(XLayerItem &item) {
    XLayer &curr_layer = item.getLayer();
    
    // assume that the LayerItem does exist in a layer.
    XLayerItem *ret = curr_layer.getItemAbove(item);

    if (!ret) {
        ret = getLowestItemAboveLayer(curr_layer.getLayerNum());
    }    

    return ret;
}    

void MultLayers::addToTop(XLayerItem &item, int layernum) {
    if (layernum < 0) 
        layernum = 0; 
    else if (layernum >= m_layers.size())
        layernum = m_layers.size()-1;

    m_layers[layernum]->insert(item);
    restack();
}

/* raise the item one level */
void MultLayers::raise(XLayerItem &item) {
    // get the layer it is in
    XLayer &curr_layer = item.getLayer();
    moveToLayer(item, curr_layer.getLayerNum()-1);
}

void MultLayers::moveToLayer(XLayerItem &item, int layernum) {
    // get the layer it is in
    XLayer &curr_layer = item.getLayer();

    // do nothing if the item already is in the requested layer
    if (curr_layer.getLayerNum() == layernum)
        return;

    // clamp layer number
    if (layernum < 0) 
        layernum = 0; 
    else if (layernum >= m_layers.size()) 
        layernum = m_layers.size()-1;
    // remove item from old layer and insert it into the 
    item.setLayer(*m_layers[layernum]);
    curr_layer.remove(item);
    m_layers[layernum]->insert(item);
}

void MultLayers::restack() {
    size_t winlist_size=0;
    for (size_t layer=0; layer < m_layers.size(); layer++) {
        winlist_size += m_layers[layer]->countWindows();
    }

    Window *winlist = new Window[winlist_size];
    for (size_t layer=0, window=0; layer < m_layers.size(); layer++) {

        XLayer::ItemList::iterator item_it = m_layers[layer]->getItemList().begin();
        XLayer::ItemList::iterator item_it_end = m_layers[layer]->getItemList().end();
        for (; item_it != item_it_end; ++item_it, window++)
            winlist[window] = (*item_it)->window();
    }

    XRestackWindows(FbTk::App::instance()->display(), winlist, winlist_size);

    delete[] winlist;
}

int MultLayers::size() {
    int i = 0, num = 0;
    for (; i < m_layers.size(); i++) {
        num += m_layers[i]->countWindows();
    }
    return num;
}

XLayer *MultLayers::getLayer(size_t num) {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

const XLayer *MultLayers::getLayer(size_t num) const {
    if (num >= m_layers.size())
        return 0;
    return m_layers[num];
}

