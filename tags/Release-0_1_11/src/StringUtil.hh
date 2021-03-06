// StringUtil.hh for fluxbox 
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.	IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

//$Id: StringUtil.hh,v 1.9 2002/08/14 22:43:30 fluxgen Exp $

#ifndef STRINGUTIL_HH
#define STRINGUTIL_HH

#include <string>

namespace StringUtil
{

char *strdup(const char *);
	
//Similar to `strstr' but this function ignores the case of both strings
const char *strcasestr(const char *str, const char *ptn);
	
std::string expandFilename(const std::string &filename);
int getStringBetween(std::string& out, const char *instr, const char first, const char last,
	const char *ok_chars=" \t\n");

//--------- stringtok ----------------------------------
// Breaks a string into tokens
//--------------------------------------------------
template <typename Container>
static void
stringtok (Container &container, std::string const &in,
 	         const char * const delimiters = " \t\n")
{
	const std::string::size_type len = in.length();
	std::string::size_type i = 0;

	while ( i < len ) {
		// eat leading whitespace
		i = in.find_first_not_of(delimiters, i);
		if (i == std::string::npos)
			return;   // nothing left but white space

			// find the end of the token
			std::string::size_type j = in.find_first_of(delimiters, i);

			// push token
			if (j == std::string::npos) {
				container.push_back(in.substr(i));
				return;
			} else
				container.push_back(in.substr(i, j-i));

			// set up for next loop
			i = j + 1;
		}
}

};//end namespace StringUtil



#endif // STRINGUTIL_HH
