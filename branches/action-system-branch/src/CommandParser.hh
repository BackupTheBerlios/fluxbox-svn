// CommandParser.hh for Fluxbox - an X11 Window manager
// Copyright (c) 2003 Henrik Kinnunen (fluxgen{<a*t>}users.sourceforge.net)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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

// $Id: CommandParser.hh,v 1.2.2.1 2003/10/28 21:34:52 rathnor Exp $

#ifndef COMMANDPARSER_HH
#define COMMANDPARSER_HH

#include <string>
#include <map>

#include "RefCount.hh"

namespace FbTk {
class Command;
class Action;
class ActionBinding;
};

/// Creates commands from command and argument.
/// Used for modules to add new commands in compile/run time
class CommandFactory {
public:
    CommandFactory();
    virtual ~CommandFactory();
    virtual FbTk::Command *stringToCommand(const std::string &command, 
                                           const std::string &arguments) = 0;
    // some actions need to know what binding they were called as
    virtual FbTk::Action  *stringToAction (const std::string &action, 
                                           const std::string &arguments,
                                           FbTk::ActionBinding *binding) = 0;

protected:
    void addCommand(const std::string &value);
};

/// Parses text into a command
class CommandParser {
public:
    typedef std::map<std::string, CommandFactory *> CommandFactoryMap;

    /// @return parses and returns a command matching the line
    FbTk::Command *parseCommand(const std::string &line);
    FbTk::Action  *parseAction (const std::string &line,
                                FbTk::ActionBinding *binding = 0);


    /// @return instance of command parser
    static CommandParser &instance();
private:
    // so CommandFactory can associate it's commands
    friend class CommandFactory;
    /// associate a command with a factory
    void associateCommand(const std::string &name, CommandFactory &factory);
    /// remove all associations with the factory
    void removeAssociations(CommandFactory &factory);

    /// search for a command in our command factory map
    FbTk::Command *toCommand(const std::string &command,
                             const std::string &arguments);
    FbTk::Action  *toAction (const std::string &action,
                             const std::string &arguments,
                             FbTk::ActionBinding *binding);
    
    CommandFactoryMap m_commandfactorys; ///< a string to factory map
    
};

#endif // COMMANDPARSER_HH
