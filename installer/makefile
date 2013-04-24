!Ifndef VERSION

# The farting around with bash is because you can't do anything like `...` in
# cmd.exe.

all: cvsnt
release: cvsnt

cvsnt:
	@bash -c "cmd.exe /c $(MAKE) /NOLOGO $(MAKEFLAGS) VERSION=`cvs version -q` LICENSE=gpl.rtf $@"

combined: suite

suite:	suite-server suite-client

suite-server:
	@bash -c "cmd.exe /c $(MAKE) /NOLOGO $(MAKEFLAGS) VERSION=`cvs version -q` LICENSE=combined-license.rtf $@"

suite-client:
	@bash -c "cmd.exe /c $(MAKE) /NOLOGO $(MAKEFLAGS) VERSION=`cvs version -q` LICENSE=combined-license.rtf $@"

clean:
	del *.wixobj *.msi *.cab

!Else

CVSNT_OBJS=cvsnt-installer.wixobj ui.wixobj cvsnt.wixobj server.wixobj workspaceviewer.wixobj
SUITE_CLIENT_OBJS=cvs-suite-installer.wixobj ui.wixobj cvsnt.wixobj wincvs.wixobj winmerge.wixobj tortoisecvs.wixobj releasemanager.wixobj workspacemanager.wixobj ebook.wixobj scci.wixobj
SUITE_SERVER_OBJS=cvs-suite-server-installer.wixobj ui.wixobj cvsnt.wixobj server.wixobj triggers.wixobj unison.wixobj wait4cvs.wixobj

CANDLEFLAGS=-sw1044 -dCVSNT_VERSION=$(VERSION) -dLICENSE=$(LICENSE) -dCVSNT_BASE=d:\cvsbin -dCVSNT64_BASE=d:\cvsbin64 -dWINCVS_BASE=d:\march-hare\wincvs -dWINMERGE_BASE=d:\march-hare\winmerge -dTORTOISECVS_BASE=d:\march-hare\TortoiseCVS -dMARCH_HARE=d:\march-hare\binRelease

CVSNT_TARGET=cvsnt-$(VERSION).msi
SUITE_CLIENT_TARGET=cvs-suite-$(VERSION).msi
SUITE_SERVER_TARGET=cvs-suite-server-$(VERSION).msi

all: cvsnt

cvsnt: $(CVSNT_TARGET)

suite-client: $(SUITE_CLIENT_TARGET)

suite-server: $(SUITE_SERVER_TARGET)

clean:
	del *.wixobj *.msi *.cab

$(CVSNT_TARGET): $(CVSNT_OBJS)
	light -o $@ $(CVSNT_OBJS)
	signcode -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $(CVSNT_TARGET)

$(SUITE_CLIENT_TARGET): $(SUITE_CLIENT_OBJS)
	light -o $@ $(SUITE_CLIENT_OBJS)
	signcode -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $(SUITE_CLIENT_TARGET)

$(SUITE_SERVER_TARGET): $(SUITE_SERVER_OBJS)
	light -o $@ $(SUITE_SERVER_OBJS)
	signcode -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $(SUITE_SERVER_TARGET)

.wxs.wixobj:
	candle $(CANDLEFLAGS) $<

.SUFFIXES: .wxs

!Endif
