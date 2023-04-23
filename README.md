# ffmpegGUI
by the HaikuArchives Team

ffmpegGUI is a graphical interface to help constructing the commandline for ffmpeg to encode/convert video files. You can also encode files directly with ffmpegGUI and avoid having to open Terminal to paste the assembled ffmpeg commandline. You can also create encoding jobs to process them all in a batch later.

![Screenshot of ffmpegGUI](/documentation/images/mainwindow.png?raw-true "The ffmpegGUI main window")

## Output format
* Containers: avi, mkv, mp4, mpg, ogg, webm / mp3, oga, wav
* Video codecs: mjpeg, mpeg4, theora, vp8, vp9, wmv1, wmv2
* Audio codecs: aac, ac3, dts, flac, mp3, pcm, vorbis

... and many more - ~~sky~~ FFmpeg's codec support list is the limit

## Adjustable settings
* Video bitrate, framerate
* Video resolution, cropping
* Audio bitrate, sampling rate, channel number

For more information, please see the [ffmpegGUI documentation file](http://htmlpreview.github.io/?https://github.com/HaikuArchives/ffmpegGUI/master/documentation/ReadMe.html).

## Building ffmpegGUI

It's very easy, just a ```make``` followed by ```make bindcatalogs``` to include translations.

For the Help menu to work, the contents of the "documentation" folder needs to be copied to, for example, ```/boot/home/config/non-packaged/documentation/packages/ffmpeggui```.
