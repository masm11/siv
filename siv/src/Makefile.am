
SUBDIRS = images

bin_PROGRAMS = siv

siv_SOURCES =	\
  siv.c		\
  image.c	\
  image.h	\
  pixbuf.c	\
  pixbuf.h	\
  sivicon.c	\
  sivicon.h	\
  sivnail.c	\
  sivnail.h	\
  sivnailview.c	\
  sivnailview.h	\
  mainview.c	\
  mainview.h	\
  image-format.c \
  image-format.h

AM_CPPFLAGS = $(GSTUFF_CFLAGS) -I$(top_srcdir)/include -DPKGLIBEXECDIR=\"$(pkglibexecdir)\"
AM_CFLAGS = -Wall
AM_LDFLAGS = -rdynamic
LDADD = $(GSTUFF_LIBS)
