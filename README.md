# XTuner

Tuner for Jack Audio Connection Kit 

## Features

- Virtual Tuner for [Jack Audio Connection Kit](https://jackaudio.org/)
- Including [NSM](https://linuxaudio.github.io/new-session-manager/) support


## Dependencies

- libcairo2-dev
- libx11-dev
- liblo-dev
- libsigc++-2.0-dev
- libzita-resampler-dev
- libjack-(jackd2)-dev

## Build

- git submodule init
- git submodule update
- make
- sudo make install # will install into /usr/bin
