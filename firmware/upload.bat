@echo off
echo Waiting to upload...
:loop
sleep 0.1s
copy fw.sfe y:\ 2> nul:
if %errorlevel% == 0 goto done
goto loop
:done
echo Uploaded!