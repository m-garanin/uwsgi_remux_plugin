import os

NAME='remuxer'
FFMPEG_LIBS=["libavdevice",                        
             "libavformat",                        
             "libavfilter",                        
             "libavcodec",                         
             "libswresample",                      
             "libswscale,"                         
             "libavutil" ]

CFLAGS = ['-Wall', '-g']
LDFLAGS = ['remuxlib.o']
LIBS = []

for el in FFMPEG_LIBS:
    CFLAGS += os.popen('pkg-config --cflags %s' % el).read().rstrip().split()
    LIBS += os.popen('pkg-config --libs %s' % el).read().rstrip().split()


GCC_LIST=['remuxer_plugin']
