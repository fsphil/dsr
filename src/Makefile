
PREFIX  := /usr/local
VERSION := $(shell git log -1 --pretty='%cd' --date=format:'%Y%m%d')-$(shell git describe --dirty --always)
CC      := $(CROSS_HOST)gcc
PKGCONF := pkg-config
CFLAGS  := -g -Wall -pthread -O3 $(EXTRA_CFLAGS) -DVERSION=\"$(VERSION)\"
LDFLAGS := -g -lm -pthread $(EXTRA_LDFLAGS)
OBJS    := dsrtx.o dsr.o bits.o conf.o src.o src_tone.o src_rawaudio.o rf.o rf_file.o
PKGS    := $(EXTRA_PKGS)

FFMPEG := $(shell $(PKGCONF) --exists libavcodec && echo ffmpeg)
ifeq ($(FFMPEG),ffmpeg)
	OBJS += src_ffmpeg.o
	PKGS += libavcodec libavformat libavdevice libswresample libavutil
	CFLAGS += -DHAVE_FFMPEG
endif

HACKRF := $(shell $(PKGCONF) --exists libhackrf && echo hackrf)
ifeq ($(HACKRF),hackrf)
	OBJS += rf_hackrf.o
	PKGS += libhackrf
	CFLAGS += -DHAVE_HACKRF
endif

SOAPYSDR := $(shell $(PKGCONF) --exists SoapySDR && echo SoapySDR)
ifeq ($(SOAPYSDR),SoapySDR)
	OBJS += rf_soapysdr.o
	PKGS += SoapySDR
	CFLAGS += -DHAVE_SOAPYSDR
endif

CFLAGS  += $(shell $(PKGCONF) --cflags $(PKGS))
LDFLAGS += $(shell $(PKGCONF) --libs $(PKGS))

all: dsrtx

dsrtx: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

install:
	cp -f dsrtx $(PREFIX)/bin/

clean:
	rm -f *.o *.d dsrtx

-include $(OBJS:.o=.d)

