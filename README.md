
<p align="center">
<img alt="edhttp" title="Event Dispatcher HTTP -- client and server HTTP and some HTML functions."
src="https://snapwebsites.org/sites/snapwebsites.org/files/images/edhttp-logo.png" width="300" height="150"/>
</p>


# Introduction

This event dispatcher extension is the implementation of HTTP 1.1, 2, and 3.
The library also includes all sorts of HTML extensions.


# Sample for Health

The project can include an example class in the library that a service can
create in order to offer a way for other services to check the current status
of a service.

The service can have a way to update its status especially if the it has
some long running tasks (workers threads).


# Change of Mind

The more I'm thinking about it, the more I'm thinking it has to be
an eventdispatcher extension to be 100% seamless. That way one just creates
an `http_client_connection` or an `http_server_connection`. What happens
under the hood would be completely hidden. Plus we can handle the headers
using our classes found in the libsnapwebsites (most already moved here).
So a user sets up the headers, including a buffer of data when required,
then does a `send_request()` on their client or server and voila, it works.

The implementation of HTTP/2 can most certainly be done within the
same `http_client_connection` and `http_server_connection` object.
However, for HTTP/3, one can directly _connect_ to the HTTP/3 UDP
port. Therefore, we should also offer separate classes for that
protocol (`http3_client_connection` and `http3_server_connection`).

The `http_client_connection` should automatically know which type of
connection to attempt first and of course we can give the programmer
ways to give hints (i.e. `set_use(HTTP1_ONLY)`).


# QUIC

The following page has information about the HTTP/3 protocol which piggyback
on HTTP/2. Both of which piggyback on HTTP 0.9/1.0/1.1 for the first
connection although HTTP/3 does introduce a new UDP port which allows for
a direct connection to HTTP/3 instead of having to connect to HTTP/1.1 and
request to convert the connection to HTTP/2.

See https://quicwg.org/

# HTTP/1.1 over TLS

The secure connection we have in eventdispatcher is already a TLS layer all
we have to do is send HTTP data over such a connection. Our main problem is
to do it as a server as well (i.e. be able to switch to the TLS layer when
the HTTP connection happens.) Also that requires the certificate to be a
safe one way connection system.

# HTTP/2

The HTTP/2 service starts with HTTP/1.1 over TLS and then makes a request to
switch to HTTP/2. This still uses TCP, but instead of sending/receiving a
text HTTP header and a body, the system sends binary only in what is called
frames.

Note: servers that do not support HTTP/2 will ignore (not understand) the
request to switch to HTTP/2.

# HTTP/3

From what I understand at this point, the HTTP/3  protocol is very similar
to HTTP/2 over what is called QUIC which is a UDP protocol. The advantage
of using UDP in this case is the fact that one packet won't block another.

The HTTP data itself is transferred using a very similar protocol as with
HTTP/2.

# Layers

    IP <--> TCP <--> TLS <-+-> HTTP/1.1
                           |
	                   +-> HTTP/2

    IP <--> UDP <--> QUIC <--> HTTP/3

The QUIC transport includes the TLS encryption.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/edhttp/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._
