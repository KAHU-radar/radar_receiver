export CXXFLAGS="-I$(pwd)/deps/wxWidgets/dist/include"
export LDFLAGS="-L$(pwd)/deps/wxWidgets/dist/lib"

npm install
npm run build
