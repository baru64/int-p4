src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS += -D_REENTRANT  
LDFLAGS = -Wall -lpthread -lcurl

int_collector: $(obj)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -r *.o
