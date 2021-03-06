call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
rc /nologo /l 0x0409 /fo resource.res resource.rc
cl /W4 /O2 pingkeepalive.c /link /subsystem:windows kernel32.lib user32.lib gdi32.lib shell32.lib iphlpapi.lib ws2_32.lib resource.res /out:pingkeepalive_x86.exe

pause
exit