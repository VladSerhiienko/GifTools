var wasmBinaryFile = '../bin/GifTools.wasm';
var wasmJSFile = '../bin/GifTools.js';

const GifToolsLoader = require(wasmJSFile);

GifToolsLoader({wasmBinaryFile})
    .then(GifToolsModule => {
        var _3 = GifToolsModule._testAdd('12e10', 2);
        console.log(_3);
    });
