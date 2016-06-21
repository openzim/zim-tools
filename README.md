zimwriterfs
-----------

`zimwriterfs` is a console tool to create [ZIM](http://www.openzim.org)
files from a locally-stored directory containing "self-sufficient"
HTML content (with pictures, javascript, and stylesheets). The result will
contain all the files of the local directory compressed and merged in
the ZIM file. Nothing more, nothing less. The generated file can be
opened with a ZIM reader; [Kiwix](http://www.kiwix.org) is one example, but
there are [others](http://openzim.org/wiki/ZIM_Readers).

`zimwriterfs` works for now only on POSIX-compatible systems, you simply
need to compile it and run it. The software does not need a lot of
resources, but if you create a pretty big ZIM files, then it could
take a while to complete.

GNU/Linux compilation
---------------------

To compile zimwriterfs, you need the GNU compiler with the GNU
autotools suite. You also need to have a few dependencies (dev
version) installed on the system:

* liblzma (http://tukaani.org/xz/, most of the time packaged),
  resp. for the LZMA comp.
* libzim (http://openzim.org/download/, probably not packaged),
  resp. for the ZIM compilation
* libmagic (http://www.darwinsys.com/file/, most of the time
  packaged), resp. for the mimeType detection
* libz (http://www.zlib.net/), resp. for unpack compressed HTML files
* gumbo (https://github.com/google/gumbo-parser), a pure-C DOM parser

On Debian, you can ensure these are installed with:
```
sudo apt-get install liblzma-dev libmagic-dev zlib1g-dev libgumbo-dev
cd ../zimlib && ./autogen.sh && ./configure && make && cd ../zimwriterfs
```

Once the dependencies are in place, to build:
```
./autogen.sh
./configure CXXFLAGS=-I../zimlib/include LDFLAGS=-L../zimlib/src/.libs
make
```

OSX compilation
---------------
OSX builds are similar to Linux, except we use homebrew.  Change to
`../zimlib` and build zimlib as instructed in the README there.  Then
return here and:
```
brew install gumbo-parser
./autogen.sh
./configure CXXFLAGS="-I../zimlib/include -I/usr/local/include" LDFLAGS=-L../zimlib/src/.libs
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
