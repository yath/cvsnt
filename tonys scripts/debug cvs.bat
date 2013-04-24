@echo off
set TYPE=windebug

rem set TARGET=d:\cvsbin
rem set BASE=d:\cvssrc\cvsnt-2501
rem call %BASE%/"tonys scripts"/copy_common.cmd  >nul:

set BASE=/cygdrive/d/cvssrc/cvsnt-2501
set TARGET=/cygdrive/d/cvsbin
bash %BASE%/"tonys scripts"/copy_common.sh

