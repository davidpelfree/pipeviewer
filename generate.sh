#!/bin/sh
#
# Script to autogenerate the `configure' script, and rebuild the dependency
# list.
#
# $Id$

set -e

cd autoconf
autoconf configure.in > ../configure
cd ..
chmod 755 configure

echo -n > autoconf/make/depend.mk~
echo -n > autoconf/make/filelist.mk~
echo -n > autoconf/make/modules.mk~

rm -rf .gen
mkdir .gen
cd .gen
sh ../configure
make make
make dep
cd ..
rm -rf .gen

# EOF $Id$
