
* Add support for streaming
  - the reading of an HTTP body, whether compressed or not should always
    be through a form of stream
  - there are several modes for reading streams and that applies to HTTP/1.1,
    HTTP/2, and HTTP/3 and all need to be taken in account
* Add Brotli compression (enhanced LZ77 by Google "br" encoder/decoder)
* Remove duplicated code for time related convertions/tests (mktime, LEAP).
* Look into supporting all http protocols: HTTP/HTTPS/HTTP2/HTTP3 (libhttp2?)
  - for HTTP/3, the Google library is probably best (quiche)
  - for HTTP/2, there is a library similar to quiche (i.e. just a state machine
    and no direct I/O of its own) named nghttp2
* Create the HTTP client & server as separate headers/implementations.
* Get a better grip on the address ranges.
* Add support for punycode encoding/decoding
  - This offers support for international domain names
  - We need to finish up our Unicode implementation in libutf8
* Add tests for everything to be clearly verified.
* Added a normalization service which can load a file and normalize "anything"
  - we often want to normalize things such as the user agent, this tool could
    be used for that
  - read a file to determine the normalization
  - the format of the file can define a "group" (i.e. user agent, browser
    version, etc.)
* Look into making the list of tags dynamic so new libraries can register
  themselves (i.e. prinbee for gzip)
* Look at the following lists as those represent robots and thus we can avoid
  extra work whenever such connects to us; also if we were to add advertising
  and such on a page, we do not want to do it if the IP is present in one of
  those files or the user agent is clearly representing a robot
  - https://github.com/opawg/user-agents-v2/blob/master/src/apps.json
  - https://ip-ranges.amazonaws.com/ip-ranges.json
  - https://www.gstatic.com/ipranges/goog.json
* Remove the hash-bang support from the uri class (...#!/path/here)
* Consider create a urlencode() and urldecode() that take `std::string &` as
  input and save the results in that string (avoid extra allocations)

