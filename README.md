ZIM tools
=============

Various ZIM command line tools. More information about the ZIM format
and the openZIM project at http://www.openzim.org/

Disclaimer
----------

This document assumes you have a little knowledge about software
compilation. If you experience difficulties with the dependencies or
with the ZIM libary compilation itself, we recommend to have a look to
[kiwix-build](https://github.com/kiwix/kiwix-build).

Dependencies
------------

The Kiwix library relies on the libzim.

* LibZIM ...................................... http://www.openzim.org/
(package libzim-dev on Debian/Ubuntu)

These dependencies may or may not be packaged by your operating
system. They may also be packaged but only in an older version. The
compilation script will tell you if one of them is missing or too old.
In the worse case, you will have to download and compile a more recent
version by hand.

If you want to install these dependencies locally, then ensure that
meson (through pkg-config) will properly find them.

Environnement
-------------

The ZIM tools build using [Meson](http://mesonbuild.com/) version 0.39
or higher. Meson relies itself on Ninja, pkg-config and few other
compilation tools.

Install first the few common compilation tools:
* Automake
* Libtool
* Virtualenv
* Pkg-config

Then install Meson itself:
```
virtualenv -p python3 ./ # Create virtualenv
source bin/activate      # Activate the virtualenv
pip3 install meson       # Install Meson
hash -r                  # Refresh bash paths
```

Finally we need the Ninja tool (available in the $PATH). Here is a
solution to download, build and install it locally:

```
git clone git://github.com/ninja-build/ninja.git
cd ninja
git checkout release
./configure.py --bootstrap
mkdir ../bin
cp ninja ../bin
cd ..
```

Compilation
-----------

Once all dependencies are installed, you can compile the ZIM tools
with:
```
mkdir build
meson . build
cd build
ninja
```

Depending of you system, `ninja` may be called `ninja-build`.

Installation
------------

If you want to install the ZIM tools you just have compiled on your
system, here we go:

```
ninja install # You have to be in the "build" directory
```

You might need to run the command as root (or using 'sudo'), depending
where you want to install the libraries. After the installation
succeeded, you may need to run ldconfig (as root).

Uninstallation
------------

If you want to uninstall the ZIM tools:

```
ninja uninstall # You have to be in the "build" directory
```

Like for the installation, you might need to run the command as root
(or using 'sudo').

License
-------

GPLv2 or later, see COPYING for more details.