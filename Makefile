CC	= gcc
# prepare check for gcc 3.3, $(GCC_3.3) will either be 0 or 1
GCC_3.3 := $(shell expr `$(CC) -dumpversion` \>= 3.3)

GCC_4.2 := $(shell expr `$(CC) -dumpversion` \>= 4.2)

ETHERBOOT:=yes 
INCLUDE = -I$(TOPDIR)/grub -I$(TOPDIR)/include -I$(TOPDIR)/ -I./ -I$(TOPDIR)/fs/cdrom \
	-I$(TOPDIR)/fs/fatx -I$(TOPDIR)/lib/eeprom -I$(TOPDIR)/lib/crypt \
	-I$(TOPDIR)/drivers/video -I$(TOPDIR)/drivers/ide -I$(TOPDIR)/drivers/flash -I$(TOPDIR)/lib/misc \
	-I$(TOPDIR)/boot_xbe/ -I$(TOPDIR)/fs/grub -I$(TOPDIR)/lib/font -I$(TOPDIR)/lib/jpeg-6b \
	-I$(TOPDIR)/startuploader -I$(TOPDIR)/drivers/cpu
	
#These are intended to be non-overridable.
CROM_CFLAGS=$(INCLUDE)

#You can override these if you wish.
CFLAGS= -Os -march=pentium -m32 -pipe -fomit-frame-pointer -Wstrict-prototypes -DIPv4 -fpack-struct -Wreturn-type -ffreestanding

# add the option for gcc 3.3 only, again, non-overridable
ifeq ($(GCC_3.3), 1)
CROM_CFLAGS += -fno-zero-initialized-in-bss
endif

ifeq ($(GCC_4.2), 1)
CROM_CFLAGS += -fno-stack-protector -U_FORTIFY_SOURCE
endif

LD      = ld
OBJCOPY = objcopy

export CC

TOPDIR  := $(shell /bin/pwd)
SUBDIRS	= boot_rom fs drivers lib boot
#### Etherboot specific stuff
#ifeq ($(ETHERBOOT), yes)
ETH_SUBDIRS = etherboot
CROM_CFLAGS	+= -DETHERBOOT
ETH_INCLUDE = 	-I$(TOPDIR)/etherboot/include -I$(TOPDIR)/etherboot/arch/i386/include	
ETH_CFLAGS  = 	-O2 -march=pentium -m32 -Werror -Wreturn-type $(ETH_INCLUDE) -Wstrict-prototypes -fomit-frame-pointer -pipe -ffreestanding
ifeq ($(GCC_4.2), 1)
ETH_CFLAGS += -fno-stack-protector -U_FORTIFY_SOURCE
endif
#endif

LDFLAGS-ROM     = -s -S -T $(TOPDIR)/scripts/ldscript-crom.ld
LDFLAGS-XBEBOOT = -s -S -T $(TOPDIR)/scripts/xbeboot.ld
LDFLAGS-ROMBOOT = -s -S -T $(TOPDIR)/boot_rom/bootrom.ld
LDFLAGS-VMLBOOT = -s -S -T $(TOPDIR)/boot_vml/vml_start.ld
#ifeq ($(ETHERBOOT), yes)
LDFLAGS-ETHBOOT = -s -S -T $(TOPDIR)/boot_eth/eth_start.ld
#endif

# add the option for gcc 3.3 only
ifeq ($(GCC_3.3), 1)
ETH_CFLAGS += -fno-zero-initialized-in-bss
endif
#### End Etherboot specific stuff

OBJECTS-XBE = $(TOPDIR)/boot_xbe/xbeboot.o

OBJECTS-VML = $(TOPDIR)/boot_vml/vml_Startup.o
#ifeq ($(ETHERBOOT), yes)
OBJECTS-ETH = $(TOPDIR)/boot_eth/eth_Startup.o
#endif
                                             
OBJECTS-ROMBOOT = $(TOPDIR)/obj/2bBootStartup.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bPicResponseAction.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootStartBios.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/sha1.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/2bBootLibrary.o
OBJECTS-ROMBOOT += $(TOPDIR)/obj/misc.o
                                             
OBJECTS-CROM = $(TOPDIR)/obj/BootStartup.o
OBJECTS-CROM += $(TOPDIR)/obj/BootResetAction.o
OBJECTS-CROM += $(TOPDIR)/obj/i2cio.o
OBJECTS-CROM += $(TOPDIR)/obj/pci.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVgaInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/VideoInitialization.o
OBJECTS-CROM += $(TOPDIR)/obj/focus.o
OBJECTS-CROM += $(TOPDIR)/obj/xcalibur.o
OBJECTS-CROM += $(TOPDIR)/obj/conexant.o
OBJECTS-CROM += $(TOPDIR)/obj/BootIde.o
OBJECTS-CROM += $(TOPDIR)/obj/BootHddKey.o
OBJECTS-CROM += $(TOPDIR)/obj/rc4.o
OBJECTS-CROM += $(TOPDIR)/obj/sha1.o
OBJECTS-CROM += $(TOPDIR)/obj/BootVideoHelpers.o
OBJECTS-CROM += $(TOPDIR)/obj/vsprintf.o
#OBJECTS-CROM += $(TOPDIR)/obj/filtror.o
#OBJECTS-CROM += $(TOPDIR)/obj/BootStartBios.o
OBJECTS-CROM += $(TOPDIR)/obj/NiceMenu.o
OBJECTS-CROM += $(TOPDIR)/obj/FlashBios.o
OBJECTS-CROM += $(TOPDIR)/obj/ConfirmDialog.o
OBJECTS-CROM += $(TOPDIR)/obj/setup.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFilesystemIso9660.o
OBJECTS-CROM += $(TOPDIR)/obj/BootLibrary.o
OBJECTS-CROM += $(TOPDIR)/obj/cputools.o
OBJECTS-CROM += $(TOPDIR)/obj/microcode.o
OBJECTS-CROM += $(TOPDIR)/obj/ioapic.o
OBJECTS-CROM += $(TOPDIR)/obj/BootInterrupts.o
#OBJECTS-CROM += $(TOPDIR)/obj/fsys_reiserfs.o
#OBJECTS-CROM += $(TOPDIR)/obj/fsys_ext2fs.o
#OBJECTS-CROM += $(TOPDIR)/obj/char_io.o
#OBJECTS-CROM += $(TOPDIR)/obj/disk_io.o
ifeq ("","")
OBJECTS-CROM += $(TOPDIR)/obj/jdapimin.o
OBJECTS-CROM += $(TOPDIR)/obj/jdapistd.o
OBJECTS-CROM += $(TOPDIR)/obj/jdtrans.o
OBJECTS-CROM += $(TOPDIR)/obj/jdatasrc.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmaster.o
OBJECTS-CROM += $(TOPDIR)/obj/jdinput.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmarker.o
OBJECTS-CROM += $(TOPDIR)/obj/jdhuff.o
OBJECTS-CROM += $(TOPDIR)/obj/jdphuff.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmainct.o
OBJECTS-CROM += $(TOPDIR)/obj/jdcoefct.o
OBJECTS-CROM += $(TOPDIR)/obj/jdpostct.o
OBJECTS-CROM += $(TOPDIR)/obj/jddctmgr.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctfst.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctflt.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctint.o
OBJECTS-CROM += $(TOPDIR)/obj/jidctred.o
OBJECTS-CROM += $(TOPDIR)/obj/jdsample.o
OBJECTS-CROM += $(TOPDIR)/obj/jdcolor.o
OBJECTS-CROM += $(TOPDIR)/obj/jquant1.o
OBJECTS-CROM += $(TOPDIR)/obj/jquant2.o
OBJECTS-CROM += $(TOPDIR)/obj/jdmerge.o
OBJECTS-CROM += $(TOPDIR)/obj/jmemnobs.o
OBJECTS-CROM += $(TOPDIR)/obj/jmemmgr.o
OBJECTS-CROM += $(TOPDIR)/obj/jcomapi.o
OBJECTS-CROM += $(TOPDIR)/obj/jutils.o
OBJECTS-CROM += $(TOPDIR)/obj/jerror.o
endif
#AUDIO
#OBJECTS-CROM += $(TOPDIR)/obj/BootAudio.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlash.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFlashUi.o
OBJECTS-CROM += $(TOPDIR)/obj/BootEEPROM.o
OBJECTS-CROM += $(TOPDIR)/obj/BootParser.o
OBJECTS-CROM += $(TOPDIR)/obj/BootFATX.o
#USB
OBJECTS-CROM += $(TOPDIR)/obj/config.o 
OBJECTS-CROM += $(TOPDIR)/obj/hcd-pci.o
OBJECTS-CROM += $(TOPDIR)/obj/hcd.o
OBJECTS-CROM += $(TOPDIR)/obj/hub.o
OBJECTS-CROM += $(TOPDIR)/obj/message.o
OBJECTS-CROM += $(TOPDIR)/obj/ohci-hcd.o
OBJECTS-CROM += $(TOPDIR)/obj/buffer_simple.o
OBJECTS-CROM += $(TOPDIR)/obj/urb.o
OBJECTS-CROM += $(TOPDIR)/obj/usb-debug.o
OBJECTS-CROM += $(TOPDIR)/obj/usb.o
OBJECTS-CROM += $(TOPDIR)/obj/BootUSB.o
OBJECTS-CROM += $(TOPDIR)/obj/usbwrapper.o
OBJECTS-CROM += $(TOPDIR)/obj/linuxwrapper.o
OBJECTS-CROM += $(TOPDIR)/obj/xpad.o
OBJECTS-CROM += $(TOPDIR)/obj/xremote.o
OBJECTS-CROM += $(TOPDIR)/obj/usbkey.o
OBJECTS-CROM += $(TOPDIR)/obj/risefall.o
#ETHERBOOT
#ifeq ($(ETHERBOOT), yes)
#OBJECTS-CROM += $(TOPDIR)/obj/nfs.o
OBJECTS-CROM += $(TOPDIR)/obj/nic.o
#OBJECTS-CROM += $(TOPDIR)/obj/osloader.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox.o
OBJECTS-CROM += $(TOPDIR)/obj/forcedeth.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox_misc.o
OBJECTS-CROM += $(TOPDIR)/obj/xbox_pci.o
OBJECTS-CROM += $(TOPDIR)/obj/etherboot_config.o
#OBJECTS-CROM += $(TOPDIR)/obj/xbox_main.o
#OBJECTS-CROM += $(TOPDIR)/obj/elf.o
#endif


SUBDIRS += lwip
OBJECTS-LWIP = $(addprefix $(TOPDIR)/obj/,mem.o memp.o netif.o pbuf.o raw.o stats.o sys.o tcp.o tcp_in.o tcp_out.o udp.o dhcp.o icmp.o ip.o inet.o ip_addr.o ip_frag.o etharp.o ebd.o webserver.o)
#httpd.o http-pages.o)
OBJECTS-CROM += $(OBJECTS-LWIP)



RESOURCES = $(TOPDIR)/obj/backdrop.elf
#RESOURCES += $(TOPDIR)/obj/pcrombios.elf

export INCLUDE
export TOPDIR

#ifeq ($(ETHERBOOT), yes)
BOOT_ETH_DIR = boot_eth/ethboot
BOOT_ETH_SUBDIRS = ethsubdirs
#endif

all: clean
	@$(MAKE) -j16 --no-print-directory resources $(BOOT_ETH_SUBDIRS) cromsubdirs xbeboot xromwell.xbe vml_startup vmlboot $(BOOT_ETH_DIR) obj/image-crom.bin cromwell.bin imagecompress 256KBBinGen

#ifeq ($(ETHERBOOT), yes)
ethsubdirs: $(patsubst %, _dir_%, $(ETH_SUBDIRS))
$(patsubst %, _dir_%, $(ETH_SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(ETH_CFLAGS)" -C $(patsubst _dir_%, %, $@)
#endif

cromsubdirs: $(patsubst %, _dir_%, $(SUBDIRS))
$(patsubst %, _dir_%, $(SUBDIRS)) : dummy
	$(MAKE) CFLAGS="$(CFLAGS) $(CROM_CFLAGS)" -C $(patsubst _dir_%, %, $@)

dummy:

resources:
	# Background
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/backdrop.elf -T $(TOPDIR)/scripts/backdrop.ld -b binary $(TOPDIR)/pics/backdrop.jpg	
	${LD} -r --oformat elf32-i386 -o $(TOPDIR)/obj/pcrombios.elf -T $(TOPDIR)/scripts/pcrombios.ld -b binary $(TOPDIR)/pcbios/rombios.bin
	
clean:
	find . \( -name '*.[oas]' -o -name core -o -name '.*.flags' \) -type f -print \
		| grep -v lxdialog/ | xargs rm -f
	rm -f $(TOPDIR)/obj/*.gz 
	rm -f $(TOPDIR)/obj/*.bin 
	rm -f $(TOPDIR)/obj/*.elf
	rm -f $(TOPDIR)/image/*.bin 
	rm -f $(TOPDIR)/image/*.xbe 
	rm -f $(TOPDIR)/xbe/*.xbe $(TOPDIR)/xbe/*.bin
	rm -f $(TOPDIR)/xbe/*.elf
	rm -f $(TOPDIR)/image/*.bin
	rm -f $(TOPDIR)/bin/imagebld*
	rm -f $(TOPDIR)/boot_vml/disk/vmlboot
	rm -f boot_eth/ethboot
	mkdir -p $(TOPDIR)/xbe 
	mkdir -p $(TOPDIR)/image
	mkdir -p $(TOPDIR)/obj 
	mkdir -p $(TOPDIR)/bin

#rombios.bin:
#	${GCC295} -E $< > _rombios_.c
#	${BCC} -o rombios.s -c -D__i86__ -0 _rombios_.c
#	sed -e 's/^\.text//' -e 's/^\.data//' rombios.s > _rombios_.s
#	as86 _rombios_.s -b rombios.bin -u- -w- -g -0 -j -O -l rombios.txt
#	ls -l rombios.bin
#
#rombios.elf : rombios.bin
#	${LD} -r --oformat elf32-i386 -o $@ -T rombios.ld -b binary rombios.bin

 
obj/image-crom.bin: cromsubdirs resources
	${LD} -o obj/image-crom.elf ${OBJECTS-CROM} ${RESOURCES} ${LDFLAGS-ROM} -Map $(TOPDIR)/obj/image-crom.map
	${OBJCOPY} --output-target=binary --strip-all obj/image-crom.elf $@

vmlboot: vml_startup 
	${LD} -o $(TOPDIR)/obj/vmlboot.elf ${OBJECTS-VML} ${LDFLAGS-VMLBOOT}
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/vmlboot.elf $(TOPDIR)/boot_vml/disk/$@
	
vml_startup:
	$(CC) ${CFLAGS} -c -o ${OBJECTS-VML} boot_vml/vml_Startup.S

#ifeq ($(ETHERBOOT), yes)
boot_eth/ethboot: ethboot obj/image-crom.bin
	${LD} -o obj/ethboot.elf ${OBJECTS-ETH} -b binary obj/image-crom.bin ${LDFLAGS-ETHBOOT} -Map $(TOPDIR)/obj/ethboot.map
	${OBJCOPY} --output-target=binary --strip-all obj/ethboot.elf obj/ethboot.bin
	perl -I boot_eth boot_eth/mknbi.pl --output=$@ obj/ethboot.bin
	
ethboot:
	$(CC) ${CFLAGS} -c -o ${OBJECTS-ETH} boot_eth/eth_Startup.S
#endif

xromwell.xbe: xbeboot
	${LD} -o $(TOPDIR)/obj/xbeboot.elf ${OBJECTS-XBE} ${LDFLAGS-XBEBOOT}
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/xbeboot.elf $(TOPDIR)/xbe/flashBIOS.xbe
	
xbeboot:
	$(CC) ${CFLAGS} -c -o ${OBJECTS-XBE} boot_xbe/xbeboot.S

cromwell.bin: cromsubdirs
	${LD} -o $(TOPDIR)/obj/2lbimage.elf ${OBJECTS-ROMBOOT} ${LDFLAGS-ROMBOOT} -Map $(TOPDIR)/obj/2lbimage.map
	${OBJCOPY} --output-target=binary --strip-all $(TOPDIR)/obj/2lbimage.elf $(TOPDIR)/obj/2blimage.bin

# This is a local executable, so don't use a cross compiler...
bin/imagebld:
	gcc -Ilib/crypt -o bin/sha1.o -c lib/crypt/sha1.c
	gcc -Ilib/crypt -o bin/md5.o -c lib/crypt/md5.c
	gcc -Ilib/crypt -o bin/imagebld.o -c lib/imagebld/imagebld.c
	gcc -o bin/imagebld bin/imagebld.o bin/sha1.o bin/md5.o
	
imagecompress: obj/image-crom.bin bin/imagebld
	cp obj/image-crom.bin obj/c
	gzip -9 obj/c
	bin/imagebld -xbe xbe/flashBIOS.xbe obj/image-crom.bin
	bin/imagebld -vml boot_vml/disk/vmlboot obj/image-crom.bin f

256KBBinGen: imagecompress cromwell.bin
	bin/imagebld -rom obj/2blimage.bin obj/c.gz image/cromwell.bin
