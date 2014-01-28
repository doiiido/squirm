# $Id: Makefile,v 1.2 2005/08/19 07:31:06 chris Exp $

# The path to install squirm under
BINDIR=/usr/bin
LOGDIR=/var/log/squid
ETCDIR=/etc/squid
DESTDIR=

# The username that squid runs as (see cache_effective_user in squid.conf)
USER=proxy

# The group that squid runs as (see cache_effective_group in squid.conf)
GROUP=daemon

# The group that the root user belongs to
ROOT_GROUP = root

# The regex library (-lgnuregex is common on FreeBSD, none for some Linux'es)
EXTRALIBS=
# EXTRALIBS=-lgnuregex



# ---
# you should need to edit anything below here

TOP = .

OFILES = squirm.o main.o config.o log.o lists.o ip.o util.o
HFILES = squirm.h paths.h lists.h log.h ip.h util.h

CC=gcc


OPTIMISATION=-O3
BINARIES = squirm

CFLAGS = -O3 -Wall -funroll-loops -DBINDIR=\"$(BINDIR)\" -DLOGDIR=\"$(LOGDIR)\" -DETCDIR=\"$(ETCDIR)\"
#CFLAGS = -Wall -g -DPREFIX=\"$(PREFIX)\"
#CFLAGS = -Wall -g -DDEBUG

all:	$(BINARIES)

install:	all
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(ETCDIR)
	mkdir -p $(DESTDIR)$(LOGDIR)
	install -m 644 -c squirm.conf.dist squirm.patterns.dist $(DESTDIR)$(ETCDIR)
	install -m 755 -o root -g $(ROOT_GROUP) --strip squirm $(DESTDIR)$(BINDIR)

squirm.o:	squirm.c $(HFILES)
			$(CC) -c squirm.c 	$(CFLAGS)

main.o:		main.c $(HFILES)
			$(CC) -c main.c		$(CFLAGS)

config.o:	config.c $(HFILES)
			$(CC) -c config.c	$(CFLAGS)

log.o:		log.c $(HFILES)
			$(CC) -c log.c		$(CFLAGS)

lists.o:	lists.c $(HFILES)
			$(CC) -c lists.c	$(CFLAGS)

ip.o:		ip.c $(HFILES)
		    $(CC) -c ip.c            $(CFLAGS)

util.o:		util.c $(HFILES)
		    $(CC) -c util.c	     $(CFLAGS)

squirm:		$(OFILES) $(HFILES)
		$(CC) $(OPTIMISATION) -o squirm $(OFILES) $(EXTRALIBS) $(LDOPTS)


pure:		clean
			rm -f *~

clean:		
			rm -f squirm.o main.o config.o log.o lists.o \
			 ip.o util.o core squirm

distclean:		pure clean

dummy:


