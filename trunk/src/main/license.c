/*
 * Output software license information to stdout.
 *
 * Copyright 2002 Andrew Wood, distributed under the Artistic License.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>


/*
 * Display software license(s).
 */
void display_license(void)
{
	printf(_("%s %s - Copyright (C) %s %s"), PROGRAM_NAME, VERSION,
	  COPYRIGHT_YEAR, COPYRIGHT_HOLDER);
	printf("\n\n");
	printf(_("This program is Open Source software, and is being "
	  "distributed under the\nterms of the Artistic License."));
	printf("\n\n");
	printf(_("Parts of this program may be copyright (C) Free Software\n"
	  "Foundation, Inc. and are distributed under the terms of the\n"
	  "GNU Lesser General Public License."));
	printf("\n\n");
	printf("----------------------------------------------------------");
	printf("\n\n");
	printf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", _(
"The Artistic License\n"
"\n"
"   Preamble\n"
"\n"
"   The intent of this document is to state the conditions under which a\n"
"   Package may be copied, such that the Copyright Holder maintains some\n"
"   semblance of artistic control over the development of the package,\n"
"   while giving the users of the package the right to use and distribute\n"
"   the Package in a more-or-less customary fashion, plus the right to\n"
"   make reasonable modifications.\n"
"\n"), _(
"   Definitions:\n"
"\n"
"     * \"Package\" refers to the collection of files distributed by the\n"
"       Copyright Holder, and derivatives of that collection of files\n"
"       created through textual modification.\n"
"\n"), _(
"     * \"Standard Version\" refers to such a Package if it has not been\n"
"       modified, or has been modified in accordance with the wishes of\n"
"       the Copyright Holder.\n"
"\n"), _(
"     * \"Copyright Holder\" is whoever is named in the copyright or\n"
"       copyrights for the package.\n"
"\n"), _(
"     * \"You\" is you, if you're thinking about copying or distributing\n"
"       this Package.\n"
"\n"), _(
"     * \"Reasonable copying fee\" is whatever you can justify on the basis\n"
"       of media cost, duplication charges, time of people involved, and\n"
"       so on. (You will not be required to justify it to the Copyright\n"
"       Holder, but only to the computing community at large as a market\n"
"       that must bear the fee.)\n"
"\n"), _(
"     * \"Freely Available\" means that no fee is charged for the item\n"
"       itself, though there may be fees involved in handling the item. It\n"
"       also means that recipients of the item may redistribute it under\n"
"       the same conditions they received it.\n"
"\n"), _(
"   1. You may make and give away verbatim copies of the source form of\n"
"   the Standard Version of this Package without restriction, provided\n"
"   that you duplicate all of the original copyright notices and\n"
"   associated disclaimers.\n"
"\n"), _(
"   2. You may apply bug fixes, portability fixes and other modifications\n"
"   derived from the Public Domain or from the Copyright Holder. A Package\n"
"   modified in such a way shall still be considered the Standard Version.\n"
"\n"), _(
"   3. You may otherwise modify your copy of this Package in any way,\n"
"   provided that you insert a prominent notice in each changed file\n"
"   stating how and when you changed that file, and provided that you do\n"
"   at least ONE of the following:\n"
"\n"), _(
"     a) place your modifications in the Public Domain or otherwise make\n"
"     them Freely Available, such as by posting said modifications to\n"
"     Usenet or an equivalent medium, or placing the modifications on a\n"
"     major archive site such as ftp.uu.net, or by allowing the Copyright\n"
"     Holder to include your modifications in the Standard Version of the\n"
"     Package.\n"
"\n"), _(
"     b) use the modified Package only within your corporation or\n"
"     organization.\n"
"\n"
"     c) rename any non-standard executables so the names do not conflict\n"
"     with standard executables, which must also be provided, and provide\n"
"     a separate manual page for each non-standard executable that\n"
"     clearly documents how it differs from the Standard Version.\n"
"\n"
"     d) make other distribution arrangements with the Copyright Holder.\n"
"\n"), _(
"   4. You may distribute the programs of this Package in object code or\n"
"   executable form, provided that you do at least ONE of the following:\n"
"\n"
"     a) distribute a Standard Version of the executables and library\n"
"     files, together with instructions (in the manual page or\n"
"     equivalent) on where to get the Standard Version.\n"
"\n"
"     b) accompany the distribution with the machine-readable source of\n"
"     the Package with your modifications.\n"
"\n"), _(
"     c) accompany any non-standard executables with their corresponding\n"
"     Standard Version executables, giving the non-standard executables\n"
"     non-standard names, and clearly documenting the differences in\n"
"     manual pages (or equivalent), together with instructions on where\n"
"     to get the Standard Version.\n"
"\n"
"     d) make other distribution arrangements with the Copyright Holder.\n"
"\n"), _(
"   5. You may charge a reasonable copying fee for any distribution of\n"
"   this Package. You may charge any fee you choose for support of this\n"
"   Package. You may not charge a fee for this Package itself. However,\n"
"   you may distribute this Package in aggregate with other (possibly\n"
"   commercial) programs as part of a larger (possibly commercial)\n"
"   software distribution provided that you do not advertise this Package\n"
"   as a product of your own.\n"
"\n"), _(
"   6. The scripts and library files supplied as input to or produced as\n"
"   output from the programs of this Package do not automatically fall\n"
"   under the copyright of this Package, but belong to whomever generated\n"
"   them, and may be sold commercially, and may be aggregated with this\n"
"   Package.\n"
"\n"), _(
"   7. C or perl subroutines supplied by you and linked into this Package\n"
"   shall not be considered part of this Package.\n"
"\n"), _(
"   8. The name of the Copyright Holder may not be used to endorse or\n"
"   promote products derived from this software without specific prior\n"
"   written permission.\n"
"\n"), _(
"   9. THIS PACKAGE IS PROVIDED \"AS IS\" AND WITHOUT ANY EXPRESS OR IMPLIED\n"
"   WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF\n"
"   MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.\n"
"\n"
"   The End"));
	printf("\n\n");
	printf("----------------------------------------------------------");
	printf("\n\n");
	printf(_(
"For a copy of the GNU Lesser General Public License, please see\n"
"http://www.gnu.org/ or write to the Free Software Foundation, Inc.,\n"
"59 Temple Place - Suite 330, Boston, MA 02111-1307, USA."));
	printf("\n\n");
	printf("----------------------------------------------------------");
	printf("\n\n");
	printf(_(
"If that scrolled by too quickly, you may want to pipe it to a\n"
"pager such as `more' or `less', eg `%s --license | more'."),
	  PROGRAM_NAME);
	printf("\n");
}

/* EOF $Id$ */
