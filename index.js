const wasmBinaryFile = './bin/GifTools.wasm';
const wasmJSFile = './bin/GifTools.js';

const GifToolsLoader = require(wasmJSFile);;
module.exports = function() {
    return GifToolsLoader({wasmBinaryFile});
}
