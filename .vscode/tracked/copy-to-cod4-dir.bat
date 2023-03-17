@echo off 
set path=%~1
cd ../build/bin/%path%

@echo on
copy t5xr.dll "%BO1_ROOT%\bin\"
copy t5xr.pdb "%BO1_ROOT%\bin\"