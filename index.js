const path = require('path');

var binding = require('node-gyp-build')(path.join(__dirname))
module.exports = binding
