# FFmpegGUI
By Zach Dykstra et al.

FFmpegGUI is a Haiku graphical frontend for FFmpeg, which allows users to easily transcode video and audio files. FFmpegGUI parses user's input and produces a ready to use FFmpeg command.

 This app uses [liblayout](https://github.com/diversys/liblayout) for easier arrangement of graphical elements.

![Screenshot of ffmpegGUI](/screenshot.png?raw-true "Default ffmpegGUI screen")

## Available options
### Output format
* Container: avi, vcd, mp4, mpeg, mkv, webm
* Video: mpeg4, vp7, vp8, vp9, wmv1
* Audio: ac3, aac, opus, vorbis

... and many more - ~~sky~~ FFmpeg's codec support list is the limit

## Adjustable settings
* Bitrate
* Framerate
* Audio Sampling Rate
* Video crop
* Video Resolution
