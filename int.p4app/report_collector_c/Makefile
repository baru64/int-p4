src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -D_REENTRANT -Wall -lpthread

int_collector: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm -r *.o
