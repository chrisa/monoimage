#!/usr/bin/perl
use strict;
use Getopt::Std;

use constant BOOTIMAGE_HEADER_SIZE => 16;

my %opt;
getopts('v:f:k:i:r:', \%opt);

my $version = $opt{v};
unless ($version == 1 || $version == 2) {
    print STDERR "must specify version\n";
    exit 1;
}

my $format;
if ($version == 2) {
    if ($opt{f} eq 'ext2') {
	$format = 0;
    }
    if ($opt{f} eq 'ext3') {
	$format = 1;
    }
    if ($opt{f} eq 'tar') {
	$format = 2;
    }
    if ($opt{f} eq 'cpio') {
	$format = 3;
    }
    unless (defined $format) {
	print STDERR "version2, must specify rootfs format\n";
	exit 1;
    }
}

unless (defined $opt{k} && defined $opt{r} && defined $opt{i}) {
    print STDERR "usage: -v version -f root-format -k kernel -i initrd -r rootfs\n";
    exit 1;
}

my ($kernel, $ramdisk, $rootfs) = ($opt{k}, $opt{i}, $opt{r});

my $kernel_size  = (stat $kernel)[7];
my $ramdisk_size = (stat $ramdisk)[7];
my $rootfs_size  = (stat $rootfs)[7];

open KERNEL,  $kernel  or die "kernel: $!";
open RAMDISK, $ramdisk or die "ramdisk: $!";
open ROOTFS,  $rootfs  or die "rootfs: $!";

if ($version == 1) {
    
    print "BI"; 
    print pack("SIII", 1,
	       BOOTIMAGE_HEADER_SIZE,
	       (BOOTIMAGE_HEADER_SIZE + $kernel_size),
	       (BOOTIMAGE_HEADER_SIZE + $kernel_size + $ramdisk_size) );

} elsif ($version == 2) {

    print "BI"; 
    print pack("SSSII", 1,
	       $format,
	       BOOTIMAGE_HEADER_SIZE,
	       (BOOTIMAGE_HEADER_SIZE + $kernel_size),
	       (BOOTIMAGE_HEADER_SIZE + $kernel_size + $ramdisk_size) );
}    

$/ = undef;
print <KERNEL>;
print <RAMDISK>;
print <ROOTFS>;
