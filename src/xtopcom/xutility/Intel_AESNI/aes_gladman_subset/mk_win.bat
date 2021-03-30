
@rem arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)
@rem this needs to be run in an x86 or x64 compiler environment
SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
set arch=%1
if "%arch%" == "86" goto OK
if "%arch%" == "64" goto OK
echo One arg in required. Must be 86 (for 32bit compiler) or 64 (for 64bit compiler)
goto :ERR



:OK

if "%arch%" == "86" (
	@rem I get a crash in modetest86.exe with visual studio 2010 in Gladman's code. 
	@rem I've reproduced the crash in his unmodified code as well.
	@rem Perhaps I just don't have the compiler options correct.  
	@rem In any case, the vs2010 workaround works on vs 2005, 2008, 2010
	set vs2010_workaround=yes
	if vs2010_workaround == no (
	set sz=32
	set asm=aes_x86_v2
	set defines= -D ASM_X86_V2
	set src=aeskey aesaux aes_modes modetest process_options my_getopt
	) else (
	set sz=32
	set asm=aes_x86_v1
	set defines= -D ASM_X86_V1C
	set options=
	set src=aesaux aes_modes modetest aeskey aestab process_options my_getopt
	)
)
if "%arch%" == "64" (
	set sz=64
	set asm=aes_amd64
	set defines= -D ASM_AMD64_C
	set src=aeskey aestab aesaux aes_modes modetest process_options my_getopt
)
pushd .
cd ..\intel_aes_lib
call mk_win_lib%arch%.bat
if ERRORLEVEL 1 goto :ERR
popd

set yasm=..\yasm\yasm-0.8.0-win%sz%.exe
set out=bin\modetest%arch%.exe
mkdir bin
del /q /s %out%

mkdir obj\x%arch%

%yasm% -g cv8 -f win%sz% asm\x%arch%\%asm%.asm -o obj\x%arch%\%asm%.obj
if ERRORLEVEL 1 goto :ERR

set lvl=/Ox /I..\intel_aes_lib\include %defines%
set objs=
for %%i in (%src%) do (
	set obj=obj\x%arch%\%%i.obj
	set objs=!objs! !obj!
	cl %lvl% /Zi /D "_CONSOLE" -Iinclude /Fo!obj! -c src\%%i.c
	if ERRORLEVEL 1 goto :ERR
)
set objs=!objs! obj\x%arch%\%asm%.obj
echo cl %lvl% /Zi /D "_CONSOLE" /Fe%out% /MT !objs! winmm.lib Kernel32.lib ..\intel_aes_lib\lib\x%arch%\intel_aes%arch%.lib
cl %lvl% /Zi /D "_CONSOLE" /Fe%out% /MT !objs! winmm.lib Kernel32.lib ..\intel_aes_lib\lib\x%arch%\intel_aes%arch%.lib
if ERRORLEVEL 1 goto :ERR
goto :EOF

:ERR
echo Got an error




