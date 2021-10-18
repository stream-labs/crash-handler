@echo on
echo Running symbol script test
dir
cd %1
powershell.exe -ExecutionPolicy Bypass -Command ".\symbols.ps1 '%2' '%3' '%4' '%5' '%6' '%7' '%8'"
