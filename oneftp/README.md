NAME
----

oneftp - transfer one file from a client to a server

DESCRIPTION
-----------

This is a very simple file transfer program which can be used to
transfer one file at a time.  The server is started and told
what file name to save the data as.  Then the client is started
and told what host to send to and what file to send.

    (terminal 1)
    $ ./oneftp_server outfile.txt

    (terminal 2)
    $ ./oneftp_client localhost infile.txt

When the file is transferred both the server and client exit.

CREDITS
-------

Jeremiah Mahler <jmmahler@gmail.com>.

COPYRIGHT
---------

Copyright &copy; 2013, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

