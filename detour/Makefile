TARGET   = libdetour.so
LDFLAGS += -fPIC
LDFLAGS += -shared
SRC      = hook.c \
           drastic.c

ifneq (ut,$(MOD))
    SRC += drastic.S
endif

.PHONY: all
all:
	$(CROSS)gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS)

.PHONY: clean
clean:
	rm -rf $(TARGET)
