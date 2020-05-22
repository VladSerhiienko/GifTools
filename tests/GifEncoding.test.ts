import * as fs from 'fs';
import {GifTools, GifToolsVideoFrame} from '../src';

describe('GifTools', () => {
    var actualResultsDirPath = './tests/bin/results_actual_ts';

    // if (fs.existsSync(actualResultsDirPath)) { fs.rmdirSync(actualResultsDirPath, {recursive: true}); }
    fs.mkdirSync(actualResultsDirPath, {recursive: true});

    var baseGifToolsGifTest = function(done: any, width: number, height: number, id: string) {
        const delay = 30;

        var imgBuffers: Uint8Array[] = [
            fs.readFileSync('./tests/bin/image/IMG_20191217_083053.jpg'),
            fs.readFileSync('./tests/bin/image/IMG_20191217_083055.jpg'),
            fs.readFileSync('./tests/bin/image/IMG_20191217_083056.jpg'),
            fs.readFileSync('./tests/bin/image/IMG_20191217_083058.jpg'),
            fs.readFileSync('./tests/bin/image/IMG_20191217_083059.jpg'),
            fs.readFileSync('./tests/bin/image/IMG_20191217_083101.jpg')
        ];

        var imgIndices = [0, 1, 2, 3, 4, 5, 4, 3, 2, 1];

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            expect(succeeded).toBeTruthy();
            if (!succeeded) { done(); }

            expect(gifTools.module()).toBeTruthy();
            expect(gifTools.gifEncoderBegin(width, height, delay)).toBeTruthy();

            imgIndices.forEach((fileIndex, i) => {
                var fileBuffer = imgBuffers[fileIndex];
                expect(fileBuffer).toBeTruthy();

                var loadedImgId = gifTools.imageLoadFromFileBuffer(fileBuffer);
                console.log('loadedImgId', loadedImgId);
                expect(loadedImgId).toBeTruthy();

                var resizedImgId = gifTools.imageResize(loadedImgId, width, height);
                expect(resizedImgId).toBeTruthy();
                console.log('resizedImgId', resizedImgId);

                var pngBufferId = gifTools.vm.imageExportToPngFileMemory(resizedImgId);
                var pngArrayBuffer = gifTools.vm.bufferToUint8Array(pngBufferId);
                fs.writeFileSync(actualResultsDirPath + '/dump_gif_' + id + '_resized_image_' + i + '.png', pngArrayBuffer);

                expect(gifTools.gifEncoderAddImage(resizedImgId, delay)).toBeTruthy();

                gifTools.vm.objectFree(pngBufferId);
                gifTools.internalFreeObjIds(resizedImgId);
                gifTools.internalFreeObjIds(loadedImgId);
            });

            const gifBuffer = gifTools.gifEncoderEnd();
            expect(gifBuffer).toBeTruthy();

            fs.writeFileSync(actualResultsDirPath + '/dump_' + id + '.gif', gifBuffer);

            gifTools.deinit();
            done();
        });
    }; // testGifEncoder()

    test('GIF-360p', done => {
        const width = 640;
        const height = 360;
        baseGifToolsGifTest(done, width, height, '360p');
    });

    test('GIF-720p', done => {
        const width = 1280;
        const height = 720;
        baseGifToolsGifTest(done, width, height, '720p');
    });

    test('GIF-FHD', done => {
        const width = 1920;
        const height = 1080;
        baseGifToolsGifTest(done, width, height, 'fhd');
    });

    test('GIF-UHD', done => {
        const width = 4608;
        const height = 3456;
        baseGifToolsGifTest(done, width, height, 'uhd');
    });

    test('GIF-MP4-360p', done => {
        var videoBuffers: Uint8Array[] = [fs.readFileSync('./tests/bin/video/VID_20200503_154756_L.mp4')];

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            if (!succeeded) { done(); }
            const delay = 100;
            const width = 640;
            const height = 360;

            expect(gifTools.module()).toBeTruthy();
            expect(gifTools.gifEncoderBegin(width, height, delay)).toBeTruthy();

            videoBuffers.forEach(videoBuffer => {
                expect(videoBuffer).toBeTruthy();
                console.log('videoBuffer.byteLength', videoBuffer.byteLength);

                expect(gifTools.videoDecoderOpenVideoStream(videoBuffer)).toBeTruthy();
                var frames: (GifToolsVideoFrame|null)[] = [];

                for (var i = 0; i < 27; ++i) {
                    frames[i] = gifTools.videoDecoderPickClosestVideoFrame(i);
                    if (frames[i] == null) { break; }

                    var pngBufferId = gifTools.vm.imageExportToPngFileMemory(frames[i]!.imageId);
                    var pngArrayBuffer = gifTools.vm.bufferToUint8Array(pngBufferId);
                    fs.writeFileSync(actualResultsDirPath + '/dump_mp4_360p_image_' + i + '.png', pngArrayBuffer);
                    pngArrayBuffer = null;

                    expect(gifTools.gifEncoderAddImage(frames[i]!.imageId, delay)).toBeTruthy();

                    gifTools.vm.objectFree(pngBufferId);
                    gifTools.internalFreeObjIds(pngBufferId);
                    gifTools.videoDecoderFreeVideoFrame(frames[i]);
                }
            });

            gifTools.deinit();
            done();
        });
    });
})
