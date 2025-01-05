TARGET   = libcommon.so
LDFLAGS += -fPIC
LDFLAGS += -shared
LDFLAGS += -ljson-c
SRC      = log.c \
           cfg.c \
           file.c \
           cfg.pb.c

ifneq (ut,$(MOD))
    SRC += memcpy.S
endif

.PHONY: all
all:
	$(CROSS)gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS)

.PHONY: proto
proto:
	sudo protoc --nanopb_out=-v:. cfg.proto

.PHONY: clean
clean:
	rm -rf $(TARGET)
