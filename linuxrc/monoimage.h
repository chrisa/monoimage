struct monoimage_header {
  uint8_t  magic[2];
  uint16_t version;
  uint32_t kernel_offset;
  uint32_t ramdisk_offset;
  uint32_t rootfs_offset;
};

