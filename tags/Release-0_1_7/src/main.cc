// main.cc for Fluxbox Window manager
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen@linuxmail.org)
//
// main.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// $Id: main.cc,v 1.4 2002/02/02 21:54:31 pekdon Exp $

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#include "../version.h"

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "fluxbox.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_UNISTD_H
#include <sys/types.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN

#include <iostream>
using namespace std;

#ifdef DEBUG_UDS

#include <uds/init.hh>
#include <uds/uds.hh>

// configure UDS
uds::uds_flags_t uds::flags = uds::leak_check;

#endif //!DEBUG_UDS

int main(int argc, char **argv) {
	#ifdef DEBUG_UDS
	uds::Init uds_init;
	#endif //!DEBUG_UDS
	
	char *session_display = (char *) 0;
	char *rc_file = (char *) 0;

	NLSInit("blackbox.cat");
	I18n *i18n = I18n::instance();
	
	int i;
	for (i = 1; i < argc; ++i) {
		if (! strcmp(argv[i], "-rc")) {
		 // look for alternative rc file to use

			if ((++i) >= argc) {
				fprintf(stderr,
				i18n->getMessage(
#ifdef    NLS
				 mainSet, mainRCRequiresArg,
#else // !NLS
				 0, 0,
#endif // NLS
				 "error: '-rc' requires and argument\n"));

				::exit(1);
      }

			rc_file = argv[i];
    } else if (! strcmp(argv[i], "-display")) {
			// check for -display option... to run on a display other than the one
			// set by the environment variable DISPLAY

			if ((++i) >= argc) {
				fprintf(stderr,
					i18n->getMessage(
#ifdef    NLS
					 mainSet, mainDISPLAYRequiresArg,
#else // !NLS
					 0, 0,
#endif // NLS
					 "error: '-display' requires an argument\n"));

				::exit(1);
      }

      session_display = argv[i];
      char dtmp[MAXPATHLEN];
      sprintf(dtmp, "DISPLAY=%s", session_display);

      if (putenv(dtmp)) {
				fprintf(stderr,
					i18n->
					getMessage(
#ifdef    NLS
				   mainSet, mainWarnDisplaySet,
#else // !NLS
				   0, 0,
#endif // NLS
				   "warning: couldn't set environment variable 'DISPLAY'\n"));
				perror("putenv()");
      }
    } else if (! strcmp(argv[i], "-version")) {
			// print current version string
			printf("Fluxbox %s : (c) 2001-2002 Henrik Kinnunen \n\n",
						__fluxbox_version);

      ::exit(0);
    } else if (! strcmp(argv[i], "-help")) {
      // print program usage and command line options
      printf(i18n->
	     getMessage(
#ifdef    NLS
			mainSet, mainUsage,
#else // !NLS
			0, 0,
#endif // NLS
			"Fluxbox %s : (c) 2001-2002 Henrik Kinnunen\n\n"
			"  -display <string>\t\tuse display connection.\n"
			"  -rc <string>\t\t\tuse alternate resource file.\n"
			"  -version\t\t\tdisplay version and exit.\n"
			"  -help\t\t\t\tdisplay this help text and exit.\n\n"),
	     __fluxbox_version);

      // some people have requested that we print out command line options
      // as well
      printf(i18n->
	     getMessage(
#ifdef    NLS
			mainSet, mainCompileOptions,
#else // !NLS
			0, 0,
#endif // NLS
			"Compile time options:\n"
			"  Debugging:\t\t\t%s\n"
			"  Interlacing:\t\t\t%s\n"
			"  Shape:\t\t\t%s\n"
			"  Slit:\t\t\t\t%s\n"
			"  8bpp Ordered Dithering:\t%s\n\n"),
#ifdef    DEBUG
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonYes,
#else // !NLS
			      0, 0,
#endif // NLS
			      "yes"),
#else // !DEBUG
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonNo,
#else // !NLS
			      0, 0,
#endif // NLS
			      "no"),
#endif // DEBUG

#ifdef    INTERLACE
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonYes,
#else // !NLS
			      0, 0,
#endif // NLS
			      "yes"),
#else // !INTERLACE
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonNo,
#else // !NLS
			      0, 0,
#endif // NLS
			      "no"),
#endif // INTERLACE

#ifdef    SHAPE
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonYes,
#else // !NLS
			      0, 0,
#endif // NLS
			      "yes"),
#else // !SHAPE
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonNo,
#else // !NLS
			      0, 0,
#endif // NLS
			      "no"),
#endif // SHAPE

#ifdef    SLIT
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonYes,
#else // !NLS
			      0, 0,
#endif // NLS
			      "yes"),
#else // !SLIT
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonNo,
#else // !NLS
			      0, 0,
#endif // NLS
			      "no"),
#endif // SLIT

#ifdef    ORDEREDPSEUDO
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonYes,
#else // !NLS
			      0, 0,
#endif // NLS
			      "yes")
#else // !ORDEREDPSEUDO
	     i18n->getMessage(
#ifdef    NLS
			      CommonSet, CommonNo,
#else // !NLS
			      0, 0,
#endif // NLS
			      "no")
#endif // ORDEREDPSEUDO

	     );

      ::exit(0);
    }
  }

#ifdef    __EMX__
  _chdir2(getenv("X11ROOT"));
#endif // __EMX__
	Fluxbox *fluxbox=0;
	int exitcode=EXIT_SUCCESS;
  try	{
		
		fluxbox = new Fluxbox(argc, argv, session_display, rc_file);	  
		fluxbox->eventLoop();
		
	} catch (int _exitcode) {
		exitcode=_exitcode;
	} catch (...) {
		cerr<<"Fluxbox: Unknown error."<<endl;
	}
	
	if (fluxbox)
		delete fluxbox;
  exit(exitcode);
}
