
CC      := gcc
PKGCONF := pkg-config
CFLAGS  := -g -Wall -O3 -pthread
LDFLAGS := -g -lm -pthread
OBJS    := dsrtx.o dsr.o bits.o conf.o src.o src_tone.o src_rawaudio.o rf.o rf_file.o rf_hackrf.o
PKGS    := libhackrf

CFLAGS  += $(shell $(PKGCONF) --cflags $(PKGS))
LDFLAGS += $(shell $(PKGCONF) --libs $(PKGS))

all: dsrtx

dsrtx: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

clean:
	rm -f *.o *.d dsrtx

-include $(OBJS:.o=.d)

