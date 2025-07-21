cc=cl
link=link

cflags=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "TELNET" /D "GAPING_SECURITY_HOLE" /YX /FD /c 
lflags=kernel32.lib user32.lib wsock32.lib winmm.lib /nologo /subsystem:console /incremental:yes /machine:I386 /out:nc.exe

all: tawqa.exe

getopt.obj: tawqa_getopt.cc
    $(cc) $(cflags) tawqa_getopt.c

doexec.obj: tawqa_doexec.cc
    $(cc) $(cflags) tawqa_doexec.cc

netcat.obj: tawqa.cc
    $(cc) $(cflags) tawqa.cc


nc.exe: getopt.obj doexec.obj netcat.obj
    $(link) getopt.obj doexec.obj netcat.obj $(lflags)