import socket
import time
import select
from optparse import OptionParser

PORT = 8000
HOST = "localhost"


def start_tcp_client():
    """
    Simple TCP client which interact with the tcp server.

    Blocking socket on both side.
    """
    print "Starting Client"
    sock = socket.socket(socket.AF_INET, type=socket.SOCK_STREAM, proto=0)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.connect((HOST, PORT))

    count = 1
    while 1:
        print "sending " + str(count)
        data = sock.recv(10)

        #
        # Does some action on the data. and return the response.
        #
        time.sleep(0.5)

        sock.send('Hi i got the data')
        print data
        count += 1


def start_tcp_client_with_fix():
    """
    TCP client with non-blocking socket.

    The TCP client with fix applied. So it will read complete data from socket
    and avoid the errors like 'errno 104' or 'errno 107'.

    """
    print "Starting the newly implemented client..."

    sock = socket.socket(socket.AF_INET, type=socket.SOCK_STREAM, proto=0)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.connect((HOST, PORT))

    # make the socket non-blocking
    sock.setblocking(0)

    count = 1

    while 1:
        print "sending " + str(count)
        data = ""
        t_data = ""

        rsock, _, _ = select.select([sock], [], [])
        if rsock:
            rsock = rsock[0]
            try:
                while(1):
                    t_data = rsock.recv(1024)
                    data += t_data
            except Exception as ex:
                if ex.errno == 11:  # EAGAIN
                    # Nothing more to read;
                    pass
                else:
                    raise ex
        else:
            continue
        #
        # Doese some action on the package `data`
        #
        print "Handled {} bytes".format(len(data))
        rsock.send(str(len(data)))

        count += 1

if __name__ == "__main__":

    parser = OptionParser()

    parser.add_option("-c", "--client", action="store_true",
                      dest="new_client",
                      default=False,
                      help="Start the new client with fix applied.")

    options, args = parser.parse_args()
    new_client = options.new_client

    if new_client:
        start_tcp_client_with_fix()
    else:
        start_tcp_client()
