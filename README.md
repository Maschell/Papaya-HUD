# Papaya HUD

This is a HUD plugin for the Aroma CFW on the Wii U.


## Features

Supported fields:

 - FPS

 - Network bandwidth usage


## Build instructions

This is an Automake package that's intended to be cross-compiled using devkitPro's
environment.

If you got the sources through a release tarball, you can skip step 0.

0. `./bootstrap`

1. `./configure --host=powerpc-eabi`

2. `make`


