#
# Targets.
#

$(package): src/main.o src/library.o @NLSOBJ@
	$(CC) $(LINKFLAGS) $(CFLAGS) -o $@ src/main.o src/library.o @NLSOBJ@ $(LIBS)

$(package)-static: src/main.o src/library.o @NLSOBJ@
	$(CC) $(LINKFLAGS) $(CFLAGS) -static -o $@ src/main.o src/library.o @NLSOBJ@ $(LIBS)

# EOF
