#!/usr/bin/perl -w
#
# Change some of the crappier things about texi2html's formatting.
#

$body = 0;

while (<>) {

	$body = 1 if (/^<BODY>$/);

	if ($body > 0) {
		$body ++ if (/^<P><HR><P>$/);
		if ($body > 1) {
			print "<BODY>\n<!-- start -->\n";
			$body = 0;
		}
		next;
	}

	s@</P>@<P>@g;
	s@<H1>@<P><HR><P><H1 ALIGN="LEFT">@g;
	s@<PRE>@<P><PRE>@g;
	s@<UL>@<P><UL>@g;
	s@<DL@<P><DL@g;
	if (/^<\/BODY>$/) {
		print 'It was then munged by a Perl script to stop it ' .
		  'looking horrible in <A HREF="http://lynx.browser.org/">' .
		  "Lynx</a>.\n<!-- end -->\n";
	}
	print;
}

# EOF
