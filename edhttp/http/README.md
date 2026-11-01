
# Organization

The HTTP implementation is pretty large so I create separate files for
each class. All of which are detail classes except for
`http_client_connection`. These detail classes, you do not have access
to. For my own sake, though here is how these are organized.

    http_client_connection
        |
        +---> http_client_connection_impl
                  |
                  +---> http_domain
                  |         |
                  |         +---> http_tcp_client_async_connection
                  |                   |
                  |                   +---> thread_signal_handler
                  |                   |
                  |                   +---> runner
                  |                   |
                  |                   +---> http_tcp_client_connection
                  |          
                  +---> http_udp_connection

## `http_client_connection`

This is the connection you create and manage. It allows you to send
requests and receive responses. That's pretty much it. There are a few
settings you can update before sending requests.

The `http_client_connection` creates an implementation which is just
a timer. That class is what manages the internal state.

## `http_client_connection_impl`

This implementation manages any number of TCP connections,
(`http_tcp_client_async_connection`), and UDP connections
(`http_udp_connection`).

The TCP connections are teared down if unused for a while.
This is because we can keep them alive for a time but in
most cases this is really short (maybe 1 or 2 minutes).

For HTTP/2 connections and websockets, this may be longer.

The service also limits the number of TCP connections as much
as possible. Only connections still in use are guaranteed to
be kept around.

For the UDP connections, the system has one or two depending on
the IP addresses. It needs one `tcp_udp_connection` per type of
IP: so one for IPv4 and one for IPv6.

## `http_udp_connection`

Since a UDP connection is not actually connected, it can be
used over and over again. So the `http_client_connection_impl`
creates one instance of this class per IP type and uses them for
the entire lifetime of the `http_client_connection_impl` object.

## `http_tcp_client_async_connection`

The asynchronous TCP client connection manages a connection which
can connect without the wait by using a thread. This allows the
system to attempt multiple connections in parallel as per the
Ha[[y Eyeballs algorithm.

The thread used to connect is expected to send a signal once done
using a `thread_signal_handler` object.

The thread is passed an instance of a `runner` which attempts the
connection to the server. Since it uses a thread, it is non-blocking
for the system as a whole.

Note: one day we'll try with a non-blocking socket instance. This
      will require connections to know/understand three states
      (connecting, connected, disconnected).

## `thread_signal_handler`

The `thread_signal_handler` is a connection used by the
`http_tcp_client_async_connection` to be awaken once the `runner` is
done, whether it successfully connected or not.

## `runner`

The thread runner attempting a connection to an HTTP server
using the TCP protocol. The thread does a single connection
attempt. The different attempts are managed by the `http_domain`
object which knows how to handle the Happy Eyeballs algorithm.

It uses the specific IP address passed down and the mode
as defined in the `http_request` object to know how to connect.

The host is necessary if you attempt a secure connection (it is
sent in the TLS handshaker so the correct certificate can be
used). The mode defines whether to use TLS or not.

In TLS mode, we attempt a connection with either HTTP/2 or HTTP/1.1.
In plain mode, we automatically fallback to using HTTP/1.1.

TODO: create a few threads that we can reuse instead of create/destroy
      as now.

## `http_tcp_client_connection`

The actual HTTP connection is created by this object. This is an actual
`ed::tcp_client_connection` which knows how to read and write HTTP code
to send a request and receive a response.

These objects are created by the `runner`. They also are ephemeral
compared to the `http_domain` object which can re-create
`http_tcp_client_async_connection` objects as required.

## `http_request`

This class is not a detail. You actually setup one of those objects
before sending it to your `http_client_connection`. As a result,
you get a callback to your implementation of the
`http_client_connection::process_response()`
virtual function with an `http_response`.

Note that it is possible to create an `http_client_connection` and
setup a callback to receive the `http_response` through that callback
instead of re-implementing the `http_client_connection` object. The
`http_request` also supports a callback so that way you can specifically
get called for that one request.

## `http_response`

Once a request was sent, the HTTP connection waits for a response.
This may include data or just some headers. Once the response was
read in its entirety, the system calls your callback which can
then handle the content.

TODO: look at implementing a streaming version so once we got the
      header the body gets read by the caller through a stream
      instead of using a potentially very large memory buffer. The
      `http_response` object would be capable of returning a handle
      to that stream.

