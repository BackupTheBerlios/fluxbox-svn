// Key2.cc for Fluxbox - an X11 Window manager
// Copyright (c) 2001 Henrik Kinnunen (fluxgen@linuxmail.org)
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


#ifdef		HAVE_CONFIG_H
#	 include "config.h"
#endif

#ifdef		HAVE_STDIO_H
#	 include <stdio.h>
#endif									// HAVE_STDIO_H

#ifdef		HAVE_CTYPE_H
#	 include <ctype.h>
#endif									// HAVE_CTYPE_H

#ifdef		STDC_HEADERS
#	 include <stdlib.h>
#	 include <string.h>
#  include <errno.h>
#endif									// STDC_HEADERS

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#ifdef		HAVE_SYS_TYPES_H
#	 include <sys/types.h>
#endif									// HAVE_SYS_TYPES_H

#ifdef		HAVE_SYS_WAIT_H
#	 include <sys/wait.h>
#endif									// HAVE_SYS_WAIT_H

#ifdef		HAVE_UNISTD_H
#	 include <unistd.h>
#endif									// HAVE_UNISTD_H

#ifdef		HAVE_SYS_STAT_H
#	 include <sys/stat.h>
#endif									// HAVE_SYS_STAT_H

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>

#include "Keys.hh"
#include "fluxbox.hh"

#include <iostream>
#include <fstream>

using namespace std;

Keys::t_actionstr Keys::m_actionlist[] = {
		{"Minimize", grabIconify},
		{"Raise", grabRaise},
		{"Lower", grabLower},
		{"Close", grabClose},
		{"AbortKeychain", grabAbortKeychain},
		{"Workspace1", grabWorkspace1},
		{"Workspace2", grabWorkspace2},
		{"Workspace3", grabWorkspace3},
		{"Workspace4", grabWorkspace4},
		{"Workspace5", grabWorkspace5},
		{"Workspace6", grabWorkspace6},
		{"Workspace7", grabWorkspace7},
		{"Workspace8", grabWorkspace8},
		{"Workspace9", grabWorkspace9},
		{"Workspace10", grabWorkspace10},
		{"Workspace11", grabWorkspace11},
		{"Workspace12",  grabWorkspace12},
		{"NextWorkspace", grabNextWorkspace},
		{"PrevWorkspace", grabPrevWorkspace},
		{"LeftWorkspace", grabLeftWorkspace},
		{"RightWorkspace", grabRightWorkspace},
		{"KillWindow", grabKillWindow},
		{"NextWindow", grabNextWindow},
		{"PrevWindow", grabPrevWindow},
		{"NextTab", grabNextTab},
		{"PrevTab", grabPrevTab},
		{"ShadeWindow", grabShade},
		{"MaximizeWindow", grabMaximize},
		{"StickWindow", grabStick},
		{"ExecCommand", grabExecute},
		{"MaximizeVertical", grabVertMax},
		{"MaximizeHorizontal", grabHorizMax},
		{"NudgeRight", grabNudgeRight},
		{"NudgeLeft", grabNudgeLeft},
		{"NudgeUp", grabNudgeUp},
		{"NudgeDown", grabNudgeDown},
		{"BigNudgeRight", grabBigNudgeRight},
		{"BigNudgeLeft", grabBigNudgeLeft},
		{"BigNudgeUp", grabBigNudgeUp},
		{"BigNudgeDown", grabBigNudgeDown},
		{"HorizontalIncrement", grabHorizInc},
		{"VerticalIncrement", grabVertInc},
		{"HorizontalDecrement", grabHorizDec},
		{"VerticalDecrement", grabVertDec},
		{"ToggleDecor", grabToggleDecor},	
		{0, lastKeygrab}
		};	

Keys::Keys(char *filename) {
	m_abortkey=0;	
	load(filename);
}

Keys::~Keys() {
	deleteTree();
}
//--------- deleteTree -----------
// Destroys the keytree and m_abortkey
//--------------------------------
void Keys::deleteTree() {
	while (!m_keylist.empty()) {
		if (m_keylist.back())
			delete m_keylist.back();		
		m_keylist.pop_back();
	}
	if (m_abortkey) {
		delete m_abortkey;
		m_abortkey=0;
	}	
}
//-------------- load ----------------
// Load and grab keys
// Returns true on success else false
// TODO: error checking and nls on them?
// and possible replacement of strtok
//------------------------------------
bool Keys::load(char *filename) {
	if (!filename)
		return false;
	
	Fluxbox *fluxbox = Fluxbox::instance();
	Display *display = fluxbox->getXDisplay();
	ScreenInfo *screeninfo=0;
	//ungrab all keys
	int screen=0;
	while ((screeninfo = fluxbox->getScreenInfo(screen++)) ) {
		XUngrabKey(display, AnyKey, AnyModifier,
		screeninfo->getRootWindow());		
	}
	
	XSync(display, False);
						
	//open the file
	ifstream infile(filename);
	if (!infile)
		return false;
	
	
	char *linebuffer = new char[1024];
	int line=0;
	int linepos=0; //position in the line

	while (!infile.eof()) {
		infile.getline(linebuffer, 1024);		

		line++;
		char *val = strtok(linebuffer, " ");
		linepos = (val==0 ? 0 : strlen(val) + 1);	

		int numarg = 1;
		unsigned int key=0, mod=0;		
		char keyarg=0;
		t_key *current_key=0, *last_key=0;
		
		while (val) {			

			if (val[0]!=':') {
				keyarg++;
				if (keyarg==1) //first arg is modifier
					mod = getModifier(val);					
				else if (keyarg>1) {
					
					//keyarg=0;
					key = getKey(val);
					if (!key){ //if no keycode was found try the modifier
						int tmpmod = getModifier(val);
						if (tmpmod)
							mod |= tmpmod; //add it to modifier
							
					} else if (!current_key) {
									
						current_key = new t_key(key, mod);
						last_key = current_key;
						
					} else { 
						
						t_key *temp_key = new t_key(key, mod);
						last_key->keylist.push_back(temp_key);
						last_key = temp_key;
					}

				}			

			} else {
				
				val++; //ignore the ':'

				unsigned int i=0;

				for (i=0; i< lastKeygrab; i++) {
					if (strcasecmp(m_actionlist[i].string, val) == 0)
						break;	
				}

				if (i < lastKeygrab ) {
					if (!current_key) {
						cerr<<"Error on line: "<<line<<endl;
						cerr<<linebuffer<<endl;
						delete current_key;
						current_key = 0;
						last_key = 0;
						break; //break out and read next line
					}

					//special case for grabAbortKeychain
					if (m_actionlist[i].action == grabAbortKeychain) {
						if (last_key!=current_key)
							cerr<<"Keys: "<<m_actionlist[i].string<<" cant be in chained mode"<<endl;
						else if (m_abortkey)
							cerr<<"Keys: "<<m_actionlist[i].string<<" is already bound."<<endl;
						else
							m_abortkey = new t_key(current_key->key, current_key->mod, grabAbortKeychain);

						delete current_key;
						current_key = 0;
						last_key = 0;
						break; //break out and read next line
					}
					
					last_key->action = m_actionlist[i].action;
					if (last_key->action == grabExecute)
						last_key->execcommand = &linebuffer[linepos];

					//add the keychain to list										
					if (!mergeTree(current_key))
						cerr<<"Keys: Faild to merge keytree!"<<endl;
		
					#ifdef DEBUG
				  if (m_actionlist[i].action == Keys::grabExecute) {					
						
						cerr<<"linepos:"<<linepos<<endl;
						cerr<<"buffer:"<<&linebuffer[linepos]<<endl;
						cerr<<"command:"<<last_key->execcommand<<endl;
		
					}
					#endif
				
					//clear keypointers now that we have them in m_keylist					
					delete current_key;
					current_key = 0;
					last_key = 0;
					
				}	else { //destroy list if no action is found
					#ifdef DEBUG
					cerr<<"Didnt find action="<<val<<endl;									
					#endif					
					//destroy current_key ... this will also destroy the last_key										
					delete current_key;
					current_key = 0;
					last_key = 0;
				}
				
				break; //dont process this linebuffer more
			}			
			numarg++;
			val = strtok(0, " ");			
			linepos += (val == 0 ? 0 : strlen(val) + 1);
		}
	}
	
	delete linebuffer;
	#ifdef DEBUG
	showTree();
	#endif
	return true;
}

//--------- grabKey ---------------
// Grabs a key with the modifier
// and with numlock,capslock and scrollock
//---------------------------------
void Keys::grabKey(unsigned int key, unsigned int mod) {
	
	Fluxbox *fluxbox = Fluxbox::instance();	
	Display *display = fluxbox->getXDisplay();
	
	#ifdef DEBUG
	cerr<<__FILE__<<"("<<__LINE__<<"): keycode "<<key<<" mod "<<hex<<mod<<dec<<endl;
	#endif	
	int i=0;
	ScreenInfo *screeninfo=0;
	
	while ((screeninfo = fluxbox->getScreenInfo(i++)) ) {
		Window root = screeninfo->getRootWindow();
		XGrabKey(display, key, mod,
							root, True,
							GrabModeAsync, GrabModeAsync);
						
		// Grab with numlock, capslock and scrlock	

		//numlock	
		XGrabKey(display, key, mod|Mod2Mask,
							root, True,
							GrabModeAsync, GrabModeAsync);		
		//scrolllock
		XGrabKey(display, key, mod|Mod5Mask,
						root, True,
						GrabModeAsync, GrabModeAsync);	
		//capslock
		XGrabKey(display, key, mod|LockMask,
							root, True,
							GrabModeAsync, GrabModeAsync);
	
		//capslock+numlock
		XGrabKey(display, key, mod|LockMask|Mod2Mask,
							root, True,
							GrabModeAsync, GrabModeAsync);

		//capslock+scrolllock
		XGrabKey(display, key, mod|LockMask|Mod5Mask,
							root, True,
							GrabModeAsync, GrabModeAsync);						
	
		//capslock+numlock+scrolllock
		XGrabKey(display, key, mod|Mod2Mask|Mod5Mask|LockMask,
							root, True,
							GrabModeAsync, GrabModeAsync);						

		//numlock+scrollLock
		XGrabKey(display, key, mod|Mod2Mask|Mod5Mask,
							root, True,
							GrabModeAsync, GrabModeAsync);
	
	}
			
}

//------------ getModifier ---------------
// Returns the modifier for the modstr
// else zero on failure.
// TODO fix more masks
//----------------------------------------
unsigned int Keys::getModifier(char *modstr) {
	if (!modstr)
		return 0;
	struct t_modlist{
		char *string;
		unsigned int mask;
		bool operator == (char *modstr) {
			return  (strcasecmp(string, modstr) == 0 && mask !=0);
		}
	} modlist[] = {
			{"SHIFT", ShiftMask},
			{"CONTROL", ControlMask},
			{"MOD1", Mod1Mask},
			{"MOD2", Mod2Mask},
			{"MOD3", Mod3Mask},
			{"MOD4", Mod4Mask},
			{"MOD5", Mod5Mask},
			{0, 0}
		};
		
	for (unsigned int i=0; modlist[i].string!=0; i++) {
		if (modlist[i]==modstr)		
			return modlist[i].mask;		
	}
	
	return 0;	
}

//----------- getKey ----------------
// Returns keycode of keystr on success
// else it returns zero
//-----------------------------------
unsigned int Keys::getKey(char *keystr) {
	if (!keystr)
		return 0;
	return XKeysymToKeycode(Fluxbox::instance()->getXDisplay(),
							 XStringToKeysym
							 (keystr));
}

//--------- getAction -----------------
// returns the KeyAction of the XKeyEvent
//-------------------------------------
Keys::KeyAction Keys::getAction(XKeyEvent *ke) {	
	static t_key *next_key = 0;
	//remove numlock, capslock and scrolllock
	ke->state &= ~Mod2Mask & ~Mod5Mask & ~LockMask;
	
	if (m_abortkey && *m_abortkey==ke) { //abort current keychain
		next_key = 0;		
		return m_abortkey->action;
	}
	
	if (!next_key) {
	
		for (unsigned int i=0; i<m_keylist.size(); i++) {
			if (*m_keylist[i] == ke) {
				if (m_keylist[i]->keylist.size()) {
					next_key = m_keylist[i];
					break; //end for-loop 
				} else {
					if (m_keylist[i]->action == grabExecute)
						m_execcmdstring = m_keylist[i]->execcommand; //update execcmdstring if action is grabExecute
					return m_keylist[i]->action;
				}
			}
		}
	
	} else { //check the nextkey
		t_key *temp_key = next_key->find(ke);
		if (temp_key) {
			if (temp_key->keylist.size()) {
				next_key = temp_key;								
			} else {
				next_key = 0;
				if (temp_key->action == grabExecute)
					m_execcmdstring = temp_key->execcommand; //update execcmdstring if action is grabExecute
				return temp_key->action;
			}
		}  else  {
			temp_key = next_key;		
			next_key = 0;
			if (temp_key->action == grabExecute)
				m_execcmdstring = temp_key->execcommand; //update execcmdstring if action is grabExecute
			return temp_key->action;				
		}
		
	}
	
	return lastKeygrab;
}

//--------- reconfigure -------------
// deletes the tree and load configuration
// returns true on success else false
//-----------------------------------
bool Keys::reconfigure(char *filename) {
	deleteTree();
	return load(filename);
}

//------------- getActionStr ------------------
// Tries to find the action for the key
// Returns actionstring on success else
// 0 on failure
//---------------------------------------------
const char *Keys::getActionStr(KeyAction action) {
	for (unsigned int i=0; m_actionlist[i].string!=0 ; i++) {
		if (m_actionlist[i].action == action) 
			return m_actionlist[i].string;
	}
	
	return 0;
}

#ifdef DEBUG
//--------- showTree -----------
// Debug function that show the
// keytree. Starts with the
// rootlist
//------------------------------
void Keys::showTree() {
	for (unsigned int i=0; i<m_keylist.size(); i++) {
		if (m_keylist[i]) {
			cerr<<i<<" ";
			showKeyTree(m_keylist[i]);	
		} else
			cerr<<"Null @ "<<i<<endl;
	}
}

//---------- showKeyTree --------
// Debug function to show t_key tree
//-------------------------------
void Keys::showKeyTree(t_key *key, unsigned int w) {	
	for (unsigned int i=0; i<w+1; i++)
		cerr<<"-";	
	if (!key->keylist.empty()) {
		for (unsigned int i=0; i<key->keylist.size(); i++) {
			cerr<<"( "<<(int)key->key<<" "<<(int)key->mod<<" )";
			showKeyTree(key->keylist[i], 4);		
			cerr<<endl;
		}
	} else
		cerr<<"( "<<(int)key->key<<" "<<(int)key->mod<<" ) {"<<getActionStr(key->action)<<"}"<<endl;
}
#endif //DEBUG
//------------ mergeTree ---------------
// Merges two chains and binds new keys
// Returns true on success else false.
//---------------------------------------
bool Keys::mergeTree(t_key *newtree, t_key *basetree) {
	if (basetree==0) {
		unsigned int baselist_i=0;
		for (; baselist_i<m_keylist.size(); baselist_i++) {
			if (m_keylist[baselist_i]->mod == newtree->mod && 
					m_keylist[baselist_i]->key == newtree->key) {
				if (newtree->keylist.size() && m_keylist[baselist_i]->action == lastKeygrab) {
					//assumes the newtree only have one branch
					return mergeTree(newtree->keylist[0], m_keylist[baselist_i]);
				} else
					break;
			}
		}

		if (baselist_i == m_keylist.size()) {
			grabKey(newtree->key, newtree->mod);
			m_keylist.push_back(new t_key(newtree));			
			if (newtree->keylist.size())
				return mergeTree(newtree->keylist[0], m_keylist.back());
			return true;
		}
		
	} else {
		unsigned int baselist_i = 0;
		for (; baselist_i<basetree->keylist.size(); baselist_i++) {
			if (basetree->keylist[baselist_i]->mod == newtree->mod &&
					basetree->keylist[baselist_i]->key == newtree->key) {
				if (newtree->keylist.size()) {
					//assumes the newtree only have on branch
					return mergeTree(newtree->keylist[0], basetree->keylist[baselist_i]);
				} else
					return false;
			}					
		}
		//if it wasn't in the list grab the key and add it to the list
		if (baselist_i==basetree->keylist.size()) {			
			grabKey(newtree->key, newtree->mod);
			basetree->keylist.push_back(new t_key(newtree));
			if (newtree->keylist.size())
				return mergeTree(newtree->keylist[0], basetree->keylist.back());
			return true;		
		}		
	}
	
	return false;
}

Keys::t_key::t_key(unsigned int key_, unsigned int mod_, KeyAction action_) {
	action = action_;
	key = key_;
	mod = mod_;	
}

Keys::t_key::t_key(t_key *k) {
	action = k->action;
	key = k->key;
	mod = k->mod;
	execcommand = k->execcommand;
}

Keys::t_key::~t_key() {	
	while (!keylist.empty()) {		
		t_key *k = keylist.back();
		delete k;
		keylist.pop_back();				
	}
}
