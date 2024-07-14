
* Remove duplicated code for time related convertions/tests (mktime, LEAP).
* Look into supporting all http protocols: HTTP/HTTPS/HTTP2/HTTP3 (libhttp2?)
* Create the HTTP client & server as separate headers/implementations.
* Get a better grip on the address ranges.
* Add tests for everything to be clearly verified.
* Added a normalization service which can load a file and normalize "anything"
  - we often want to normalize things such as the user agent, this tool could
    be used for that
  - read a file to determine the normalizations
  - the format of the file can define a "group" (i.e. user agent, browser
    version, etc.)

