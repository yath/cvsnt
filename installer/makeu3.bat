@echo off
cd u3
copy \cvsbin\cvsagent.exe host
wzzip -rp ../cvsagent-%1.u3p *
cd ..
