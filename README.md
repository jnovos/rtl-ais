`rtl-ais`, a simple AIS tuner and generic dual-frequency FM demodulator
-----------------------------------------------------------------------

rtl-ais provides the `rtl_ais` command, which decodes AIS data from Software Defined Radio (SDR) and outputs `AIVDM` / `AIVDO` sentences. Support for **own mmsi** and **configured for openwrt** 

This is a [mik3y](https://github.com/mik3y) fork, thank Tcp socket keep option -k :-)

| OS support |   |
|------------|---|
| Linux      | ✅ |


Command Line
------------
```
Use: rtl_ais [options] [outputfile]
        [-l left_frequency (default: 161.975M)]
        [-r right_frequency (default: 162.025M)]
            left freq < right freq
            frequencies must be within 1.2MHz
        [-s sample_rate (default: 24k)]
            maximum value, might be down to 12k
        [-o output_rate (default: 48k)]
            must be equal or greater than twice -s value
        [-E toggle edge tuning (default: off)]
        [-D toggle DC filter (default: on)]
        [-d device_index (default: 0)]
        [-g tuner_gain (default: automatic)]
        [-p ppm_error (default: 0)]
        [-R enable RTL chip AGC (default: off)]
        [-A turn off built-in AIS decoder (default: on)]
            use this option to output samples to file or stdout.
        Built-in AIS decoder options:
        [-h host (default: 127.0.0.1)]
        [-P port (default: 10110)]
        [-T use TCP communication as tcp listener ( -h is ignored)]
        [-k keep TCP socket open and write new messages to it as they arrive]
        [-t time to keep ais messages in sec, using tcp listener (default: 15)]
        [-n log NMEA sentences to console (stderr) (default off)]
        [-M your MMSI identification number]
			  [-v Debug and verbosity]
        [-L log sound levels to console (stderr) (default off)]
        [-I add sample index to NMEA mesages (default off)]
        [-S seconds_for_decoder_stats (default 0=off)]
        When the built-in AIS decoder is disabled the samples are sent to
        to [outputfile] (a '-' dumps samples to stdout)
            omitting the filename also uses stdout
        Output is stereo 2x16 bit signed ints
        Examples:
        Receive AIS traffic,sent UDP NMEA sentences to 127.0.0.1 port 10110
             and log the senteces to console:
        rtl_ais -n
        Tune two fm stations and play one on each channel:
        rtl_ais -l233.15M  -r233.20M -A  | play -r48k -traw -es -b16 -c2 -V1 -
        Example  mmsi + ppm + gain + Tcp + keep TCP   
```


Compiling
---------
Make sure you have the following dependencies:
  - librtlsdr
  - libusb
  - libpthread

```console
$ # Get the source code:
$ git clone https://github.com/jnovos/rtl-ais
$ # Change to the source dir
$ cd rtl-ais
$ make
$ # Test running the command
$ ./rtl_ais
```

Installing
----------
* On Linux, `sudo make install`

Running
-------

rtl-ais uses software defined radio (SDR).  The specific
hardware we use for this is a DVB-T dongle. A good starting point is:
https://www.rtl-sdr.com/about-rtl-sdr

You'll need also an antenna, and be located near (some miles)  the
passing vessels.

You'll also need to do some procedure to get the tunning error for the
specfic dongle you have (aka ppm error), and pass that number as parameter
of rtl-ais.
Example: `` /usr/bin/rtl_ais -M [Own-MMSi] -p [ppm-value] -g[gain] -T -k `` Send nmea default port 10110 by TCP an eliminates own MMSi


Known Issues
------------
* The `[-p ppm error]` parameter is essential for rtl_ais to work.
  * The ppm error is the frequency deviation in parts per million from the desired tuning
frequency, and the real tuned frequency due to the crystal oscillator deviation. This
figure is different for each device, it's very important to know  this value and pass this parameter to rtl_ais.

How to find the correct ppm

In Debian:
```
  sudo apt-get install libtool autoconf automake libfftw3-dev
  git clone https://github.com/steve-m/kalibrate-rtl.git
  cd kalibrate-rtl/
  ./bootstrap
  ./configure
  make
```
Execute:
 * **kal -s GSM900 -g 48**  Show a list with all channels.
 * Choose the best channel.
 * **kal -c `number-channel` -g 48 -v** At the end of the command execution the ppm will be displayed "average absolute error : value ppm"

  * Some instructions for get the ppm error are here:
    http://www.rtl-sdr.com/how-to-calibrate-rtl-sdr-using-kalibrate-rtl-on-linux
  * and here (using SDR#):
    http://www.atouk.com/SDRSharpQuickStart.html#adjusting
  * and here (using HDSDR ad AIS traffic)
    http://www.cruisersforum.com/forums/f134/new-rtlsdr-plugin-102929-11.html#post1844966
