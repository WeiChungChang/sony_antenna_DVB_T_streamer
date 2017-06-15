
TOPDIR=../..
TOPINC=$(TOPDIR)/include
TOPLIB=$(TOPDIR)/lib

I2CLINUXINC=devio/i2c/linux_i2cdev
I2CINC=devio/i2c
SPILINUXINC=devio/spi/linux_spidev
SPIINC=devio/spi
SDIOCSINC=devio/sdio/linux_csdio
SDIOINC=devio/sdio
SDIOCXD288XINC=devio/sdio/linux_cxd288x_sdio
DEVIOINC=devio
DVBINC=dvb
ISDBINC=isdb
LIBINC=lib
TUNERMODULEINC=tunermodule

LIBEW100INC=$(TOPINC)/libew100

include $(TOPDIR)/mkconfig.action

CFLAGS += -I. -I$(I2CLINUXINC) -I$(I2CINC) -I$(SPILINUXINC) -I$(SPIINC) \
	-I$(SDIOCSINC) -I$(SDIOINC) -I$(SDIOCXD288XINC) -I$(DEVIOINC) -I$(DVBINC) -I$(ISDBINC) -I$(LIBINC) -I$(TUNERMODULEINC)

PROJECT_NAME = libew100
LIB_DIR = 
LIB_FILE =
LIB_FILE = devio/i2c/linux_i2cdev/sony_i2c_i2cdev.o \
           devio/i2c/sony_i2c_log.o \
           devio/i2c/sony_i2c.o \
           devio/spi/linux_spidev/sony_spi_spidev.o \
           devio/sdio/linux_cxd288x_sdio/sony_sdio_cxd288x_sdio.o \
           devio/sony_devio_i2c.o \
           devio/sony_regio.o \
           devio/sony_regio_log.o \
           devio/sony_devio_spi.o \
           devio/sony_devio_sdio.o \
           dvb/sony_tunerdemod_dvbt.o \
           dvb/sony_tunerdemod_dvbt2_monitor.o \
           dvb/sony_integ_dvbt2.o \
           dvb/sony_integ_dvbt_t2.o \
           dvb/sony_tunerdemod_dvbt2.o \
           dvb/sony_integ_dvbt.o \
           dvb/sony_tunerdemod_dvbt_monitor.o \
           isdb/sony_tunerdemod_isdbtsb.o \
           isdb/sony_tunerdemod_isdbt.o \
           isdb/sony_tunerdemod_isdbtmm.o \
           isdb/sony_integ_isdbtmm.o \
           isdb/sony_tunerdemod_isdbt_monitor.o \
           isdb/sony_integ_isdbt.o \
           isdb/sony_integ_isdbtsb.o \
           lib/sony_math.o \
           tunermodule/sony_tunermodule.o \
           sony_common.o \
           sony_integ.o \
           sony_stopwatch_port.o \
           sony_tunerdemod.o \
           sony_tunerdemod_monitor.o

TARGET_LIB = $(TOPLIB)/$(PROJECT_NAME).a

all: $(TARGET_LIB)

$(TARGET_LIB): $(LIB_FILE)
	mkdir -p $(LIBEW100INC)
	$(LD) -o $@ -r $(LIB_FILE)
	cp -f *.h $(LIBEW100INC)/
	cp -f $(I2CLINUXINC)/*.h $(LIBEW100INC)/
	cp -f $(I2CINC)/*.h $(LIBEW100INC)/
	cp -f $(SPILINUXINC)/*.h $(LIBEW100INC)/
	cp -f $(SPIINC)/*.h $(LIBEW100INC)/
	cp -f $(SDIOCSINC)/*.h $(LIBEW100INC)/
	cp -f $(SDIOINC)/*.h $(LIBEW100INC)/
	cp -f $(SDIOCXD288XINC)/*.h $(LIBEW100INC)/
	cp -f $(DEVIOINC)/*.h $(LIBEW100INC)/
	cp -f $(DVBINC)/*.h $(LIBEW100INC)/
	cp -f $(ISDBINC)/*.h $(LIBEW100INC)/
	cp -f $(LIBINC)/*.h $(LIBEW100INC)/
	cp -f $(TUNERMODULEINC)/*.h $(LIBEW100INC)/

clean:
	rm -rf $(LIB_FILE)
	rm -rf $(TARGET_LIB)
	rm -rf $(LIBEW100INC)

