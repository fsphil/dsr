
CC      := $(CROSS_HOST)gcc
CFLAGS  := -g -Wall -O3 $(EXTRA_CFLAGS)
LDFLAGS := -g -lm $(EXTRA_LDFLAGS)
OBJS    := main.o dsr.o bits.o

all: dsrtest

dsrtest: $(OBJS)
	$(CC) -o dsrtest $(OBJS) $(LDFLAGS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -MM $< -o $(@:.o=.d)

clean:
	rm -f *.o *.d dsrtest

-include $(OBJS:.o=.d)

