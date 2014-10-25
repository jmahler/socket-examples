NAME
----

oneftp - transfer one file from a client to a server

DESCRIPTION
-----------

This is a very simple file transfer program which can be used to
transfer one file at a time.  The server is started and given a file
name where the data will be saved to.  The client then connects to the
server and sends a file.

    (terminal 1)
    $ ./oneftp_server out.dat

    (terminal 2)
    $ ./oneftp_client localhost in.dat

When the file is transferred both the server and client exit.
And the output file should be identical to the input file.

    $ md5sum *.dat
    5bd3f31b788354d533814ae9b225a4e2  in.dat
    5bd3f31b788354d533814ae9b225a4e2  out.dat
    $

The chunk size is configurable by setting the macro BUFSIZE
in each of the source files.  Timing tests can be run to
see the effect changes in chunk size have on transfer speed.

    $ time ./oneftp_client localhost in.dat
    real    0m0.023s
    user    0m0.000s
    sys     0m0.012s
    $

CREDITS
-------

Jeremiah Mahler <jmmahler@gmail.com>.

COPYRIGHT
---------

Copyright &copy; 2014, Jeremiah Mahler.  All Rights Reserved.<br>
This project is free software and released under
the [GNU General Public License][gpl].

 [gpl]: http://www.gnu.org/licenses/gpl.html

