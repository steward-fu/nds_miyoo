TARGET   = libcommon.so
LDFLAGS += -fPIC
LDFLAGS += -shared
LDFLAGS += -ljson-c
LDFLAGS += -lprotobuf-nanopb
SRC      = log.c cfg.c cfg.pb.c

ifneq (ut,$(MOD))
    SRC += neon_memcpy.S
endif

.PHONY: all
all:
	$(CROSS)gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS)

proto:
	sudo protoc --nanopb_out=. cfg.proto

.PHONY: clean
clean:
	rm -rf $(TARGET)
