# iptv
an IPTV command line tool

This can be used with .m3u files from: https://github.com/iptv-org/iptv/tree/master/streams

## Dependencies:
- mpv - player
- ncurses

## Notes:
Currently if the stream is broken or 404, the application fails silently. Streams uptime is not always 100%.

## Usage:
Simply compile and point to a folder containing `.m3u` files.
`./iptv /somepath/streams`
