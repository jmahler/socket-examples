
NAME
----

arp_resolver - send ARP requests and display the response

DESCRIPTION
-----------

Given an IP address as input it tries to resolve the MAC address using ARP.

    $ sudo ./arp_resolver wlan0
    Enter next IP address: 192.168.2.1
    MAC: AB:CD:EF:00:12:34
    Enter next IP address: 8.8.8.8
    MAC: Lookup failed
    Enter next IP address:
    $


The [libpcap][libpcap] library is used to send/receive the packets.

 [libpcap]: http://www.tcpdump.org

COPYRIGHT
---------

Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

