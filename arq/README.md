NAME
----

arq - automatic repeat request (ARQ) data echo

DESCRIPTION
-----------

This example shows how to construct a simple stop and wait
automatic repeat request (ARQ) algorithm to compensate
for an unreliable sendto function (packetErrorSendTo).

    (terminal 1)
    ./snw-server
    Port: 16245

    (terminal 2)
    ./snw-client localhost 16245 data
    (output will be in data.out)

    md5sum data.*
    (should be identical)

To generate test data the 'dd' command can be used.

    dd if=/dev/urandom of=data bs=1M count=10

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

Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

