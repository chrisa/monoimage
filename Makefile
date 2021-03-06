# master makefile for monoimage bits
#
# do a make world to build these things:
# * linuxrc
# * monoboot
# * a rootfs
# * an initrd
# * an image
#

all: 
	$(MAKE) -C libcli
	$(MAKE) -C linuxrc
	$(MAKE) -C initrd
	$(MAKE) -C monoboot
	$(MAKE) kexec-tools
	$(MAKE) -C gpio
	$(MAKE) monoimage

libcli/libcli.a: 
	$(MAKE) -C libcli

initrd: initrd/initrd.img

initrd/initrd.img: linuxrc/linuxrc
	$(MAKE) -C initrd

monoboot/monoboot: ../libcli/libcli.a
	$(MAKE) -C monoboot

linuxrc/linuxrc:
	$(MAKE) -C linuxrc

rootfs.img:
	$(MAKE) -C rootfs

clean:
	$(MAKE) -C linuxrc clean
	$(MAKE) -C monoboot clean
	$(MAKE) -C initrd clean
	$(MAKE) -C kexec-tools clean
	$(MAKE) -C gpio clean
	$(MAKE) -C libcli clean

install:
	$(MAKE) -C monoboot install
	$(MAKE) -C kexec-tools install

release:
	mkdir monoimage-tools-$(VERSION)
	tar cTf MANIFEST - | (cd monoimage-tools-$(VERSION) && tar xvf -)
	tar zcvf monoimage-tools-$(VERSION).tar.gz monoimage-tools-$(VERSION)

monoimage: initrd/initrd.img bzImage rootfs.img
	perl utils/makeimage.pl -d loop -f cramfs -k bzImage -i initrd/initrd.img -r rootfs.img > monoimage

kexec-tools: kexec-tools/config.log
	$(MAKE) -C kexec-tools

kexec-tools/config.log:
	(cd kexec-tools && ./configure --with-libext2fs)
