src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS += -D_REENTRANT  
LDFLAGS = -Wall -lpthread -lcurl

all: int_collector clean

int_collector: $(obj)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm $(obj)
