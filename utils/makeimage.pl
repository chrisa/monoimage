#!/usr/bin/perl
use strict;
use Getopt::Std;

use constant BOOTIMAGE_HEADER_SIZE => 16;

use constant MI_MAGIC => "MI";
use constant MI_HEADER_FORMAT => "SCCIII";

use constant MI_RUNFROM_LOOP  => 0;
use constant MI_RUNFROM_CLOOP => 1;
use constant MI_RUNFROM_TMPFS => 2;
use constant MI_RUNFROM_NFS   => 3;

use constant MI_FSTYPE_EXT2     => 0;
use constant MI_FSTYPE_SQUASHFS => 1;
use constant MI_FSTYPE_TAR      => 2;
use constant MI_FSTYPE_TARGZ    => 3;
use constant MI_FSTYPE_CPIO     => 4;
use constant MI_FSTYPE_CPIOGZ   => 5;

my %opt;
getopts('f:r:k:i:r:', \%opt);

unless ( defined $opt{k} && 
	 defined $opt{r} && 
	 defined $opt{i}) {
    
    print STDERR "usage: -f root-format -r run-from -k kernel -i initrd -r rootfs\n";
    exit 1;
}

my $format;
if ($opt{f} eq 'ext2') {
    $format = MI_FSTYPE_EXT2;
}
if ($opt{f} eq 'squashfs') {
    $format = MI_FSTYPE_SQUASHFS;
}
if ($opt{f} eq 'tar') {
    $format = MI_FSTYPE_TAR;
}
if ($opt{f} eq 'targz') {
    $format = MI_FSTYPE_TARGZ;
}
if ($opt{f} eq 'cpio') {
    $format = MI_FSTYPE_CPIO;
}
if ($opt{f} eq 'cpiogz') {
    $format = MI_FSTYPE_CPIOGZ;
}

unless (defined $format) {
    print STDERR "defaulting to ext2 root fs\n";
    $format = MI_FSTYPE_EXT2;
}

my $runfrom;
if ($opt{r} eq 'loop') {
    $runfrom = MI_RUNFROM_LOOP;
}
if ($opt{r} eq 'cloop') {
    $runfrom = MI_RUNFROM_CLOOP;
}
if ($opt{r} eq 'tmpfs') {
    $runfrom = MI_RUNFROM_TMPFS;
}
if ($opt{r} eq 'nfs') {
    $runfrom = MI_RUNFROM_NFS;
}

unless (defined $runfrom) {
    print STDERR "defaulting to run from loop\n";
    $runfrom = MI_RUNFROM_LOOP;
}


my ($kernel, $ramdisk, $rootfs) = ($opt{k}, $opt{i}, $opt{r});

my $kernel_size  = (stat $kernel)[7];
my $ramdisk_size = (stat $ramdisk)[7];
my $rootfs_size  = (stat $rootfs)[7];

open KERNEL,  $kernel  or die "kernel: $!";
open RAMDISK, $ramdisk or die "ramdisk: $!";
open ROOTFS,  $rootfs  or die "rootfs: $!";

print MI_MAGIC;
print pack(MI_HEADER_FORMAT, 1,
	   $format,
	   $runfrom,
	   BOOTIMAGE_HEADER_SIZE,
	   (BOOTIMAGE_HEADER_SIZE + $kernel_size),
	   (BOOTIMAGE_HEADER_SIZE + $kernel_size + $ramdisk_size) );

$/ = undef;
print <KERNEL>;
print <RAMDISK>;
print <ROOTFS>;
