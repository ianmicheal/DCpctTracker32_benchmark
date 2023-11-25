#~ include ../../Makefile
EXTRAOPT=-Wall -Wextra -Wno-implicit-fallthrough -O3 -mpretend-cmove
EXTRAOPT+=-ffast-math -ffp-contract=fast -mfsrra -mfsca

TARGET = demo.elf
#~ KOS_ROMDISK_DIR=romdisk


TESTOBJS = transform.o \
	transform_p.o \
	ocindex.o \
	bench_pref.o \
	pct_func.o \
	bench_sq.o \
	bench_ta.o \
	pctracker.o \
	main.o

.PHONY: all clean run

test: $(TARGET)

all: $(TARGET) 

clean:
	rm -f $(TARGET) $(TESTOBJS) $(RELEASE_OBJS) $(DEBUG_OBJS) $(LIB_DEBUG_OBJS) $(TARGETLIB_RELEASE) $(TARGETLIB_DEBUG) $(TARGETLIB_LIBDEBUG) romdisk.img

run: $(TARGET)
	$(KOS_LOADER) $(TARGET) -n


%.o: %.c
	kos-cc $(CFLAGS) $(NODEBUGING) $(MATHOPT) $(EXTRAOPT) $(ALIGNS) -c $< -o $@

%.o: %.s
	kos-cc -c $< -o $@

%.o: %.S
	kos-cc -c -D _ASSEM $< -o $@

	
# Define KOS_ROMDISK_DIR in your Makefile if you want these two handy rules.
ifdef KOS_ROMDISK_DIR
romdisk.img:
	$(KOS_GENROMFS) -f romdisk.img -d $(KOS_ROMDISK_DIR) -v -x .svn

romdisk.o: romdisk.img
	$(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o
endif
######################################

$(TARGET): $(TESTOBJS) $(TARGETLIB_LIBDEBUG)
	$(KOS_CC) $(KOS_CFLAGS) $(MATHOPT) $(KOS_LDFLAGS) -o $(TARGET) $(KOS_START) \
		$(TESTOBJS) $(OBJEXTRA) $(KOS_LIBS) -lm

dist:
	rm -f $(OBJS)
	$(KOS_STRIP) $(TARGET)
