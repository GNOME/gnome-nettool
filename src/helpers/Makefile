CC = gcc
CFLAGS = -g -O2 -Wall 
LIBS =
# On Solaris it should be
# LIBS = -lsocket -lnsl

CLEANFILES = core *.o *.out 

OBJS = gnome-ping-helper.o 

PROGS =	gnome-ping-helper

all:	${PROGS}

gnome-ping-helper:	${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

clean:
	rm -f ${PROGS} ${CLEANFILES}
