
NAME
----

packets - read packets and display header information

DESCRIPTION
-----------

Will monitor live network traffic or process a .pcap dump and display
basic header information about the packets.

    $ sudo ./packets wlan0
    Capturing on interface 'wlan0'
    48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
    48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
    0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [ARP] 192.168.2.1 requests 192.168.2.113 
    8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [ARP] 192.168.2.113 at 8c:70:5a:83:2b:64 
    48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
    48:5b:39:5b:6b:87 -> 1:80:c2:0:0:0 [len:46] 
    8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 8.8.8.8 
    0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 8.8.8.8 -> 192.168.2.113 
    8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 149.20.4.69 
    0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 149.20.4.69 -> 192.168.2.113 
    8c:70:5a:83:2b:64 -> 0:1a:70:5a:6e:9 [IPv4] 192.168.2.113 -> 8.8.8.8 
    0:1a:70:5a:6e:9 -> 8c:70:5a:83:2b:64 [IPv4] 8.8.8.8 -> 192.168.2.113 

The [libpcap][libpcap] library is used to read the packets.

 [libpcap]: http://www.tcpdump.org

COPYRIGHT
---------

Copyright &copy; 2015, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

