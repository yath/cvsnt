rem DOS version.  Doesn't do any of the clever stuff.

rmdir /s /q %TARGET%\pdb
rmdir /s /q %TARGET%\inc
rmdir /s /q %TARGET%\lib
rmdir /s /q %TARGET%\protocols
rmdir /s /q %TARGET%\triggers
rmdir /s /q %TARGET%\xdiff
md %TARGET%\inc
md %TARGET%\inc\win32
md %TARGET%\inc\unix
md %TARGET%\inc\lib
md %TARGET%\inc\diff
md %TARGET%\inc\apiloader
md %TARGET%\lib
md %TARGET%\pdb
md %TARGET%\pdb\triggers
md %TARGET%\pdb\protocols
md %TARGET%\pdb\xdiff
md %TARGET%\triggers
md %TARGET%\protocols
md %TARGET%\xdiff
pushd %BASE%
copy %TYPE%\*.exe %TARGET%
copy %TYPE%\*.dll %TARGET%
copy %TYPE%\*.cpl %TARGET%
copy %TYPE%\triggers\*.dll %TARGET%\triggers
copy %TYPE%\protocols\*.dll %TARGET%\protocols
copy %TYPE%\xdiff\*.dll %TARGET%\xdiff
copy %TYPE%\setuid.dll ${SYSTEMROOT}\system32
copy %TYPE%\setuid.dll ${SYSTEMROOT}\system32\setuid2.dll
copy %TYPE%\apiloader.lib %TARGET%\lib
copy %TYPE%\cvsapi.lib %TARGET%\lib
copy %TYPE%\cvstools.lib %TARGET%\lib
copy triggers\sql\*.sql %TARGET%\sql
copy relnotes.rtf %TARGET%
copy ca.pem %TARGET%
copy extnt.ini %TARGET%
copy protocol_map.ini %TARGET%
copy COPYING %TARGET%
copy doc\*.chm %TARGET%
copy version*.h %TARGET%\inc
copy build.h %TARGET%\inc
copy cvsapi\cvsapi.h %TARGET%\inc
copy cvsapi\win32\config.h %TARGET%\inc\win32
copy cvstools\cvstools.h %TARGET%\inc
popd
