// CommandParser.cc for Fluxbox - an X11 Window manager
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

// $Id: CommandParser.cc,v 1.4.2.1 2003/10/28 21:34:52 rathnor Exp $

#include "CommandParser.hh"

#include "StringUtil.hh"

#include <vector>
using namespace std;

namespace {

string::size_type removeFirstWhitespace(std::string &str) {
    string::size_type first_pos = str.find_first_not_of(" \t");
    if (first_pos != string::npos)
        str.erase(0, first_pos);
    return first_pos;
}

}; // end anonymous namespace

CommandFactory::CommandFactory() {

}

CommandFactory::~CommandFactory() {
    // remove all associations with this factory
    CommandParser::instance().removeAssociations(*this);
}

void CommandFactory::addCommand(const std::string &command_name) {
    CommandParser::instance().associateCommand(command_name, *this);
}

CommandParser &CommandParser::instance() {
    static CommandParser singleton;
    return singleton;
}

FbTk::Command *CommandParser::parseCommand(const std::string &line) {

    // separate arguments and command
    string command = line;
    string arguments;
    string::size_type first_pos = removeFirstWhitespace(command);
    string::size_type second_pos = command.find_first_of(" \t", first_pos);
    if (second_pos != string::npos) {
        // ok we have arguments, parsing them here
        arguments = command.substr(second_pos);
        removeFirstWhitespace(arguments);
        command.erase(second_pos); // remove argument from command
    }

    // now we have parsed command and arguments

    command = FbTk::StringUtil::toLower(command);

    // we didn't find any matching command in default commands,
    // so we search in the command creators modules for a 
    // matching command string
    return toCommand(command, arguments);

}

// binding that this action is associated with
FbTk::Action *CommandParser::parseAction(const std::string &line, FbTk::ActionBinding *binding) {

    // separate arguments and command
    string command = line;
    string arguments;
    string::size_type first_pos = removeFirstWhitespace(command);
    string::size_type second_pos = command.find_first_of(" \t", first_pos);
    if (second_pos != string::npos) {
        // ok we have arguments, parsing them here
        arguments = command.substr(second_pos);
        removeFirstWhitespace(arguments);
        command.erase(second_pos); // remove argument from command
    }

    // now we have parsed command and arguments

    command = FbTk::StringUtil::toLower(command);

    // we didn't find any matching command in default commands,
    // so we search in the command creators modules for a 
    // matching command string
    return toAction(command, arguments, binding);

}

FbTk::Command *CommandParser::toCommand(const std::string &command_str, const std::string &arguments) {
    if (m_commandfactorys[command_str] != 0)
        return m_commandfactorys[command_str]->stringToCommand(command_str, arguments);

    return 0;
}

FbTk::Action *CommandParser::toAction(const std::string &command_str,
                                      const std::string &arguments,
                                      FbTk::ActionBinding *binding) {

    if (m_commandfactorys[command_str] != 0)
        return m_commandfactorys[command_str]->stringToAction(command_str, arguments, binding);

    return 0;
}


void CommandParser::associateCommand(const std::string &command, CommandFactory &factory) {
    // we shouldnt override other commands
    if (m_commandfactorys[command] != 0)
        return;

    m_commandfactorys[command] = &factory;
}

void CommandParser::removeAssociations(CommandFactory &factory) {
    // commands that are associated with the factory
    std::vector<std::string> commands; 
    // find associations
    CommandFactoryMap::iterator factory_it = m_commandfactorys.begin();
    const CommandFactoryMap::iterator factory_it_end = m_commandfactorys.end();
    for (; factory_it != factory_it_end; ++factory_it) {
        if ((*factory_it).second == &factory)
            commands.push_back((*factory_it).first);
    }
    // remove all associations
    while (!commands.empty()) {
        m_commandfactorys.erase(commands.back());
        commands.pop_back();
    }
}
