#
# Targets.
#
# $Id$

objgetopt = @needgetopt@
intldeps = @INTLDEPS@
intlobjs = @INTLOBJS@

$(package): src/main.o $(intldeps) $(intlobjs) $(objgetopt)
	$(CC) $(CFLAGS) -o $@ src/main.o $(intldeps) $(intlobjs) $(objgetopt) $(LIBS)

$(package)-static: src/main.o $(intldeps) $(intlobjs) $(objgetopt)
	$(CC) $(CFLAGS) -static -o $@ src/main.o $(intldeps) $(intlobjs) $(objgetopt) $(LIBS)

# EOF $Id$
