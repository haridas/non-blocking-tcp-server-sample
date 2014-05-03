non-blocking-tcp-server-sample
==============================

Simple Non blocking TCP Server and client to simulate the socket error 104 and 107 at the client side.

For more details about this repo, please read my blog entry - 

http://haridas.in/what-happens-when-we-hit-the-speed-limit-of-python.html


How to Run this TCP server and Client
-------------------------------------

Start the C server. Which listening on the port 8000 and handles the
Client connections on another thread.


    $ gcc tcp_server.c
    $ ./a.out


On another shell environemnt run python client.
It has two options -

1. Run the Python client which gives the error 104 and 107 issue. Which is the
   issue I'm getting right now on the production machine.

2. Another implementation of the Python client which fixes the issue and read
   the socket data without causing socket error 104 and 107

    $ python tcp_server.py  # Simulate the tcp client with socket problem.

This will crash after while since the server closes the socket on its
side due to the kernel sending buffer is full on the server side.

    $ python tcp_server.py -c # This client has the option to fix that issue.

This will read the socket data in non-blocking mode and read entire
data from the socket till it throws EAGAIN exception.

This time the client won't throw any excetion and the server and client
work smoothly. So finally I got the protype to fix the actual production
system.

To See what is happening while running this scripts run `tcpdump`
caommand.

    $ sudo tcpdump -i any port 8000
