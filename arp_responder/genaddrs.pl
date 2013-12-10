#!/usr/bin/perl
use strict;

#
# This program is used to generate a whole bunch of
# ip and mac address pairs.
#
#   $ ./genaddrs.pl > addresses.txt
#
# Be sure to set the $ip_stem for your own network.
#

my $ip_stem = "192.168.2";
my $mac_stem = "DE:AD:BE:EF:AA";

for (my $i = 1; $i < 254; $i++) {
	print "$ip_stem.$i  $mac_stem:" . sprintf("%02X", $i) . "\n";
}
