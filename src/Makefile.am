LIBEXWORD_LIBRARY_VERSION=1:0:0
lib_LTLIBRARIES = libexword.la
bin_PROGRAMS = exword
libexword_la_SOURCES =	exword.c \
			exword.h \
			obex.c   \
			obex.h \
			databuffer.c \
			databuffer.h \
			list.h

include_HEADERS = exword.h

libexword_la_CFLAGS = \
        $(USB_CFLAGS)        \
        $(WARN_CFLAGS)          \
        $(AM_CFLAGS)

libexword_la_LDFLAGS = -version-info $(LIBEXWORD_LIBRARY_VERSION)
libexword_la_LIBADD = $(USB_LIBS)

exword_SOURCES = main.c
exword_CFLAGS = \
        $(WARN_CFLAGS)          \
        $(AM_CFLAGS)

exword_LDFLAGS = $(AM_LDFLAGS)
exword_LDADD = $(READLINE_LIBS) $(POPT_LIBS) libexword.la
