# master makefile for monoimage bits
#
# do a make world to build these things:
# * linuxrc
# * monoboot
# * a rootfs
# * an initrd
# * an image
#
world:
	$(MAKE) -C linuxrc
	$(MAKE) -C monoboot
	$(MAKE) -C initrd
	$(MAKE) -C rootfs
	perl utils/makeimage.pl -k vmlinuz -i initrd.img -r rootfs.img > monoimage

clean:
	$(MAKE) -C linuxrc clean
	$(MAKE) -C monoboot clean
	-rm initrd.img rootfs.img monoimage


