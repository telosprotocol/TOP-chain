\yasm\yasm-0.8.0-win32.exe -f win64 aes_amd64.asm

cl /Ox /D "NDEBUG" /D "_CONSOLE" modetest.c aes_modes.c aeskey.c aestab.c aesaux.c aes_amd64.obj
