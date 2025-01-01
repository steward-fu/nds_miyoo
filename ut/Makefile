TARGET   = ut
CFLAGS  += -ggdb
CFLAGS  += -Isrc
CFLAGS  += -Iextras/memory/src
CFLAGS  += -Iextras/fixture/src
LDFLAGS += -Wl,--no-undefined
LDFLAGS += libasound.so.2
LDFLAGS += libSDL2-2.0.so.0
LDFLAGS += libcommon.so
LDFLAGS += libdetour.so
SRC      = main.c \
           src/unity.c \
           extras/memory/src/unity_memory.c \
           extras/fixture/src/unity_fixture.c

.PHONY: mini a30
mini a30:
	rm -rf $(TARGET)

.PHONY: ut
ut: clean
	$(CROSS)gcc $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(MOREFLAGS)
	LD_LIBRARY_PATH=../drastic/libs ./$(TARGET) -v
	#LD_LIBRARY_PATH=../drastic/libs gdb --args ./$(TARGET) -v

.PHONY: clean
clean:
	rm -rf $(TARGET) miyoo_drastic_log.txt
