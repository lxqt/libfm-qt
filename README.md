# libfm-qt

## Overview

libfm-qt is the Qt port of libfm, a library that provides components for building
desktop file managers, belonging to [LXDE](https://lxde.org).

libfm-qt is licensed under the terms of the
[LGPLv2.1](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
or any later version. See file LICENSE for its full text.  

fm-qt-config.cmake.in is licensed under the terms of the
[BSD-3-Clause](https://opensource.org/licenses/BSD-3-Clause)

## Installation

### Compiling source code

Runtime dependencies are Qt X11 Extras (although libfm-qt works under Wayland too)
and menu-cache (not all libfm features are provided by libfm-qt yet).  
Additional build dependencies are CMake,
[lxqt-build-tools](https://github.com/lxqt/lxqt-build-tools) and, optionally, Git
for pulling latest VCS checkouts.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems. Depending on the way library
paths are dealt with on 64bit systems, variables like `CMAKE_INSTALL_LIBDIR` may
have to be set as well.  

To build run `make`, to install `make install`, which accepts variable `DESTDIR`
as usual.  

### Binary packages

Official binary packages are provided by all major Linux and BSD distributions. Just use your package manager to search for string `libfm-qt`

## Development

Issues should go to the tracker of PCManFM-Qt at
https://github.com/lxqt/pcmanfm-qt/issues.


### Translation

Translations can be done in [LXQt-Weblate](https://translate.lxqt-project.org/projects/lxqt-desktop/libfm-qt/)

<a href="https://translate.lxqt-project.org/projects/lxqt-desktop/libfm-qt/">
<img src="https://translate.lxqt-project.org/widgets/lxqt-desktop/-/libfm-qt/multi-auto.svg" alt="Translation status" />
</a>
