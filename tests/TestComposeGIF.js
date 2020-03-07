// https://stackoverflow.com/questions/51403326/how-to-use-webassembly-from-node-js

const util = require('util');
const fs = require('fs');

var wasmBinaryFile = '/Users/vserhiienko/Projects/GifTools/build_emsdk/GifTools.wasm';
var wasmJSFile = '/Users/vserhiienko/Projects/GifTools/build_emsdk/GifTools.js';

const GifToolsLoader = require(wasmJSFile);
const GifToolsModule = GifToolsLoader({wasmBinaryFile});

function main() {
    GifToolsModule._testPing();
    var _3 = GifToolsModule._testAdd(1, 2);
    console.log(_3);
}

GifToolsModule.onRuntimeInitialized = function() {
    main();
}
