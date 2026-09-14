CFLAGS += -std=gnu99 -Wpedantic -Wall -Wextra -Werror

.PHONY: all
all:

get_objs = $(addsuffix .o, $(basename $(1)))

LINUXTCP_FILES = examples/linuxTCP.c
LINUXTCP_OBJS = $(call get_objs,$(LINUXTCP_FILES))

linuxTCP: $(LINUXTCP_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# TODO: add proper dependencies on headers, not this bullshit
%.o: %.c $(shell find . -name "*.h")
	$(CC) -I include/ -o $@ -c $< $(CFLAGS)

clean:
	rm -f linuxTCP $(LINUXTCP_OBJS)
