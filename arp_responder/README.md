
NAME
----

arp_responder - respond to ARP requests with fake entries

DESCRIPTION
-----------

Given a file of IP address and MAC addresses

    192.168.2.189  C0:04:AB:43:22:FF
    192.168.2.190  D4:DE:AD:BE:EF:FF

this program will respond to any requests for these entries.

    $ sudo ./arp_responder eth0 addresses.txt
    ^C (quit)
    $

On an wired network if this program is running on machine A,
and then one of the entries is pinged by machine B, a complete
entry should appear for that address in the ARP table of machine B.

    (machine B)

    root@raspberrypi:~# arp -n
    Address                  HWtype  HWaddress           Flags Mask            Iface
    192.168.2.1              ether   00:1a:70:5a:6e:09   C                     eth0
    192.168.2.189            ether   c0:04:ab:43:22:ff   C                     eth0
    root@raspberrypi:~# ping 192.168.2.190
    PING 192.168.2.190 (192.168.2.190) 56(84) bytes of data.
    ^C
    --- 192.168.2.190 ping statistics ---
    3 packets transmitted, 0 received, 100% packet loss, time 2004ms
    
    root@raspberrypi:~# arp -n
    Address                  HWtype  HWaddress           Flags Mask            Iface
    192.168.2.190            ether   d4:de:ad:be:ef:ff   C                     eth0
    192.168.2.1              ether   00:1a:70:5a:6e:09   C                     eth0
    192.168.2.189            ether   c0:04:ab:43:22:ff   C                     eth0
    root@raspberrypi:~#

Notice that even though the ping did not work an new ARP entry
for .190 appeared.

The [libpcap][libpcap] library is used to send/receive the packets.

 [libpcap]: http://www.tcpdump.org

COPYRIGHT
---------

Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

