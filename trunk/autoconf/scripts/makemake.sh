#!/bin/sh
#
# Generate Makefile dependencies inclusion and module target file "depend.mk~"
# by scanning the directory "src" for files ending in ".c" and ".d", and for
# subdirectories not starting with "_".
#
# Modules live inside subdirectories called [^_]* - i.e. a directory "foo" will
# have a rule created which links all code inside it to "foo.o".
#
# The directory "src/include" is never scanned; neither are CVS directories.
#
# $Id$

outlist=$1
outlink=$2

FIND=find
which gfind >/dev/null 2>&1 && FIND=gfind

echo '# Automatically generated file listings' > $outlist
echo '#' >> $outlist
echo "# Creation time: `date`" >> $outlist
echo >> $outlist

echo '# Automatically generated module linking rules' > $outlink
echo '#' >> $outlink
echo "# Creation time: `date`" >> $outlink
echo >> $outlink

echo -n "Scanning for source files: "

allsrc=`$FIND src -type f -name "*.c" -print`
allobj=`echo $allsrc | tr ' ' '\n' | sed 's/\.c$/.o/'`
alldep=`echo $allsrc | tr ' ' '\n' | sed 's/\.c$/.d/'`

echo `echo $allsrc | wc -w | tr -d ' '` found

echo -n "Scanning for modules: "

modules=`$FIND src -mindepth 1 -type d -name "[^_]*" -print  \
         | grep -v '^src/include' | grep -v 'CVS' \
         | while read DIR; do \
           CONTENT=\$(/bin/ls -d \$DIR/* \
                     | grep -v -e '.po' -e '.gmo' -e '.mo' -e '.h' \
                     | sed -n '$p'); \
           [ -n "\$CONTENT" ] || continue; \
           echo \$DIR; \
	   done
         `

echo `echo $modules | wc -w | tr -d ' '` found

echo "Writing module linking rules"

echo -n [
for i in $modules; do echo -n ' '; done
echo -n -e ']\r['

for i in $modules; do
  echo -n '.'
  allobj="$allobj $i.o"
  deps=`$FIND $i -type f -name "*.c" -maxdepth 1 -print \
        | sed -e 's@\.c$@.o@' | tr '\n' ' '`
  deps="$deps `$FIND $i -type d -name "[^_]*" \
               -maxdepth 1 -mindepth 1 -print \
               | grep -v 'CVS' \
               | while read DIR; do \
                 CONTENT=\$(/bin/ls -d \$DIR/* \
                            | grep -v -e '.po' -e '.gmo' -e '.mo' -e '.h' \
                            | sed -n '$p'); \
                  [ -n "\$CONTENT" ] || continue; \
                  echo \$DIR; \
       	          done \
               | sed -e 's@$@.o@' \
               | tr '\n' ' '`"
  echo "$i.o: $deps" >> $outlink
  echo '	$(LD) $(LDFLAGS) -o $@' "$deps" >> $outlink
  echo >> $outlink
done

echo ']'

echo "Listing source, object and dependency files"

echo -n "allsrc = " >> $outlist
echo $allsrc | sed 's,src/nls/cat-id-tbl.c,,' | sed -e 's/ / \\!/g'\
| tr '!' '\n' >> $outlist
echo >> $outlist
echo -n "allobj = " >> $outlist
echo $allobj | sed -e 's/ / \\!/g' | tr '!' '\n' >> $outlist
echo >> $outlist
echo -n "alldep = " >> $outlist
echo $alldep | sed -e 's/ / \\!/g' | tr '!' '\n' >> $outlist

echo >> $outlist
echo >> $outlink

# EOF $Id$
