// Remember.cc for Fluxbox Window Manager
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                     and Simon Bowden    (rathnor at users.sourceforge.net)
// Copyright (c) 2002 Xavier Brouckaert
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

// $Id$

#include "Remember.hh"
#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "FbMenu.hh"
#include "FbCommands.hh"
#include "fluxbox.hh"
#include "WindowCmd.hh"
#include "Layer.hh"

#include "FbTk/I18n.hh"
#include "FbTk/StringUtil.hh"
#include "FbTk/MenuItem.hh"
#include "FbTk/App.hh"
#include "FbTk/stringstream.hh"


#include <X11/Xlib.h>

//use GNU extensions
#ifndef	 _GNU_SOURCE
#define	 _GNU_SOURCE
#endif // _GNU_SOURCE


#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <set>


using namespace std;

namespace {

class RememberMenuItem : public FbTk::MenuItem {
public:
    RememberMenuItem(const char *label,
                     Remember::Attribute attrib) :
        FbTk::MenuItem(label),
        m_attrib(attrib) {
        setToggleItem(true);
    }

    bool isSelected() const {
        if (WindowCmd<void>::window() == 0)
            return false;

        if (WindowCmd<void>::window()->numClients()) // ensure it HAS clients
            return Remember::instance().isRemembered(WindowCmd<void>::window()->winClient(), m_attrib);
        else
            return false;
    }

    bool isEnabled() const {
        if (WindowCmd<void>::window() == 0)
            return false;

        if (m_attrib != Remember::REM_JUMPWORKSPACE)
            return true;
        else if (WindowCmd<void>::window()->numClients())
            return (Remember::instance().isRemembered(WindowCmd<void>::window()->winClient(), Remember::REM_WORKSPACE));
        else
            return false;
    }

    void click(int button, int time) {
        if (WindowCmd<void>::window() != 0) {
            if (isSelected()) {
                Remember::instance().forgetAttrib(WindowCmd<void>::window()->winClient(), m_attrib);
            } else {
                Remember::instance().rememberAttrib(WindowCmd<void>::window()->winClient(), m_attrib);
            }
        }
        Remember::instance().save();
        FbTk::MenuItem::click(button, time);
    }

private:
    Remember::Attribute m_attrib;
};

FbTk::Menu *createRememberMenu(BScreen &screen) {
    // each fluxboxwindow has its own windowmenu
    // so we also create a remember menu just for it...
    FbTk::Menu *menu = screen.createMenu("");

    // if enabled, then we want this to be a unavailable menu
    /*
    if (!enabled) {
        FbTk::MenuItem *item = new FbTk::MenuItem("unavailable");
        item->setEnabled(false);
        menu->insert(item);
        menu->updateMenu();
        return menu;
    }
    */
    _FB_USES_NLS;
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Workspace, "Workspace", "Remember Workspace"),
                                      Remember::REM_WORKSPACE));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, JumpToWorkspace, "Jump to workspace", "Change active workspace to remembered one on open"),
                                      Remember::REM_JUMPWORKSPACE));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Head, "Head", "Remember Head"),
                                      Remember::REM_HEAD));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Dimensions, "Dimensions", "Remember Dimensions - with width and height"),
                                      Remember::REM_DIMENSIONS));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Position, "Position", "Remember position - window co-ordinates"),
                                      Remember::REM_POSITION));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Sticky, "Sticky", "Remember Sticky"),
                                      Remember::REM_STUCKSTATE));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Decorations, "Decorations", "Remember window decorations"),
                                      Remember::REM_DECOSTATE));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Shaded, "Shaded", "Remember shaded"),
                                      Remember::REM_SHADEDSTATE));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, Layer, "Layer", "Remember Layer"),
                                      Remember::REM_LAYER));
    menu->insert(new RememberMenuItem(_FBTEXT(Remember, SaveOnClose, "Save on close", "Save remembered attributes on close"),
                                      Remember::REM_SAVEONCLOSE));

    menu->updateMenu();
    return menu;
};

// offset is the offset in the string that we start looking from
// return true if all ok, false on error
bool handleStartupItem(const string &line, int offset) {
    int next = 0;
    string str;
    int screen = 0;

    // accept some options, for now only "screen=NN"
    // these option are given in parentheses before the command
    next = FbTk::StringUtil::getStringBetween(str,
                                              line.c_str() + offset,
                                              '(', ')');
    if (next > 0) {
        // there are some options
        string option;
        int pos = str.find('=');
        bool error = false;
        if (pos > 0) {
            option = str.substr(0, pos);
            if (option == "screen") {
                FbTk_istringstream iss(str.c_str() + pos + 1);
                iss >> screen;
            } else {
                error = true;
            }
        } else {
            error = true;
        }
        if (error) {
            cerr<<"Error parsing startup options."<<endl;
            return false;
        }
    } else {
        next = 0;
    }

    next = FbTk::StringUtil::getStringBetween(str,
                                              line.c_str() + offset + next,
                                              '{', '}');

    if (next <= 0) {
        cerr<<"Error parsing [startup] at column "<<offset<<" - expecting {command}."<<endl;
        return false;
    } else {
        FbCommands::ExecuteCmd *tmp_exec_cmd = new FbCommands::ExecuteCmd(str, screen);
#ifdef DEBUG
        cerr<<"Executing startup command '"<<str<<"' on screen "<<screen<<endl;
#endif // DEBUG
        tmp_exec_cmd->execute();
        delete tmp_exec_cmd;
        return true;
    }
};

}; // end anonymous namespace


Application::Application(bool grouped)
    : is_grouped(grouped),
      group(0)
{
    decostate_remember =
        dimensions_remember =
        focushiddenstate_remember =
        iconhiddenstate_remember =
        jumpworkspace_remember =
        layer_remember  =
        position_remember =
        shadedstate_remember =
        stuckstate_remember =
        tabstate_remember =
        workspace_remember =
        head_remember =
        save_on_close_remember = false;
}

/************
 * Remember *
 ************/

Remember *Remember::s_instance = 0;

Remember::Remember() {
    if (s_instance != 0)
        throw string("Can not create more than one instance of Remember");

    s_instance = this;
    enableUpdate();
    load();
}

Remember::~Remember() {

    // free our resources

    // the patterns free the "Application"s
    // the client mapping shouldn't need cleaning
    Patterns::iterator it;
    std::set<Application *> all_apps; // no duplicates
    while (!m_pats.empty()) {
        it = m_pats.begin();
        delete it->first; // ClientPattern
        all_apps.insert(it->second); // Application, not necessarily unique
        m_pats.erase(it);
    }

    std::set<Application *>::iterator ait = all_apps.begin(); // no duplicates
    while (ait != all_apps.end()) {
        delete (*ait);
        ++ait;
    }

    s_instance = 0;
}

Application* Remember::find(WinClient &winclient) {
    // if it is already associated with a application, return that one
    // otherwise, check it against every pattern that we've got
    Clients::iterator wc_it = m_clients.find(&winclient);
    if (wc_it != m_clients.end())
        return wc_it->second;
    else {
        Patterns::iterator it = m_pats.begin();
        for (; it != m_pats.end(); it++)
            if (it->first->match(winclient)) {
                it->first->addMatch();
                m_clients[&winclient] = it->second;
                return it->second;
            }
    }
    // oh well, no matches
    return 0;
}

Application * Remember::add(WinClient &winclient) {
    ClientPattern *p = new ClientPattern();
    Application *app = new Application(false);
    // by default, we match against the WMClass of a window.
    p->addTerm(p->getProperty(ClientPattern::NAME, winclient), ClientPattern::NAME);
    m_clients[&winclient] = app;
    p->addMatch();
    m_pats.push_back(make_pair(p, app));
    return app;
}

int Remember::parseApp(ifstream &file, Application &app, string *first_line) {
    string line;
    _FB_USES_NLS;
    int row = 0;
    while (! file.eof()) {
        if (first_line || getline(file, line)) {
            if (first_line) {
                line = *first_line;
                first_line = 0;
            }

            row++;
            FbTk::StringUtil::removeFirstWhitespace(line);
            FbTk::StringUtil::removeTrailingWhitespace(line);
            if (line.size() == 0 || line[0] == '#')
                continue;  //the line is commented or blank
            int parse_pos = 0, err = 0;
            string str_key, str_option, str_label;
            err = FbTk::StringUtil::getStringBetween(str_key,
                                                     line.c_str(),
                                                     '[', ']');
            if (err > 0) {
                int tmp;
                tmp= FbTk::StringUtil::getStringBetween(str_option,
                                                        line.c_str() + err,
                                                        '(', ')');
                if (tmp>0)
                    err += tmp;
            }
            if (err > 0 ) {
                parse_pos += err;
                err = FbTk::StringUtil::getStringBetween(str_label,
                                                         line.c_str() + parse_pos,
                                                         '{', '}');
                if (err>0) {
                    parse_pos += err;
                }
            } else
                continue; //read next line

            if (!str_key.size())
                continue; //read next line
            if (str_key == "Workspace") {
                unsigned int w;
                FbTk_istringstream iss(str_label.c_str());
                iss >> w;
                app.rememberWorkspace(w);
            } else if (str_key == "Head") {
                int h = atoi(str_label.c_str());
                app.rememberHead(h);
            } else if (str_key == "Layer") {
                unsigned int l;
                if (str_label == "DESKTOP") {
                    l = Layer::DESKTOP;
                } else if (str_label == "BOTTOM") {
                    l = Layer::BOTTOM;
                } else if (str_label == "NORMAL") {
                    l = Layer::NORMAL;
                } else if (str_label == "TOP") {
                    l = Layer::TOP;
                } else if (str_label == "DOCK") {
                    l = Layer::DOCK;
                } else if (str_label == "ABOVEDOCK") {
                    l = Layer::ABOVE_DOCK;
                } else if (str_label == "MENU") {
                    l = Layer::MENU;
                } else {
                    FbTk_istringstream iss(str_label.c_str());
                    iss >> l;
                }
                app.rememberLayer(l);
            } else if (str_key == "Dimensions") {
                unsigned int h,w;
                FbTk_istringstream iss(str_label.c_str());
                iss >> w >> h;

                app.rememberDimensions(w, h);

            } else if (str_key == "Position") {
                unsigned int r= 0;
                unsigned int x= 0;
                unsigned int y= 0;
                // more info about the parameter
                // in ::rememberPosition

                if ( str_option.length() )
                    {
                        if      ( str_option == "UPPERLEFT"  ) r= POS_UPPERLEFT;
                        else if ( str_option == "UPPERRIGHT" ) r= POS_UPPERRIGHT;
                        else if ( str_option == "LOWERLEFT"  ) r= POS_LOWERLEFT;
                        else if ( str_option == "LOWERRIGHT" ) r= POS_LOWERRIGHT;
                        else if ( str_option == "CENTER" )     r= POS_CENTER;
                        else if ( str_option == "WINCENTER" )  r= POS_WINCENTER;
                        else {
                            FbTk_istringstream iss_r(str_option.c_str());
                            iss_r >> r;
                        }
                    }

                FbTk_istringstream iss_xy(str_label.c_str());
                iss_xy >> x >> y;
                app.rememberPosition(x, y, r);
            } else if (str_key == "Shaded") {
                app.rememberShadedstate((str_label=="yes"));
            } else if (str_key == "Tab") {
                app.rememberTabstate((str_label=="yes"));
            } else if (str_key == "FocusHidden") {
                app.rememberFocusHiddenstate((str_label=="yes"));
            } else if (str_key == "IconHidden") {
                app.rememberIconHiddenstate((str_label=="yes"));
            } else if (str_key == "Hidden") {
                app.rememberIconHiddenstate((str_label=="yes"));
                app.rememberFocusHiddenstate((str_label=="yes"));
            } else if (str_key == "Deco") {
                if (str_label == "NONE") {
                    app.rememberDecostate((unsigned int) 0);
                } else if (str_label == "NORMAL") {
                    app.rememberDecostate((unsigned int) 0xfffffff);
                } else if (str_label == "TINY") {
                    app.rememberDecostate((unsigned int)
                                          FluxboxWindow::DECORM_TITLEBAR
                                          | FluxboxWindow::DECORM_ICONIFY
                                          | FluxboxWindow::DECORM_MENU
                                          );
                } else if (str_label == "TOOL") {
                    app.rememberDecostate((unsigned int)
                                          FluxboxWindow::DECORM_TITLEBAR
                                          | FluxboxWindow::DECORM_MENU
                                          );
                } else if (str_label == "BORDER") {
                    app.rememberDecostate((unsigned int)
                                          FluxboxWindow::DECORM_BORDER
                                          | FluxboxWindow::DECORM_MENU
                                          );
                } else {
                    unsigned int mask;
                    const char * str = str_label.c_str();
                    // it'll have at least one char and \0, so this is safe
                    FbTk_istringstream iss(str);
                    // check for hex
                    if (str[0] == '0' && str[1] == 'x') {
                        iss.seekg(2);
                        iss >> hex;
                    }
                    iss >> mask ;
                    app.rememberDecostate(mask);
                }
            } else if (str_key == "Sticky") {
                app.rememberStuckstate((str_label=="yes"));
            } else if (str_key == "Jump") {
                app.rememberJumpworkspace((str_label=="yes"));
            } else if (str_key == "Close") {
                app.rememberSaveOnClose((str_label=="yes"));
            } else if (str_key == "end") {
                return row;
            } else {
                cerr << _FBTEXT(Remember, Unknown, "Unknown apps key", "apps entry type not known")<<" = " << str_key << endl;
            }
        }
    }
    return row;
}

void Remember::load() {

    string apps_string = FbTk::StringUtil::expandFilename(Fluxbox::instance()->getAppsFilename());

#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): Loading apps file ["<<apps_string<<"]"<<endl;
#endif // DEBUG
    ifstream apps_file(apps_string.c_str());

    if (!apps_file.fail()) {
        if (!apps_file.eof()) {
            string line;
            int row = 0;
            bool in_group = false;
            std::list<ClientPattern *> grouped_pats;
            while (getline(apps_file, line) && ! apps_file.eof()) {
                row++;
                FbTk::StringUtil::removeFirstWhitespace(line);
                FbTk::StringUtil::removeTrailingWhitespace(line);
                if (line.size() == 0 || line[0] == '#')
                    continue;
                string key;
                int err=0;
                int pos = FbTk::StringUtil::getStringBetween(key,
                                                             line.c_str(),
                                                             '[', ']');

                if (pos > 0 && key == "app") {
                    ClientPattern *pat = new ClientPattern(line.c_str() + pos);
                    if (!in_group) {
                        if ((err = pat->error()) == 0) {
                            Application *app = new Application(false);
                            m_pats.push_back(make_pair(pat, app));
                            row += parseApp(apps_file, *app);
                        } else {
                            cerr<<"Error reading apps file at line "<<row<<", column "<<(err+pos)<<"."<<endl;
                            delete pat; // since it didn't work
                        }
                    } else {
                        grouped_pats.push_back(pat);
                    }
                } else if (pos > 0 && key == "startup") {
                    if (!handleStartupItem(line, pos)) {
                        cerr<<"Error reading apps file at line "<<row<<"."<<endl;
                    }
                    // save the item even if it was bad (aren't we nice)
                    m_startups.push_back(line.substr(pos));
                } else if (pos > 0 && key == "group") {
                    in_group = true;
                } else if (in_group) {
                    // otherwise assume that it is the start of the attributes
                    Application *app = new Application(true);
                    while (!grouped_pats.empty()) {
                        // associate all the patterns with this app
                        m_pats.push_back(make_pair(grouped_pats.front(), app));
                        grouped_pats.pop_front();
                    }

                    // we hit end... probably don't have attribs for the group
                    // so finish it off with an empty application
                    // otherwise parse the app
                    if (!(pos>0 && key == "end")) {
                        row += parseApp(apps_file, *app, &line);
                    }
                    in_group = false;
                } else
                    cerr<<"Error in apps file on line "<<row<<"."<<endl;

            }
        } else {
#ifdef DEBUG
            cerr<<__FILE__<<"("<<__FUNCTION__<< ") Empty apps file" << endl;
#endif
        }
    } else {
        cerr << "apps file failure" << endl;
    }
}

void Remember::save() {
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__FUNCTION__<<"): Saving apps file..."<<endl;
#endif // DEBUG
    string apps_string;
    Fluxbox::instance()->getDefaultDataFilename("apps", apps_string);
    ofstream apps_file(apps_string.c_str());

    // first of all we output all the startup commands
    Startups::iterator sit = m_startups.begin();
    Startups::iterator sit_end = m_startups.end();
    for (; sit != sit_end; ++sit) {
        apps_file<<"[startup] "<<(*sit)<<endl;
    }

    Patterns::iterator it = m_pats.begin();
    Patterns::iterator it_end = m_pats.end();

    std::set<Application *> grouped_apps; // no duplicates

    for (; it != it_end; ++it) {
        Application &a = *it->second;
        if (a.is_grouped) {
            // if already processed
            if (grouped_apps.find(&a) != grouped_apps.end())
                continue;
            grouped_apps.insert(&a);
            // otherwise output this whole group
            apps_file << "[group]" << endl;
            Patterns::iterator git = m_pats.begin();
            Patterns::iterator git_end = m_pats.end();
            for (; git != git_end; git++) {
                if (git->second == &a) {
                    apps_file << " [app]"<<git->first->toString()<<endl;
                }
            }
        } else {
            apps_file << "[app]"<<it->first->toString()<<endl;
        }
        if (a.workspace_remember) {
            apps_file << "  [Workspace]\t{" << a.workspace << "}" << endl;
        }
        if (a.head_remember) {
            apps_file << "  [Head]\t{" << a.head << "}" << endl;
        }
        if (a.dimensions_remember) {
            apps_file << "  [Dimensions]\t{" << a.w << " " << a.h << "}" << endl;
        }
        if (a.position_remember) {
            apps_file << "  [Position]\t(";
            switch(a.refc) {
            case POS_WINCENTER:
                apps_file << "WINCENTER";
                break;
            case POS_CENTER:
                apps_file << "CENTER";
                break;
            case POS_LOWERLEFT:
                apps_file << "LOWERLEFT";
                break;
            case POS_LOWERRIGHT:
                apps_file << "LOWERRIGHT";
                break;
            case POS_UPPERRIGHT:
                apps_file << "UPPERRIGHT";
                break;
            default:
                apps_file << "UPPERLEFT";
            }
            apps_file << ")\t{" << a.x << " " << a.y << "}" << endl;
        }
        if (a.shadedstate_remember) {
            apps_file << "  [Shaded]\t{" << ((a.shadedstate)?"yes":"no") << "}" << endl;
        }
        if (a.tabstate_remember) {
            apps_file << "  [Tab]\t\t{" << ((a.tabstate)?"yes":"no") << "}" << endl;
        }
        if (a.decostate_remember) {
            switch (a.decostate) {
            case (0) :
                apps_file << "  [Deco]\t{NONE}" << endl;
                break;
            case (0xffffffff):
            case (FluxboxWindow::DECORM_LAST - 1):
                apps_file << "  [Deco]\t{NORMAL}" << endl;
                break;
            case (FluxboxWindow::DECORM_TITLEBAR
                  | FluxboxWindow::DECORM_ICONIFY
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{TOOL}" << endl;
                break;
            case (FluxboxWindow::DECORM_TITLEBAR
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{TINY}" << endl;
                break;
            case (FluxboxWindow::DECORM_BORDER
                  | FluxboxWindow::DECORM_MENU):
                apps_file << "  [Deco]\t{BORDER}" << endl;
                break;
            default:
                apps_file << "  [Deco]\t{0x"<<hex<<a.decostate<<dec<<"}"<<endl;
                break;
            }
        }

        if (a.focushiddenstate_remember || a.iconhiddenstate_remember) {
            if (a.focushiddenstate_remember && a.iconhiddenstate_remember &&
                a.focushiddenstate && a.iconhiddenstate)
                apps_file << "  [Hidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}" << endl;
            else if (a.focushiddenstate_remember) {
                apps_file << "  [FocusHidden]\t{" << ((a.focushiddenstate)?"yes":"no") << "}" << endl;
            } else if (a.iconhiddenstate_remember) {
                apps_file << "  [IconHidden]\t{" << ((a.iconhiddenstate)?"yes":"no") << "}" << endl;
            }
        }
        if (a.stuckstate_remember) {
            apps_file << "  [Sticky]\t{" << ((a.stuckstate)?"yes":"no") << "}" << endl;
        }
        if (a.jumpworkspace_remember) {
            apps_file << "  [Jump]\t{" << ((a.jumpworkspace)?"yes":"no") << "}" << endl;
        }
        if (a.layer_remember) {
            apps_file << "  [Layer]\t{" << a.layer << "}" << endl;
        }
        if (a.save_on_close_remember) {
            apps_file << "  [Close]\t{" << ((a.save_on_close)?"yes":"no") << "}" << endl;
        }
        apps_file << "[end]" << endl;
    }
}

bool Remember::isRemembered(WinClient &winclient, Attribute attrib) {
    Application *app = find(winclient);
    if (!app) return false;
    switch (attrib) {
    case REM_WORKSPACE:
        return app->workspace_remember;
        break;
    case REM_HEAD:
        return app->head_remember;
        break;
    case REM_DIMENSIONS:
        return app->dimensions_remember;
        break;
    case REM_POSITION:
        return app->position_remember;
        break;
    case REM_FOCUSHIDDENSTATE:
        return app->focushiddenstate_remember;
        break;
    case REM_ICONHIDDENSTATE:
        return app->iconhiddenstate_remember;
        break;
    case REM_STUCKSTATE:
        return app->stuckstate_remember;
        break;
    case REM_DECOSTATE:
        return app->decostate_remember;
        break;
    case REM_SHADEDSTATE:
        return app->shadedstate_remember;
        break;
        //    case REM_TABSTATE:
        //        return app->tabstate_remember;
        //        break;
    case REM_JUMPWORKSPACE:
        return app->jumpworkspace_remember;
        break;
    case REM_LAYER:
        return app->layer_remember;
        break;
    case REM_SAVEONCLOSE:
        return app->save_on_close_remember;
        break;
    case REM_LASTATTRIB:
    default:
        return false; // should never get here
    }
}

void Remember::rememberAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    switch (attrib) {
    case REM_WORKSPACE:
        app->rememberWorkspace(win->workspaceNumber());
        break;
    case REM_HEAD:
        app->rememberHead(win->screen().getHead(win->fbWindow()));
        break;
    case REM_DIMENSIONS:
        //!! Note: This is odd, why dont we need to substract border width on win->width() ?
        app->rememberDimensions(win->width(), win->height() - 2 * win->fbWindow().borderWidth());
        break;
    case REM_POSITION:
        app->rememberPosition(win->x(), win->y());
        break;
    case REM_FOCUSHIDDENSTATE:
        app->rememberFocusHiddenstate(win->isFocusHidden());
        break;
    case REM_ICONHIDDENSTATE:
        app->rememberIconHiddenstate(win->isIconHidden());
        break;
    case REM_SHADEDSTATE:
        app->rememberShadedstate(win->isShaded());
        break;
    case REM_DECOSTATE:
        app->rememberDecostate(win->decorationMask());
        break;
    case REM_STUCKSTATE:
        app->rememberStuckstate(win->isStuck());
        break;
        //    case REM_TABSTATE:
        //        break;
    case REM_JUMPWORKSPACE:
        app->rememberJumpworkspace(true);
        break;
    case REM_LAYER:
        app->rememberLayer(win->layerNum());
        break;
    case REM_SAVEONCLOSE:
        app->rememberSaveOnClose(true);
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::forgetAttrib(WinClient &winclient, Attribute attrib) {
    FluxboxWindow *win = winclient.fbwindow();
    if (!win) return;
    Application *app = find(winclient);
    if (!app) {
        app = add(winclient);
        if (!app) return;
    }
    switch (attrib) {
    case REM_WORKSPACE:
        app->forgetWorkspace();
        break;
    case REM_HEAD:
        app->forgetHead();
        break;
    case REM_DIMENSIONS:
        app->forgetDimensions();
        break;
    case REM_POSITION:
        app->forgetPosition();
        break;
    case REM_FOCUSHIDDENSTATE:
        app->forgetFocusHiddenstate();
        break;
    case REM_ICONHIDDENSTATE:
        app->forgetIconHiddenstate();
        break;
    case REM_STUCKSTATE:
        app->forgetStuckstate();
        break;
    case REM_DECOSTATE:
        app->forgetDecostate();
        break;
    case REM_SHADEDSTATE:
        app->forgetShadedstate();
        break;
//    case REM_TABSTATE:
//        break;
    case REM_JUMPWORKSPACE:
        app->forgetJumpworkspace();
        break;
    case REM_LAYER:
        app->forgetLayer();
        break;
    case REM_SAVEONCLOSE:
        app->forgetSaveOnClose();
        break;
    case REM_LASTATTRIB:
    default:
        // nothing
        break;
    }
}

void Remember::setupFrame(FluxboxWindow &win) {
    WinClient &winclient = win.winClient();
    // we don't touch the window if it is a transient
    // of something else


    if (winclient.transientFor())
        return;

    Application *app = find(winclient);
    if (app == 0)
        return; // nothing to do

    if (app->is_grouped && app->group == 0)
        app->group = &win;

    BScreen &screen = winclient.screen();

    if (app->workspace_remember) {
        // we use setWorkspace and not reassoc because we're still initialising
        win.setWorkspace(app->workspace);
        if (app->jumpworkspace_remember)
            screen.changeWorkspaceID(app->workspace);
    }

    if (app->head_remember) {
        win.screen().setOnHead<FluxboxWindow>(win, app->head);
    }

    if (app->decostate_remember)
        win.setDecorationMask(app->decostate);


    if (app->dimensions_remember)
        win.resize(app->w, app->h);

    int head = screen.getHead(win.fbWindow());

    if (app->position_remember) {

        switch (app->refc) {
          default:
          case POS_UPPERLEFT: // upperleft corner
              win.move(screen.getHeadX(head) + app->x,
                       screen.getHeadY(head) + app->y);
              break;
          case POS_UPPERRIGHT: // upperright corner
              win.move(screen.getHeadX(head) + screen.getHeadWidth(head) - win.width() - app->x,
                       screen.getHeadY(head) + app->y);
              break;
          case POS_LOWERLEFT: // lowerleft corner
              win.move(screen.getHeadX(head) + app->x,
                       screen.getHeadHeight(head) - win.height() - app->y);
              break;
          case POS_LOWERRIGHT: // lowerright corner
              win.move(screen.getHeadWidth(head) - win.width() - app->x,
                       screen.getHeadHeight(head) - win.height() - app->y);
              break;
          case POS_CENTER: // center of the screen, windows topleft corner is on the center
                win.move((screen.getHeadWidth(head) / 2) + app->x,
                         (screen.getHeadHeight(head) / 2) + app->y);
              break;
          case POS_WINCENTER: // the window is centered REALLY upon the center
                win.move((screen.getHeadWidth(head) / 2) - ( win.width() / 2 ) + app->x,
                         (screen.getHeadHeight(head) / 2) - ( win.height() / 2 ) + app->y);
              break;
        };
    }

    if (app->shadedstate_remember)
        // if inconsistent...
        if (win.isShaded() && !app->shadedstate ||
            !win.isShaded() && app->shadedstate)
            win.shade(); // toggles

    // external tabs aren't available atm...
    //if (app->tabstate_remember) ...

    if (app->stuckstate_remember)
        // if inconsistent...
        if (win.isStuck() && !app->stuckstate ||
            !win.isStuck() && app->stuckstate)
            win.stick(); // toggles
    if (app->focushiddenstate_remember)
        win.setFocusHidden(true);
    if (app->iconhiddenstate_remember)
        win.setIconHidden(true);

    if (app->layer_remember)
        win.moveToLayer(app->layer);

}

void Remember::setupClient(WinClient &winclient) {

    Application *app = find(winclient);
    if (app == 0)
        return; // nothing to do

    if (winclient.fbwindow() == 0 && app->is_grouped && app->group) {
        app->group->attachClient(winclient);
    }
}

void Remember::updateClientClose(WinClient &winclient) {
    Application *app = find(winclient);

    if (app && (app->save_on_close_remember && app->save_on_close)) {

        for (int attrib = 0; attrib <= REM_LASTATTRIB; attrib++) {
            if (isRemembered(winclient, (Attribute) attrib)) {
                rememberAttrib(winclient, (Attribute) attrib);
            }
        }

        save();
    }

    // we need to get rid of references to this client
    Clients::iterator wc_it = m_clients.find(&winclient);

    if (wc_it != m_clients.end()) {
        m_clients.erase(wc_it);
    }

}

void Remember::initForScreen(BScreen &screen) {
    // All windows get the remember menu.
    _FB_USES_NLS;
    screen.addExtraWindowMenu(_FBTEXT(Remember, MenuItemName, "Remember...", "Remember item in menu"),
                              createRememberMenu(screen));

}

void Remember::updateFrameClose(FluxboxWindow &win) {
    // scan all applications and remove this fbw if it is a recorded group
    Patterns::iterator it = m_pats.begin();
    while (it != m_pats.end()) {
        if (&win == it->second->group)
            it->second->group = 0;
        ++it;
    }
}
