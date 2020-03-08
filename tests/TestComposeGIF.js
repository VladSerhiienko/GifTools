// https://stackoverflow.com/questions/51403326/how-to-use-webassembly-from-node-js

const util = require('util');
const fs = require('fs');

var wasmBinaryFile = '/Users/vserhiienko/Projects/GifTools/build_emscripten/GifTools.wasm';
var wasmJSFile = '/Users/vserhiienko/Projects/GifTools/build_emscripten/GifTools.js';

const GifToolsLoader = require(wasmJSFile);
const GifToolsModule = GifToolsLoader({wasmBinaryFile});

function loadToUint8ArrayFromFile(file) {
    const fileBuffer = fs.readFileSync(file);
    return new Uint8Array(fileBuffer);
}
function writeUint8ArrayToFile(file, fileBuffer) {
    fs.writeFileSync(file, fileBuffer);
}

function loadImageFromFileAndResize(file, width, height) {
    var fileBuffer = loadToUint8ArrayFromFile(file);
    var bufferId = GifToolsModule.bufferFromTypedArray(fileBuffer);

    console.log('buffer', 'id', bufferId, 'size', GifToolsModule._bufferSize(bufferId));

    var imageId = GifToolsModule._imageLoadFromBuffer(bufferId);

    console.log('image',
                'id',
                imageId,
                'w',
                GifToolsModule._imageWidth(imageId),
                'h',
                GifToolsModule._imageHeight(imageId),
                'f',
                GifToolsModule._imageFormat(imageId));


    GifToolsModule._objectFree(bufferId);
    var smallImageId = GifToolsModule._imageResizeOrClone(imageId, width, height);

    console.log('resized image',
                'id',
                smallImageId,
                'w',
                GifToolsModule._imageWidth(smallImageId),
                'h',
                GifToolsModule._imageHeight(smallImageId),
                'f',
                GifToolsModule._imageFormat(smallImageId));

    GifToolsModule._objectFree(imageId);
    return smallImageId;
}

function main() {
    const delay = 100;
    const width = 1200;
    const height = 900;

    var smallImageIds = [];
    smallImageIds[0] = loadImageFromFileAndResize('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083058.jpg', width, height);
    smallImageIds[1] = loadImageFromFileAndResize('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg', width, height);
    smallImageIds[2] = loadImageFromFileAndResize('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083101.jpg', width, height);
    smallImageIds[3] = loadImageFromFileAndResize('/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg', width, height);

    var gifBuilderId = GifToolsModule._gifBuilderInitialize(width, height, delay);
    GifToolsModule._gifBuilderAddImage(gifBuilderId, smallImageIds[0], delay);
    GifToolsModule._gifBuilderAddImage(gifBuilderId, smallImageIds[1], delay);
    GifToolsModule._gifBuilderAddImage(gifBuilderId, smallImageIds[2], delay);
    GifToolsModule._gifBuilderAddImage(gifBuilderId, smallImageIds[3], delay);
    var gifBufferId = GifToolsModule._gifBuilderFinalize(gifBuilderId);

    console.log('gif buffer', 'id', gifBufferId, 'size', GifToolsModule._bufferSize(gifBufferId));


    var gifFileBuffer = GifToolsModule.bufferMemoryView(gifBufferId);
    console.log(gifFileBuffer);

    writeUint8ArrayToFile('/Users/vserhiienko/Downloads/Photos/BuiltGif.gif', gifFileBuffer);

    GifToolsModule._objectFree(smallImageIds[0]);
    GifToolsModule._objectFree(smallImageIds[1]);
    GifToolsModule._objectFree(smallImageIds[2]);
    GifToolsModule._objectFree(smallImageIds[3]);
    GifToolsModule._objectFree(gifBufferId);
    GifToolsModule._objectFree(gifBuilderId);
}

GifToolsModule.onRuntimeInitialized = function() { main(); }
