# PING Keep Alive

a lean and mean Win32 C based background IPv4 pinger, a use case would be to keep problematic WiFi connection alive by periodically pinging the gateway in the background

  - Statically compiled with no runtime dependencies. Very low memory usage around 1 megabyte RAM (and that's GUI with systray icon) and 100KB binary!
  - Program minimizes to system tray on close
  - By default pings the target every 1500ms

# Tech

* PING Keep Alive is written in C and uses native Win32 API
* Included is a PINGKeepAlive_v0.1.exe installer file in "Releases" section of this github page

![gui](https://raw.githubusercontent.com/fatalhalt/pingkeepalive/master/screenshot.png?raw=true)

License
----

GPL2
