AUTOMAKE_OPTIONS = foreign # Ignore need for README/AUTHORS... etc

AM_CFLAGS=-ggdb -Os -Wall

# Include buildinc target
include $(top_srcdir)/buildinc.mk

bin_PROGRAMS=fonulator
man_MANS = fonulator.1
fonulator_SOURCES=fonulator.c keys.c tokens.l status.c dsp.c error.c flash.c dlist.c
noinst_HEADERS =  config.h dsp.h error.h fonulator.h state.h status.h tokens.h tree.h ver.h dlist.h
fonulator_LDADD = @LIBOBJS@ @LIBFB@ /usr/lib/libnet.a /usr/lib/libpcap.a /usr/lib/libargtable2.a 
EXTRA_DIST = $(man_MANS)

