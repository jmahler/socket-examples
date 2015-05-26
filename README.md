
# NAME

socket-examples - network socket example programs

# DESCRIPTION

This is a collection of network socket example programs.  They range
from trivial echo programs to more complex packet manipulation programs.

## echo

The simplest "hello, world" socket program is the echo client/server.
The client connects to the server and any input it sends is echoed back.

    (terminal 1)
    ./server
    Listening on port 5123
    
    (terminal 2)
    ./client server.com 5123
    abcd
    abcd
    (input is echoed back, server is working)

Examples are given for TCP, UDP and UNIX domain sockets.

## calc

The `calc` client sends a mathematical expression to the `calc server
which then calculates the answer and returns the result.

    (terminal 1)
    $ ./calc_server 5001
    
    (terminal 2)
    $ ./calc_client localhost 5001
    Enter expression:1+1
    Answer: 2
    ...

## oneftp

The `oneftp` program transfers one file from the client to the server.
It shows how to properly detect the end of a large transfer.

    (terminal 1)
    $ ./oneftp_server out.dat

    (terminal 2)
    $ ./oneftp_client localhost in.dat


## arq

[Automatic repeat request (ARQ)][arq] is the general idea of resending a
packet that has been lost.  The `arq` program uses a socket library
which is unreliable.  It then employs a stop and wait ARQ scheme to
overcome these errors and reliably perform transfers.

 [arq]: http://en.wikipedia.org/wiki/Automatic_repeat_request

    (terminal 1)
    ./snw-server
    Port: 16245
    
    (terminal 2)
    ./snw-client localhost 16245 data
    (output will be in data.out)
    
    md5sum data.*
    (should be identical)

## packets

The `packets` program uses the [libpcap][libpcap] library, like
Wireshark does, to monitor the packets being transferred on an
interface.  It then displays information about the packets such as the
addresses and type.

  [libpcap]: http://www.tcpdump.org

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

## arp_resolver

The `arp_resolver` program takes an IP address as input and tries to
resolve the MAC address by sending out ARP packets.

    $ sudo ./arp_resolver wlan0
    Enter next IP address: 192.168.2.1
    MAC: AB:CD:EF:00:12:34
    Enter next IP address: 8.8.8.8
    MAC: Lookup failed
    Enter next IP address:
    $

## arp_responder

*IMPORTANT* Do not run this program on a public network.  This could be
seen as an attack and you could be prosecuted by the authorities.

When an ARP request is sent asking for the MAC address for a given IP it
should only receive one response.  The `arp_responder` will maliciously
create a response for addresses it is not necessarily authoritative for.

Given a file of IP address and MAC addresses

    192.168.2.189  C0:04:AB:43:22:FF
    192.168.2.190  D4:DE:AD:BE:EF:FF

this program will respond to any requests for these entries.

    $ sudo ./arp_responder eth0 addresses.txt
    ^C (quit)
    $

See the README included with `arp_responder` for more information.

# COPYRIGHT

Copyright &copy; 2015, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

