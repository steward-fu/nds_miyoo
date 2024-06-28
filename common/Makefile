TARGET   = libcommon.so
LDFLAGS += -fPIC
LDFLAGS += -shared
SRC      = log.c cfg.c file.c

ifneq (ut,$(MOD))
    SRC += neon_memcpy.S
endif

.PHONY: all
all:
	$(CROSS)gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS)

.PHONY: clean
clean:
	rm -rf $(TARGET)
