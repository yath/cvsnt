This note is written in order to outline what is needed in order to build the 
Innosetup version of the cvsnt installer.
The reason this is done at all is that CVSNT has moved to the MSI installer
engine as prescribed by Microsoft and requested by corporate users (or their
IT departments maybe).
Since this leaves the realm of Open Source in favour of MS solutions the
continued maintenance of the Innosetup installer for CVSNT is a task for
some interested parties. Me included.

So down to the facts of the matter:

How to use the cvsnt.iss script?

You need to have the following:

1) A folder named cvsbin located at the root of the cvsnt module
   It will be accessed as ..\..\cvsbin from the location of this file.
   
2) Into this folder you will need to deposit all the files downloaded from
   the development wiki page as the "binaries without installer" zipfile.
   The URL to the download page is currently:
   http://www.cvsnt.org/wiki/Download
   
   You also need these 2 files here, which are not part of the binary zip:
   relnotes.rtf
   cvs.chm
   
   
3) A folder cvsbin\sysfiles where the system files that are not part of the
   cvsnt project but are needed by the cvsnt programs reagrdless. These are
   typically the Windows system dll:s used by Visual Studio DOTNET.
   
   A list of the files at the moment is:
   charset.dll
   dbghelp.dll
   iconv.dll
   libeay32_vc71.dll
   mfc71u.dll
   msvcp71.dll
   msvcr71.dll
   secur32_nt4.dll
   ssleay32_vc71.dll

4) A folder cvsbin\Keep where the executable SetAcl.exe is kept

5) InnoSetup version 4.2.7 and preferably ISTool version 4.2.7

Once these are in place you can build the installer by executing the cvsnt.iss
script file with InnoSetup.
The output will be placed in cvsbin\setup

2005-01-21
Bo Berglund
