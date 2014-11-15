###########################################################
# Radiosity Makefile

CC = g++
CFLAGS = -Wall -ggdb
INCLUDE = -I/lusr/X11/include -I/lusr/include
LIBDIR = -L/lusr/X11/lib -L/lusr/lib
# Libraries that use native graphics hardware --
# appropriate for Linux machines in Taylor basement
LIBS = -lglut -lGLU -lGL -lpthread -lm

###########################################################
# Options if compiling on Mac
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
CC = clang++
CFLAGS = -Wall -g
LIBDIR = -L/lusr/X11/lib
LIBS = -framework OpenGL -framework GLUT -framework Cocoa
endif

###########################################################
# Uncomment the following line if you are using Mesa
#LIBS = -lglut -lMesaGLU -lMesaGL -lm

rad: main.cpp
	${CC} ${CFLAGS} ${INCLUDE}  ${LIBS}  -o rad ${LIBDIR} main.cpp

clean:
	rm -f rad *.o