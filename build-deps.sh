cd deps

if ! [ -e wxWidgets/build-static/dist ]; then
  git submodule update --checkout
  bash build-wxwidgets.sh
fi
