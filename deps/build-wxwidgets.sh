#! /bin/bash

cd wxWidgets

git submodule update --init

make distclean

(cd 3rdparty/pcre; make distclean; rm -rf autom4te.cache config.log config.status Makefile; )
(cd src/expat/expat/; make distclean; rm -rf autom4te.cache config.log config.status Makefile; )

mkdir build-static
cd build-static

../configure \
  --prefix=/your/custom/prefix \
  --with-gtk=3 \
  --enable-unicode \
  --enable-compat28 \
  --enable-std_string_conv_in_wxstring \
  --enable-utf8 \
  --enable-utf8only \
  --enable-std_string \
  --prefix="$(pwd)/dist"
        
# ../configure \
#     --disable-shared \
#     --without-libpng \
#     --without-libjpeg \
#     --without-libtiff \
#     --disable-html \
#     --disable-mediactrl \
#     --disable-webview \
#     --disable-richtext \
#     --disable-xrc \
#     --disable-stc \
#     --disable-aui \
#     --disable-propgrid \
#     --disable-ribbon \
#     --disable-help \
#     --disable-plugins \
#     --without-opengl \
#     --with-libjpeg=builtin \
#     --with-libpng=builtin \
#     --with-zlib=builtin \
#     --with-expat=builtin \
#     --with-libtiff=builtin \
#     --with-regex=builtin \
#     --enable-utf8 \
#     --enable-utf8only \
#     --enable-std_string_conv_in_wxstring \
#     --prefix="$(pwd)/dist" \
#     CXXFLAGS="-DwxUSE_UNSAFE_WXSTRING_CONV=1 -DWXWIN_COMPATIBILITY_2_8=1"

# ../configure \
#     --disable-shared \
#     --without-libpng \
#     --without-libjpeg \
#     --without-libtiff \
#     --disable-html \
#     --disable-mediactrl \
#     --disable-webview \
#     --disable-clipboard \
#     --disable-dnd \
#     --disable-richtext \
#     --disable-xrc \
#     --disable-stc \
#     --disable-aui \
#     --disable-propgrid \
#     --disable-ribbon \
#     --disable-help \
#     --disable-plugins \
#     --without-opengl \
#     --with-libjpeg=builtin \
#     --with-libpng=builtin \
#     --with-zlib=builtin \
#     --with-expat=builtin \
#     --with-libtiff=builtin \
#     --with-regex=builtin \
#     --enable-utf8 \
#     --enable-utf8only \
#     --enable-std_string_conv_in_wxstring \
#     --prefix="$(pwd)/dist"

make
make install
