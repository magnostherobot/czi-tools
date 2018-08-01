# CZI Extractor [![Build Status](https://travis-ci.com/magnostherobot/zeiss.svg?token=w7ew2jpnKbReLg9cZxN1&branch=master)](https://travis-ci.com/magnostherobot/zeiss)

Software for extracting images and metadata from Carl Zeiss Images (`.czi`
files). This format is in use in microscopy.

**Note**: This software does not edit or produce any `.czi` files; it only reads
data from already-existing `.czi`s.

## Requirements

### `vips`

These tools require [vips](https://github.com/jcupitt/libvips) version 8 or
greater.

#### Ubuntu

Xenial Ubuntu and onward can install vips using `apt-get`:
```bash
sudo apt-get install libvips-dev
```
Older versions may be able to
[build from source](https://github.com/jcupitt/libvips/wiki/Build-for-Ubuntu#building-from-source).

#### MacOS

Mac users can install `vips` via [`brew`](https://brew.sh/):
```bash
brew install vips --with-imagemagick --without-graphicsmagick --with-openexr --with-openslide
```

#### Fedora
```bash
sudo dnf install vips vips-devel
```

#### Debian
```bash
sudo apt-get install libvips-dev
```

### `libuuid` Development Headers

The `libuuid` development headers are also required for building.
These can usually be installed using your system package manager, or can be
[built from source](https://github.com/karelzak/util-linux/tree/master/libuuid).

### `JxrDecApp`

Microsoft's JPEG-XR decoding tool `JxrDecApp` is required on the system's
`PATH`. Some operating systems may have a package for this, or it can be
[built from source](https://github.com/curasystems/jxrlib/blob/master/Makefile)
(only `JxrDecApp` is required from this codebase, so all necessary building can
be done using `make JxrDecApp`).

## Building

Building should be as simple as
```bash
make
```
**Note**: due to a conflict in different `ld` implementations, MacOS cannot
build this project right now.

Tests are also available:
```bash
make test
```
To move all of the built binaries to the `bin` folder:
```bash
make bin
```

## Running

### Binaries

Each of the utility programs can be found in the appropriate `src` subfolder.

Documentation for the use of these utilities is currently non-existant. All
utilities have help-text that should be kept up-to-date:
```bash
get_region -h
czinspect  -h
```

### Scripts

Some utility scripts have been supplied in the `bin` folder:

- `batch.sh` is a script designed to batch-process a bunch of `.czi` files at
once.
- `czi2png.sh`, `jxr2png.sh`, and `deczi-extracjxr.sh`
are old scripts no longer used.

### Example Use

#### Batch Conversion

You have a directory of `.czi` files called `czi-folder`,
and you want to extract every image tile (converted to `.png`s)
from each of these files into respective subdirectories under the directory
`extracted-tiles`:
```bash
batch.sh -Ed "extracted-tiles" "czi-folder"
```
Each set of `.png`s will turn up in a subdirectory of `extracted-tiles` named
after the `.czi` file they came from.

#### Retreive Specific Region of Image

You've extracted a `.czi` image, the output being a collection of `.tif` images
residing in the directory `all-extracted`, and now you want a `123x789`-sized
region with its top-left corner at x-y coordinate `3030,2929` of the whole
image. Additionally, you want the output to be a `.png`
called `my-region.png`, and the output image
to be at zoom level 8. The appropriate command would be:
```bash
get_region -i "all-extracted" -l 3030 -u 2929 -r 3153 -d 3718 -o "my-region.png" -z 8
```

## Credits

This code is actively developed by David Miller and Tom Harley,
and is based on work by Calum Duff for a project with a team including
Johannes Weck, Hafeez Abdul Rehaman and Josh Lee.
