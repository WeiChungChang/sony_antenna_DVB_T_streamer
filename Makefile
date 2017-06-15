
TOPDIR=..
TOPINC=$(TOPDIR)/include
TOPLIB=$(TOPDIR)/lib

include $(TOPDIR)/mkconfig.action

# use pkg-config for getting CFLAGS and LDLIBS
FFMPEG_LIBS=    libavformat                        \
                libavfilter                        \
                libavcodec                         \
                libswresample                      \
                libswscale                         \
                libavutil                          \
                libavdevice                        \

CFLAGS += -Wall -g
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs $(FFMPEG_LIBS)) $(LDLIBS)
CFLAGS += -I$(TOPINC) -I$(TOPINC)/libew100 -DSONY_EXAMPLE_DEVIO_SPI -DS900_SDIO


EW100_SCAN = tuner_scanDVBT.o tuner_scanhandler.o tuner_common.o tuner_spibus.o tuner_scanprograms.o tuner_packet.o

EW100_SCANISDB = tuner_scanISDBT.o tuner_scanhandler.o tuner_common.o tuner_spibus.o tuner_scanprograms.o tuner_packet.o

EW100_DVBT = tuner_dvbt.o tuner_packet.o tuner_spibus.o tuner_common.o

EW100_ISDBT = tuner_isdbt.o tuner_packet.o tuner_spibus.o tuner_common.o

#FFMpeg & Tuner integration test
EXAMPLE = tuner_avio_impl tuner_packet tuner_common ringbuf allocation ffimpl generic_io mmdbg threadqueue
OBJS=$(addsuffix .o,$(EXAMPLE))

FFEXAMPLE = ffexample
FFOBJS=$(addsuffix .o,$(FFEXAMPLE))

LDFLAGS += -L. -L$(TOPLIB) -lew100 -ltsmgr
LIB_FILE += $(TOPDIR)/lib/libew100.a $(TOPDIR)/lib/libtsmgr.a
LIB_FILE_DIR = src


all: $(TOPDIR)/lib/libew100.a $(TOPDIR)/bin/ew100_scan $(TOPDIR)/bin/ew100_dvbt $(TOPDIR)/bin/ew100_scanisdbt $(TOPDIR)/bin/ew100_isdbt $(TOPDIR)/bin/FFtest $(TOPDIR)/bin/FFexample

$(TOPDIR)/bin/ew100_dvbt:$(EW100_DVBT)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TOPDIR)/bin/ew100_scanisdbt:$(EW100_SCANISDB)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TOPDIR)/bin/ew100_scan:$(EW100_SCAN)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TOPDIR)/bin/ew100_isdbt:$(EW100_ISDBT)
	$(CXX) -o $@ $^ $(LDFLAGS)

$(TOPDIR)/lib/libew100.a:
	$(MAKE) -C src all

#FFMpeg & Tuner integration test
$(TOPDIR)/bin/FFtest:$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(TOPDIR)/bin/FFexample:$(FFOBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf $(TOPDIR)/bin/ew100*
	rm -f *.o
	$(MAKE) -C src clean
