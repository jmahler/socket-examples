NAME
----

calc_server/calc_client - Calculator server and client.

DESCRIPTION
-----------

As a simple server client example, this server provides the ability
to take a math expression as input and provide the result as output.
The client takes input from a user or from a file and sends them
to the server.  And the results are then printed back out.

    (terminal 1)
    $ ./calc_server 5001

    (terminal 2)
    $ ./calc_client localhost 5001
    Enter expression:1+1
    Answer: 2
    ...

CREDITS
-------

The `calc_server.c` code was provided by Dr. Kurtis Kredo III
as part of his intro to networking class (EECE 555) taught at
California State University Chico.

The `calc_client.c` code along with various additions to the server
were provided by Jeremiah Mahler <jmmahler@gmail.com>.

COPYRIGHT
---------

Copyright &copy; 2014, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

