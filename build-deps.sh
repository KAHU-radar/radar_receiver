cd deps/wxWidgets
mkdir -p build
cd build
../configure \
  --disable-shared \
  --with-gtk=3 \
  --prefix="$(pwd)/../dist"
make -j$(nproc)
#make install
