# cpp-torrent-client

This project is a simple torrent client, which takes a torrent file, communicates with a tracker, and communicates with other IP addresses from the trackers to download the full file. This project was written as a fun project as the technology is interesting to me, and it was written in c++, as a way to learn the language (my first c++ project). So please, go easy on me if you are reading the code ;)

# Specifcations
  - HTTP only
  - Single file
  - `.torrent` file needed
  - no magnet links
  - no DHTs

# How to Use it
Once you clone the repository follow these steps:

1. Run `make` command in the terminal to compile the program.
2. Run `./main path/to/torrent/file` (for example: `./main /Users/HughMungus/Desktop/cpp-torrent-client/res/2024-11-13-raspios-bookworm-armhf-full.img.xz.torrent`, if you want to use one of the provided `.torrent` files).
3. Program will run, and once finished, it will put the downloaded file within the `downloaded_files` folder.

# Demonstration Videos

1. `debian-mac-12.7.0-amd64-netinst.iso` -> ~660 mb -> piece size: 256kb -> no partial pieces
 - Link: https://youtu.be/6AmMXIZMFLw
3. `2024-11-13-raspios-bookworm-armhf-lite.img.xz` -> ~535 mb -> piece size: 256mb -> partial piece
 - Link: https://youtu.be/5pi72exMEig
3. `2024-11-13-raspios-bookworm-armhf-full.img.xz` -> ~2.86 gb -> piece size: 2mb -> partial piece
 - Link: https://youtu.be/WWF4WK7AbvQ
