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
	$(MAKE) -C linuxrc
	$(MAKE) -C initrd
	$(MAKE) -C monoboot
	$(MAKE) -C kexec-tools
	$(MAKE) -C gpio

initrd: linuxrc/linuxrc
	$(MAKE) -C initrd

monoboot/monoboor: 
	$(MAKE) -C monoboot

linuxrc/linuxrc:
	$(MAKE) -C linuxrc

clean:
	$(MAKE) -C linuxrc clean
	$(MAKE) -C monoboot clean
	$(MAKE) -C initrd clean
	$(MAKE) -C kexec-tools clean
	$(MAKE) -C gpio clean

install:
	$(MAKE) -C monoboot install
	$(MAKE) -C kexec-tools install

release:
	mkdir monoimage-tools-$(VERSION)
	tar cTf MANIFEST - | (cd monoimage-tools-$(VERSION) && tar xvf -)
	tar zcvf monoimage-tools-$(VERSION).tar.gz monoimage-tools-$(VERSION)
