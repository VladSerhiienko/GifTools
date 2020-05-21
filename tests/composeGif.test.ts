import { GifTools, GifToolsVideoFrame } from '../src';
import * as fs from 'fs';
import * as path from 'path';

const testImages = [
    "data:image/jpeg;base64,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",
    "data:image/jpeg;base64,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",
]

var BASE64_MARKER = ';base64,';

function convertDataURIToBinary(dataURI: string): Uint8Array {
    var base64Index = dataURI.indexOf(BASE64_MARKER) + BASE64_MARKER.length;
    var base64 = dataURI.substring(base64Index);
    var raw = window.atob(base64);
    var rawLength = raw.length;
    var array = new Uint8Array(new ArrayBuffer(rawLength));

    for (let i = 0; i < rawLength; i++) {
        array[i] = raw.charCodeAt(i);
    }
    return array;
}

function Uint8ToBase64(u8Arr: Uint8Array): string {
    var CHUNK_SIZE = 0x8000; //arbitrary number
    var index = 0;
    var length = u8Arr.length;
    var result = '';
    var slice: Uint8Array;
    while (index < length) {
        slice = u8Arr.subarray(index, Math.min(index + CHUNK_SIZE, length));
        result += String.fromCharCode.apply(null, slice);
        index += CHUNK_SIZE;
    }
    return btoa(result);
}

describe('GifTools', () => {
    var actualResultsDirPath = "./tests/bin/results_actual";
    if (fs.existsSync(actualResultsDirPath)){
        fs.rmdirSync(actualResultsDirPath, { recursive: true });
    }

    fs.mkdirSync(actualResultsDirPath);

    test('Create GIF from base64', done => {
        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            if (!succeeded) { done(); }
            const delay = 100;
            const width = 300;
            const height = 200;

            expect(gifTools.gifEncoderBegin(width, height, delay)).toBeTruthy();

            testImages.forEach(
                image => {
                    var fileBuffer = convertDataURIToBinary(image);
                    expect(fileBuffer).toBeTruthy();
                    var loadedImgId = gifTools.imageLoadFromFileBuffer(fileBuffer);
                    expect(loadedImgId).toBeTruthy();
                    var resizedImgId = gifTools.imageResize(loadedImgId, width, height);
                    expect(resizedImgId).toBeTruthy();
                    expect(gifTools.gifEncoderAddImage(resizedImgId, delay)).toBeTruthy();

                    gifTools.internalFreeObjIds(resizedImgId);
                    gifTools.internalFreeObjIds(loadedImgId);
                }
            );

            const gifBuffer = gifTools.gifEncoderEnd();
            fs.writeFileSync(actualResultsDirPath + "/dump_animation_0.gif", gifBuffer);
            
            gifTools.deinit();
            expect(gifBuffer).toBeTruthy();

            // var b64encoded = Uint8ToBase64(gifBuffer);
            // expect(b64encoded).toMatchSnapshot();
            done();
        });
    });

    test('Create GIF from images', done => {
        var imgBuffers : Uint8Array[] = [fs.readFileSync("./tests/bin/image/IMG_20191217_083053.jpg"),
                                         fs.readFileSync("./tests/bin/image/IMG_20191217_083055.jpg"),
                                         fs.readFileSync("./tests/bin/image/IMG_20191217_083056.jpg"),
                                         fs.readFileSync("./tests/bin/image/IMG_20191217_083058.jpg"),
                                         fs.readFileSync("./tests/bin/image/IMG_20191217_083059.jpg"),
                                         fs.readFileSync("./tests/bin/image/IMG_20191217_083101.jpg")];

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            if (!succeeded) { done(); }
            const delay = 100;
            const width = 1200;
            const height = 900;

            expect(gifTools.gifEncoderBegin(width, height, delay)).toBeTruthy();

            imgBuffers.forEach(
                (fileBuffer, i) => {
                    expect(fileBuffer).toBeTruthy();
                    var loadedImgId = gifTools.imageLoadFromFileBuffer(fileBuffer);
                    console.log("loadedImgId", loadedImgId);
                    expect(loadedImgId).toBeTruthy();

                    var resizedImgId = gifTools.imageResize(loadedImgId, width, height);
                    expect(resizedImgId).toBeTruthy();
                    console.log("resizedImgId", resizedImgId);

                    var pngBufferId = gifTools.vm.imageExportToPngFileMemory(resizedImgId);
                    var pngArrayBuffer = gifTools.vm.bufferToUint8Array(pngBufferId);
                    fs.writeFileSync(actualResultsDirPath + "/dump_resized_image_" + i  + ".png", pngArrayBuffer);

                    expect(gifTools.gifEncoderAddImage(resizedImgId, delay)).toBeTruthy();

                    gifTools.vm.objectFree(pngBufferId);
                    gifTools.internalFreeObjIds(resizedImgId);
                    gifTools.internalFreeObjIds(loadedImgId);
                }
            );

            const gifBuffer = gifTools.gifEncoderEnd();
            fs.writeFileSync(actualResultsDirPath + "/dump_animation_1.gif", gifBuffer);

            gifTools.deinit();
            expect(gifBuffer).toBeTruthy();

            // var b64encoded = Uint8ToBase64(gifBuffer);
            // expect(b64encoded).toMatchSnapshot();
            done();
        });
    });

    test('Decode MP4 into images', done => {
        var videoBuffers : Uint8Array[] = [fs.readFileSync("./tests/bin/video/VID_20200503_154756.mp4")];

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            if (!succeeded) { done(); }

            console.log(gifTools);

            videoBuffers.forEach(
                videoBuffer => {
                    expect(videoBuffer).toBeTruthy();
                    console.log("videoBuffer.byteLength", videoBuffer.byteLength); 

                    expect(gifTools.videoDecoderOpenVideoStream(videoBuffer)).toBeTruthy();
                    var frames: (GifToolsVideoFrame | null)[] = [];

                    for (var i = 0; i < 27; ++i) {
                        frames[i] = gifTools.videoDecoderPickClosestVideoFrame(i);
                        if (frames[i] == null) { break; }

                        var pngBufferId = gifTools.vm.imageExportToPngFileMemory(frames[i]!.imageId);
                        var pngArrayBuffer = gifTools.vm.bufferToUint8Array(pngBufferId);
                        fs.writeFileSync(actualResultsDirPath + "/dump_image_" + i  + ".png", pngArrayBuffer);
                    }

                    for (var i = 0; i < 27; ++i) {
                        if (frames[i] == null) { break; }
                        gifTools.videoDecoderFreeVideoFrame(frames[i]);
                    }
                }
            );

            gifTools.deinit();
            done();
        });
    });
})
