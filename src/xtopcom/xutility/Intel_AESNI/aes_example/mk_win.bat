
@rem arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)
set arch=%1
if "%arch%" == "86" goto OK
if "%arch%" == "64" goto OK
echo One arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)
goto :ERR

:OK
if "%arch%" == "86" set sz=32
if "%arch%" == "64" set sz=64
pushd .
cd ..\intel_aes_lib
call mk_win_lib%arch%.bat
if ERRORLEVEL 1 goto :ERR
popd

set yasm=..\yasm\yasm-0.8.0-win%sz%.exe
set out=bin\aes_example%arch%.exe
mkdir bin
del /q /s %out%

set lvl= 
set lvl= -Ox
set src=aes_example my_getopt

mkdir obj\x%arch%\


for %%i in (%src%) do (
	cl /Zi -c %lvl% /Foobj\x%arch%\%%i.obj src\%%i.c -Isrc -I..\intel_aes_lib\include 
	if ERRORLEVEL 1 goto :ERR
)
cl /Zi %lvl% /Fe%out% obj\x%arch%\aes_example.obj obj\x%arch%\my_getopt.obj ..\intel_aes_lib\lib\x%arch%\intel_aes%arch%.lib winmm.lib
if ERRORLEVEL 1 goto :ERR



goto :EOF

:ERR
echo Got an error
