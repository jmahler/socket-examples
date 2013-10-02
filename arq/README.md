NAME
----

arq - automatic repeat request example

DESCRIPTION
-----------

This example shows how to construct a simple stop and wait
automatic repeat request (ARQ) algorithm.  An unreliable
sendto() function is provided which will drop packets.
And a layer outside of this function is built to make it reliable.

This program was derived from the 'udp-echo' example program.
Refer to that project for more usage examples.

CREDITS
-------

Jeremiah Mahler <jmmahler@gmail.com>

The unreliable sendto was provided by Dr. Kredo as part of
his introduction to network class (EECE 555) taught at
California State University Chico.

    packetErrorSendTo.h
    libpacketErrorSendTo_32.a
    libpacketErrorSendTo_64.a

COPYRIGHT
---------

Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

