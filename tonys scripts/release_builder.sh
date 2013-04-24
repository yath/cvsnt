#!/bin/sh
BRANCH=CVSNT_2_0_x
PATH=/cygdrive/d/cvsbin:${PATH}:/cygdrive/c/"program files"/winzip:/cygdrive/c/"Program Files"/PuTTY:/cygdrive/c/"Program Files"/"HTML Help Workshop":/cygdrive/d/cvsbin/keep
rm -rf cvsnt
INCLUDE="/cvsdeps/openssl/include;/cvsdeps/iconv/include"
LIB="/cvsdeps/openssl/lib;/cvsdeps/iconv/lib"
TOP="`pwd`"
cvs -d :pserver:tmh@cvs.cvsnt.org:/usr/local/cvs co -r $BRANCH cvsnt
devenv.com /rebuild release cvsnt/cvsnt.sln
BUILD=`cvsnt/winrel/cvs.exe ver -q`
echo Build=$BUILD
echo $BUILD >cvsnt/doc/version.inc
TAG=CVSNT_`echo $BUILD | sed 's/\./_/g'`
echo Tag=$TAG
cd cvsnt
perl.exe /cvsdeps/cvs2cl/cvs2cl.pl -F $BRANCH --window 3600
cvs -q commit -m "Build $BUILD"
winrel/cvs.exe tag -F $TAG
cd "$TOP"
rm -f ../*
if [ -f ../cvsapi.dll ]; then
  echo Cannot build due to locked files!
  read
  exit 1
fi
export TARGET=/cygdrive/d/cvsbin
export BASE="`pwd`/cvsnt"
export TYPE=winrel
sh cvsnt/tonys\ scripts/copy_common.sh
cd cvsnt/doc
build.bat cvs
build-pdk.bat $BUILD
cd "$TOP"
cp cvsnt/doc/*.chm ..
cp cvsnt/WinRel/*.pdb ../pdb
cp cvsnt/WinRel/protocols/*.pdb ../pdb/protocols
cp cvsnt/WinRel/triggers/*.pdb ../pdb/triggers
cp cvsnt/WinRel/xdiff/*.pdb ../pdb/xdiff
cd cvsnt/installer
nmake
mv cvsnt-${BUILD}.msi ../../..
makeu3.bat ${BUILD}
mv cvsagent-${BUILD}.u3p ../../..
cd ../../..
wzzip -P cvsnt-${BUILD}-bin.zip *.dll *.exe *.cpl *.ini ca.pem infolib.h COPYING triggers protocols xdiff
wzzip -pr cvsnt-${BUILD}-pdb.zip pdb
wzzip -Pr cvsnt-${BUILD}-dev.zip cvsnt-pdk.chm inc lib
cp keep/*.* .
echo ${BUILD} >release
pscp -i d:/tony.ppk *.zip cvsnt-${BUILD}.msi *.u3p cvs.chm release tmh@sisko:/home/tmh/cvs
