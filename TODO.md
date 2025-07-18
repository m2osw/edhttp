
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
* Look into making the list of tags dynamic so new libraries can register
  themselves (i.e. prinbee for gzip)
* Look at the following lists as those represent robots and thus we can avoid
  extra work whenever such connects to us; also if we were to add advertising
  and such on a page, we do not want to do it if the IP is present in one of
  those files or the user agent is clearly representing a robot
  - https://github.com/opawg/user-agents-v2/blob/master/src/apps.json
  - https://ip-ranges.amazonaws.com/ip-ranges.json
  - https://www.gstatic.com/ipranges/goog.json

