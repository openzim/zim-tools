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

GNU/Linux compilation
---------------------

To compile zimwriterfs, you need the GNU compiler with the GNU
autotools suite. You also need to have a few software dependencies
(dev version) installed on the system:

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

On Debian, you can ensure these are installed with:

```
sudo apt-get install liblzma-dev libmagic-dev zlib1g-dev libgumbo-dev
libzim-dev libxapian-dev
```

Once the dependencies are in place, to build:
```
./autogen.sh
./configure
make
```

The above listed commands assume that all the software libraries are
available in the standard places known from the operating system. If
not, you will have to specify their location at the configuration step. To get more details:
```
./configure --help
```

If you want to install zimwriterfs on your system:
```
sudo make install
```

OSX compilation
---------------
OSX builds are similar to Linux, except we use homebrew.  Change to
`../libzim` and build libzim as instructed in the README there.  Then
return here and:
```
brew install gumbo-parser
./autogen.sh
./configure CXXFLAGS="-I../libzim/include -I/usr/local/include" LDFLAGS=-L../libzim/src/.libs
make
```

Alternatively, there is a script included here to help you build both
static and dynamic binaries for `zimwriterfs`.
You must have a working and set up Kiwix repository (with dependencies ready).

1. Install libmagic with brew (it's important)
	- `ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`
	- `brew install libmagic`
2. Build kiwix:
	- `KIWIX_ROOT=/Users/xxx/src/kiwix ./macosx-build.sh`

Troubleshooting
---------------

If the compilation fails, you might need to get a more recent version
of a dependency than the one packaged by your Linux distribution. Try
then with a source tarball distributed by the problematic upstream
project or even directly from the source code repository.
