#
# Targets.
#
# $Id$

objgetopt = @needgetopt@
intldeps = @INTLDEPS@
intlobjs = @INTLOBJS@

$(package): src/main.o $(intldeps) $(intlobjs) $(objgetopt)
	$(CC) $(FINALFLAGS) -o $@ src/main.o $(intldeps) $(intlobjs) $(objgetopt)

$(package)-static: src/main.o $(intldeps) $(intlobjs) $(objgetopt)
	$(CC) -static -o $@ src/main.o $(intldeps) $(intlobjs) $(objgetopt) $(FINALFLAGS)

# EOF $Id$
