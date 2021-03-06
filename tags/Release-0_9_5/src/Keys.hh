// Keys.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at users.sourceforge.net)
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

// $Id: Keys.hh,v 1.27 2003/08/19 16:18:54 fluxgen Exp $

#ifndef KEYS_HH
#define KEYS_HH

#include <string>
#include <vector>
#include <X11/Xlib.h>

#include "NotCopyable.hh"
#include "RefCount.hh"
namespace FbTk {
class Command;
};

class Keys:private FbTk::NotCopyable  {
public:

    /**
       Constructor
       @param display display connection
       @param filename file to load, default none
    */
    explicit Keys(const char *filename=0);
    /// destructor
    ~Keys();

    /** 
        Strip out modifiers we want to ignore
        @return the cleaned state number
    */
    static unsigned int cleanMods(unsigned int mods)
        //remove numlock, capslock and scrolllock
        { return mods & (~s_capslock_mod & ~s_numlock_mod & ~s_scrolllock_mod); }

    unsigned int keycodeToModmask(unsigned int keycode);
    void loadModmap();

    /**
       Load configuration from file
       @return true on success, else false
    */
    bool load(const char *filename=0);
    /**
       do action from XKeyEvent
    */
    void doAction(XKeyEvent &ke);
    /**
       Reload configuration from filename
       @return true on success, else false
    */
    bool reconfigure(const char *filename);

private:
    void deleteTree();
    void ungrabKeys();
    void bindKey(unsigned int key, unsigned int mod);
    /**
       @param modstr modifier string (i.e Mod4, Mod5)
       @return modifier number that match modstr
    */
    unsigned int getModifier(const char *modstr);
    /**
       @param keystr a key string (i.e F1, Enter)
       @return key number that match keystr
    */
    unsigned int getKey(const char *keystr);
    /**
       grab a key
       @param key the key
       @param mod the modifier
    */
    void grabKey(unsigned int key, unsigned int mod);
    std::string filename;	
	
    class t_key {	
    public:
        t_key(unsigned int key, unsigned int mod, 
              FbTk::RefCount<FbTk::Command> command = FbTk::RefCount<FbTk::Command>(0));
        t_key(t_key *k);
        ~t_key();
		
        inline t_key *find(unsigned int key_, unsigned int mod_) {
            for (unsigned int i=0; i<keylist.size(); i++) {
                if (keylist[i]->key == key_ && keylist[i]->mod == mod_)
                    return keylist[i];				
            }			
            return 0;
        }
        inline t_key *find(XKeyEvent &ke) {
            for (unsigned int i=0; i<keylist.size(); i++) {
                if (keylist[i]->key == ke.keycode && keylist[i]->mod == ke.state)
                    return keylist[i];				
            }			
            return 0;
        }
			
        inline bool operator == (XKeyEvent &ke) const {
            return (mod == ke.state && key == ke.keycode);
        }
		
        FbTk::RefCount<FbTk::Command> m_command;
        unsigned int key;
        unsigned int mod;
        std::vector<t_key *> keylist;
    };

    /**
       merge two linked list
       @return true on success, else false
    */
    bool mergeTree(t_key *newtree, t_key *basetree=0);

    static int s_capslock_mod, s_numlock_mod, s_scrolllock_mod; ///< modifiers
		
    std::vector<t_key *> m_keylist;	
    std::string m_execcmdstring; ///< copy of the execcommandstring
    int m_param;                 ///< copy of the param argument
    Display *m_display;          ///< display connection
    XModifierKeymap *m_modmap;   ///< Modifier->keycode mapping
};

#endif // KEYS_HH
