NAME
----

arq - Automatic Repeat Request (ARQ) file transfer

DESCRIPTION
-----------

This example shows how to perform reliable data transfers
given unreliable operations by employing [Automatic Repeat
Request (ARQ)][arq] techniques.  In this case a grossly unreliable
sendto (packetErrorSendTo) is used to exaggerate errors.

    (terminal 1)
    ./snw-server
    Port: 16245

    (terminal 2)
    ./snw-client localhost 16245 data
    (output will be in data.out)

    md5sum data.*
    (should be identical)

To generate test data the 'dd' command can be used.
The following generates 5M of random data.

    dd if=/dev/urandom of=data bs=1M count=5

The ARQ scheme is a simple [Stop and Wait][arq] with a single bit sequence
number and no sliding window.  The timeout between resends can be
configured (in arq.h).  The optimal timeout is dependent upon the network
being used.

  [arq]: http://en.wikipedia.org/wiki/Stop-and-wait_ARQ

CREDITS
-------

Jeremiah Mahler <jmmahler@gmail.com>

The unreliable sendto (packetErrorSendTo) was provided by
Dr. Kredo as part of his introduction to network class
(EECE 555) taught at California State University Chico.
It was only provided in binary form.

    packetErrorSendTo.h
    libpacketErrorSendTo_32.a
    libpacketErrorSendTo_64.a

COPYRIGHT
---------

Copyright &copy; 2014, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

