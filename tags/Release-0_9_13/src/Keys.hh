// Keys.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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

#ifndef KEYS_HH
#define KEYS_HH

#include <string>
#include <vector>
#include <X11/Xlib.h>

#include "FbTk/NotCopyable.hh"
#include "FbTk/RefCount.hh"
#include "FbTk/Command.hh"
#include "FbTk/KeyUtil.hh"

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
       Load configuration from file
       @return true on success, else false
    */
    bool load(const char *filename = 0);
    /**
       Save keybindings to a file
       Note: the file will be overwritten
       @return true on success, else false
     */
    bool save(const char *filename = 0) const;
    /// bind a key action from a string
    /// @return false on failure
    bool addBinding(const std::string &binding);

    /**
       do action from XKeyEvent
    */
    void doAction(XKeyEvent &ke);

    /**
       Reload configuration from filename
       @return true on success, else false
    */
    bool reconfigure(const char *filename);
    const std::string filename() const { return m_filename; }
private:
    void deleteTree();

    void bindKey(unsigned int key, unsigned int mod);
    
    std::string m_filename;	
	
    class t_key {	
    public:
        t_key(unsigned int key, unsigned int mod, 
              FbTk::RefCount<FbTk::Command> command = FbTk::RefCount<FbTk::Command>(0));
        t_key(t_key *k);
        ~t_key();
		
        inline t_key *find(unsigned int key_, unsigned int mod_) {
            for (unsigned int i=0; i<keylist.size(); i++) {
                if (keylist[i]->key == key_ && keylist[i]->mod == FbTk::KeyUtil::instance().isolateModifierMask(mod_))
                    return keylist[i];				
            }			
            return 0;
        }
        inline t_key *find(XKeyEvent &ke) {
            for (unsigned int i=0; i<keylist.size(); i++) {
                if (keylist[i]->key == ke.keycode && 
                        keylist[i]->mod == FbTk::KeyUtil::instance().isolateModifierMask(ke.state))
                    return keylist[i];				
            }			
            return 0;
        }
			
        inline bool operator == (XKeyEvent &ke) const {
            return (mod == FbTk::KeyUtil::instance().isolateModifierMask(ke.state) && key == ke.keycode);
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

    std::vector<t_key *> m_keylist;	

    Display *m_display;  ///< display connection
    unsigned int m_current_line;
};

#endif // KEYS_HH
