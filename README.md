zimwriterfs
===========

`zimwriterfs` is a console tool to create [ZIM](http://www.openzim.org)
files from a locally-stored directory containing "self-sufficient"
HTML content (with pictures, javascript, and stylesheets). The result will
contain all the files of the local directory compressed and merged in
the ZIM file. Nothing more, nothing less. The generated file can be
opened with a ZIM reader; [Kiwix](http://www.kiwix.org) is one example, but
there are [others](http://openzim.org/wiki/ZIM_Readers).

`zimwriterfs` works - for now - only on POSIX-compatible systems, you
simply need to compile it and run it. The software does not need a lot
of resources, but if you create a pretty big ZIM files, then it could
take a while to complete.

Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the `zimwriterfs` compilation itself, we recommend to have a look
to [kiwix-build](https://github.com/kiwix/kiwix-build).

Preamble
--------

Although `zimwriterfs` can be compiled/cross-compiled on/for many
systems, the following documentation explains how to do it on POSIX
ones. It is primarily though for GNU/Linux systems and has been tested
on recent releases of Ubuntu and Fedora.

Dependencies
------------

`zimwriterfs` relies on many third parts software libraries. They are
prerequisites to the Kiwix library compilation. Following libraries
need to be available:

* liblzma (http://tukaani.org/xz/, most of the time packaged),
  resp. for the LZMA comp.
* libzim (http://openzim.org/download/, probably not packaged),
  resp. for the ZIM compilation
* libmagic (http://www.darwinsys.com/file/, most of the time
  packaged), resp. for the mimeType detection
* libz (http://www.zlib.net/), resp. for unpack compressed HTML files
* gumbo (https://github.com/google/gumbo-parser), a pure-C DOM parser
* libicu (http://site.icu-project.org/), for unicode string
  manipulation. It'always packaged
* libxapian (http://xapian.org), which provide fulltext search
  indexing features.

On (recent) Debian/Ubuntu, you can ensure these are installed with:

```
sudo apt-get install liblzma-dev libmagic-dev zlib1g-dev libgumbo-dev
libzim-dev libxapian-dev libicu-dev
```

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worse case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
meson (through pkg-config) will properly find them.

Environment
-------------

`zimwriterfs` builds using [Meson](http://mesonbuild.com/) version
0.39 or higher. Meson relies itself on Ninja, pkg-config and few other
compilation tools.

Install first the few common compilation tools:
* Meson
* Ninja
* Pkg-config

These tools should be packaged if you use a cutting edge operating
system. If not, have a look to the "Troubleshooting" section.

Compilation
-----------

Once all dependencies are installed, you can compile `zimwriterfs` with:
```
meson . build
ninja -C build
```

By default, it will compile dynamic linked libraries. All binary files
will be created in the "build" directory created automatically by
Meson. If you want statically linked libraries, you can add
`--default-library=static` option to the Meson command.

Depending of you system, `ninja` may be called `ninja-build`.

Installation
------------

If you want to install `zimwriterfs` and the headers you just have
compiled on your system, here we go:

```
ninja -C build install
```

You might need to run the command as root (or using 'sudo'), depending
where you want to install the libraries. After the installation
succeeded, you may need to run ldconfig (as root).

Uninstallation
------------

If you want to uninstall `zimwriterfs`:

```
ninja -C build uninstall
```

Like for the installation, you might need to run the command as root
(or using 'sudo').

Binaries
---------

Statically pre-compiled binaries are provided here
https://download.openzim.org/release/zimwriterfs/.

Troubleshooting
---------------

If you need to install Meson "manually":
```
virtualenv -p python3 ./ # Create virtualenv
source bin/activate      # Activate the virtualenv
pip3 install meson       # Install Meson
hash -r                  # Refresh bash paths
```

If you need to install Ninja "manually":
```
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
