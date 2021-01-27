ZIM tools
=============

Various ZIM command line tools. More information about the ZIM format
and the [openZIM project](https://openzim.org).

[![latest release](https://img.shields.io/github/v/tag/openzim/zim-tools?label=latest%20release&sort=semver)](https://download.openzim.org/release/zim-tools/)
[![Reporitories](https://img.shields.io/repology/repositories/zim-tools?label=repositories)](https://github.com/openzim/zim-tools/wiki/Repology)
[![Build Status](https://github.com/openzim/zim-tools/workflows/CI/badge.svg?query=branch%3Amaster)](https://github.com/openzim/zim-tools/actions?query=branch%3Amaster)
[![Docker Build Status](https://img.shields.io/docker/v/openzim/zim-tools?arch=amd64&label=Docker&sort=date)](https://hub.docker.com/r/openzim/zim-tools)
[![codecov](https://codecov.io/gh/openzim/zim-tools/branch/master/graph/badge.svg)](https://codecov.io/gh/openzim/zim-tools)
[![CodeFactor](https://www.codefactor.io/repository/github/openzim/zim-tools/badge)](https://www.codefactor.io/repository/github/openzim/zim-tools)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

`zimwriterfs` is a console tool to create [ZIM](https://openzim.org)
files from a locally-stored directory containing "self-sufficient"
HTML content (with pictures, javascript and stylesheets). The result
will contain all the files of the local directory compressed and
merged in the ZIM file. Nothing more, nothing less. The generated file
can be opened with a ZIM reader; [Kiwix](https://kiwix.org) is one
example, but there are [others](https://openzim.org/wiki/ZIM_Readers).

[![Packaging status](https://repology.org/badge/vertical-allrepos/zim-tools.svg)](https://repology.org/project/zim-tools/versions)

Releases
----------

Tagged source code and prebuilt Linux x86-64 binaries can be found at:

- https://download.openzim.org/release/zim-tools/ 


Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the ZIM libary compilation itself, we recommend to have a look to
[kiwix-build](https://github.com/kiwix/kiwix-build).

Dependencies
------------

Most of the utilities that form the zim-tools programs rely on libzim:

* [ZIM](https://openzim.org) (package `libzim-dev` on Debian/Ubuntu)

`zimdump` additionally requires:
* [docopt.cpp](https://github.com/docopt/docopt.cpp) (package `libdocopt-dev` on Debian/Ubuntu)

`zimwriterfs` relies on many third-party software libraries. They are
prerequisites to compiling zimwriterfs. The following libraries
need to be available:

* [Magic](https://www.darwinsys.com/file/) (package  `libmagic-dev` on Debian/Ubuntu)
* [Z](https://zlib.net/) (package `zlib1g-dev` on Debian/Ubuntu)
* [Gumbo](https://github.com/google/gumbo-parser) (package `libgumbo-dev` on Debian/Ubuntu)
* [ICU](http://site.icu-project.org/) (package `libicu-dev` on Debian/Ubuntu)

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worst case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
meson (through `pkg-config`) will properly find them.

Environment
-------------

The ZIM tools build using [Meson](https://mesonbuild.com/) version
0.43 or higher. Meson relies itself on Ninja, pkg-config and few other
compilation tools.

Install first the few common compilation tools:
* Meson
* Ninja
* Pkg-config

These tools should be packaged if you use a cutting edge operating
system. If not, have a look to the [Troubleshooting](#Troubleshooting)
section.

Compilation
-----------

Once all dependencies are installed, you can compile ZIM tools with:
```bash
meson . build
ninja -C build
```

By default, it will compile dynamic linked libraries. All binary files
will be created in the "build" directory created automatically by
Meson. If you want statically linked libraries, you can add
`-Dstatic-linkage=true` option to the Meson command.

Depending of you system, `ninja` may be called `ninja-build`.

Testing
-------

To run the automated tests:
```bash
cd build
meson test
```

Installation
------------

If you want to install the ZIM tools you just have compiled on your
system, here we go:
```bash
ninja -C build install
```

You might need to run the command as root (or using 'sudo'), depending
where you want to install the libraries. After the installation
succeeded, you may need to run ldconfig (as root).

Uninstallation
------------

If you want to uninstall the ZIM tools:
```bash
ninja -C build uninstall
```

Like for the installation, you might need to run the command as user
`root` (or using `sudo`).

Docker
------

A Docker image with `zimwriterfs`, `zimcheck`,`zimdump` and all the other tools can be built from the `docker`
directory. The project maintains an official image available at https://hub.docker.com/r/openzim/zim-tools.

Troubleshooting
---------------

If you need to install Meson "manually":
```bash
virtualenv -p python3 ./ # Create virtualenv
source bin/activate      # Activate the virtualenv
pip3 install meson       # Install Meson
hash -r                  # Refresh bash paths
```

If you need to install Ninja "manually":
```bash
git clone git://github.com/ninja-build/ninja.git
cd ninja
git checkout release
./configure.py --bootstrap
mkdir ../bin
cp ninja ../bin
cd ..
```

If the compilation still fails, you might need to get a more recent
version of a dependency than the one packaged by your Linux
distribution. Try then with a source tarball distributed by the
problematic upstream project or even directly from the source code
repository.

License
-------

[GPLv3](https://www.gnu.org/licenses/gpl-3.0) or later, see
[LICENSE](LICENSE) for more details.
