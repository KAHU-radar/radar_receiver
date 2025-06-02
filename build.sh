export CXXFLAGS="-I$(pwd)/deps/wxWidgets/dist/include"
export LDFLAGS="-L$(pwd)/deps/wxWidgets/dist/lib"

node-gyp configure
node-gyp build V=1

#npm install
#npm run build
