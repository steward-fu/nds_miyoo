TARGET   = libasound.so.2
LDFLAGS += -fPIC
LDFLAGS += -shared
SRC      = snd.c

.PHONY: all
all:
	$(CROSS)gcc $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS) $(MOREFLAGS)

.PHONY: clean
clean:
	rm -rf $(TARGET)
