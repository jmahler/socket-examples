NAME
----

arq - Automatic Repeat Request (ARQ) file transfer

DESCRIPTION
-----------

This example shows how to perform reliable data transfers
given unreliable operations by employing [Automatic Repeat
Request (ARQ)][arq] techniques.  In this case a grossly unreliable
sendto (packetErrorSendTo) is used to exaggerate errors.

    # First, create some data to transfer.
    dd if=/dev/urandom of=data bs=1M count=5

    # Start the server so it can receive the data.
    (terminal 1)
    ./snw-server
    Port: 16245

    # Run the client to transfer 'data' to the server.
    (terminal 2)
    ./snw-client localhost 16245 data
    (output will be in data.out)

    # Verify that the transfer was a success
    md5sum data data.out
    (should be identical)

The ARQ scheme is a simple [Stop and Wait][arq] with a single bit sequence
number and no sliding window.  The timeout between resends can be
configured (in arq.h).  The optimal timeout is dependent upon the network
being used.

  [arq]: http://en.wikipedia.org/wiki/Stop-and-wait_ARQ

CREDITS
-------

Jeremiah Mahler <jmmahler@gmail.com>

COPYRIGHT
---------

Copyright &copy; 2015, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

