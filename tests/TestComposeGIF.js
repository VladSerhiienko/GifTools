// https://stackoverflow.com/questions/51403326/how-to-use-webassembly-from-node-js

const util = require('util');
const fs = require('fs');

var wasmBinaryFile = '/Users/vserhiienko/Projects/GifTools/build_emsdk/GifTools.wasm';
var wasmJSFile = '/Users/vserhiienko/Projects/GifTools/build_emsdk/GifTools.js';

const GifToolsLoader = require(wasmJSFile);
const GifToolsModule = GifToolsLoader({wasmBinaryFile});

function loadToUint8Array(file) {
    const fileBuffer = fs.readFileSync(file);
    return new Uint8Array(fileBuffer);
}

function main() {

    var tempFileBuffer = {};
    var bufferIds = [];

    tempFileBuffer = loadToUint8Array('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083058.jpg');
    bufferIds[0] = GifToolsModule.bufferFromTypedArray(tempFileBuffer);

    tempFileBuffer = loadToUint8Array('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg');
    bufferIds[1] = GifToolsModule.bufferFromTypedArray(tempFileBuffer);

    tempFileBuffer = loadToUint8Array('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083101.jpg');
    bufferIds[2] = GifToolsModule.bufferFromTypedArray(tempFileBuffer);

    tempFileBuffer = loadToUint8Array('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg');
    bufferIds[3] = GifToolsModule.bufferFromTypedArray(tempFileBuffer);

    tempFileBuffer = null;

    console.log('bufferId=', bufferIds[0]);
    console.log('bufferSize=', GifToolsModule._bufferSize(bufferIds[0]));
    console.log('bufferEmpty=', GifToolsModule._bufferEmpty(bufferIds[0]));

    // bufferIds[1] = GifToolsModule.bufferFromTypedArray(fileBuffers[1]);
    // bufferIds[2] = GifToolsModule.bufferFromTypedArray(fileBuffers[2]);
    // bufferIds[3] = GifToolsModule.bufferFromTypedArray(fileBuffers[3]);

    // console.log(bufferIds);


    // GifToolsModule._testPing();
    // var a = GifToolsModule.lerp(2, 4, 0.5);
    // var b = GifToolsModule._testAdd(1, 2);
    // console.log(a);
    // console.log(b);
}

GifToolsModule.onRuntimeInitialized = function() {
    main();
}
