@echo off
rmdir /s /q tree
rmdir /s /q repos
python testcvs.py %1 %2 %3 %4
