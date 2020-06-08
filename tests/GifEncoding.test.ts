import * as fs from 'fs';
import * as path from 'path';
import { GifTools, GifToolsVideoFrame } from '../src/GifTools';

function areEqual(a: Uint8Array, b: Uint8Array): boolean {
    if (a.byteLength !== b.byteLength) return false;
    return a.every((val, i) => val === b[i]) ? true : false;
}

enum GifToolsPrepareFramesType {
    GifToolsDoNotPrepareFrames,
    GifToolsPrepareAllFrames,
    GifToolsPrepareFrames,
}   

describe('GifTools', () => {
    // Enables all tests, it takes too long to run them all.
    // const testAll = false;

    // Debugging stuff.
    const dumpResizedImgs = false;

    // Common strings.
    const videosDirPath = './tests/bin/video';
    const imagesDirPath = './tests/bin/image';
    const actualResultsDirPath = './tests/bin/results_actual_ts';
    const expectedResultsDirPath = './tests/bin/results_expected';

    // TODO(vserhiienko): Only empty directories can be removed, wtf.
    // if (fs.existsSync(actualResultsDirPath)) { fs.rmdirSync(actualResultsDirPath, {recursive: true}); }
    
    fs.mkdirSync(actualResultsDirPath, { recursive: true });

    const baseGifToolsGifTest = function (done: any, width: number, height: number, id: string) {
        const delay = 30;

        const imgBuffers: Uint8Array[] = [
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083053.jpg'),
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083055.jpg'),
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083056.jpg'),
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083058.jpg'),
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083059.jpg'),
            fs.readFileSync(imagesDirPath + '/IMG_20191217_083101.jpg')
        ];

        const imgIndices = [0, 1, 2, 3, 4, 5, 4, 3, 2, 1];

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            expect(succeeded).toBeTruthy();
            if (!succeeded) { done(); }

            expect(gifTools.module()).toBeTruthy();
            expect(gifTools.gifEncoderBegin(width, height, delay)).toBeTruthy();

            imgIndices.forEach((fileIndex, i) => {
                const fileBuffer = imgBuffers[fileIndex];
                expect(fileBuffer).toBeTruthy();

                const loadedImgId = gifTools.imageLoadFromFileBuffer(fileBuffer);
                expect(loadedImgId).toBeTruthy();

                const resizedImgId = gifTools.imageResize(loadedImgId, width, height);
                expect(resizedImgId).toBeTruthy();

                // console.log('loadedImgId', loadedImgId);
                // console.log('resizedImgId', resizedImgId);

                if (dumpResizedImgs) {
                    const pngBufferId = gifTools.vm.imageExportToPngFileMemory(resizedImgId);
                    const pngArrayBuffer = gifTools.vm.bufferToUint8Array(pngBufferId);
                    fs.writeFileSync(actualResultsDirPath + '/dump_gif_' + id + '_resized_image_' + i + '.png', pngArrayBuffer);
                    gifTools.vm.objectFree(pngBufferId);
                }

                expect(gifTools.gifEncoderAddImage(resizedImgId, delay)).toBeTruthy();

                gifTools.internalFreeObjIds(resizedImgId);
                gifTools.internalFreeObjIds(loadedImgId);
            });

            const gifBuffer = gifTools.gifEncoderEnd();
            expect(gifBuffer).toBeTruthy();

            const actualResultPath = actualResultsDirPath + '/dump_' + id + '.gif';
            const expectedResultPath = expectedResultsDirPath + '/dump_' + id + '.gif';

            fs.writeFileSync(actualResultPath, gifBuffer, { encoding: 'binary' });

            const expectedResult: Uint8Array = fs.readFileSync(expectedResultPath);
            expect(expectedResult).toBeTruthy();
            expect(areEqual(gifBuffer, expectedResult)).toBe(true);

            gifTools.deinit();
            done();
        });
    };

    test('GIF-360p', done => {
        const width = 640;
        const height = 360;
        baseGifToolsGifTest(done, width, height, '360p');
    });

    // if (testAll) {
    //     test('GIF-720p', done => {
    //         const width = 1280;
    //         const height = 720;
    //         baseGifToolsGifTest(done, width, height, '720p');
    //     });

    //     test('GIF-FHD', done => {
    //         const width = 1920;
    //         const height = 1080;
    //         baseGifToolsGifTest(done, width, height, 'fhd');
    //     });

    //     test('GIF-UHD', done => {
    //         const width = 4608;
    //         const height = 3456;
    //         baseGifToolsGifTest(done, width, height, 'uhd');
    //     });
    // }

    const baseGifToolsFFmpegTest = function (done: any, width: number, height: number, targetResolutionId: string, videoFilePath: string, videoResolutionId: string, prepareFramesType: GifToolsPrepareFramesType, framesPerSecond: number) {
        const delay = 30;

        const videoBuffer: Uint8Array = fs.readFileSync(videosDirPath + '/' + videoFilePath);
        expect(videoBuffer).toBeTruthy();

        const gifTools = new GifTools();
        gifTools.init().then((succeeded: boolean) => {
            if (!succeeded) { done(); }

            expect(gifTools.module()).toBeTruthy();
            expect(gifTools.videoDecoderOpenVideoStream(videoBuffer)).toBeTruthy();

            const targetWidth = width ? width :  gifTools.videoDecoderWidth();
            const targetHeight = height ? height :  gifTools.videoDecoderHeight();
            expect(targetWidth).toBeTruthy();
            expect(targetHeight).toBeTruthy();

            if (prepareFramesType == GifToolsPrepareFramesType.GifToolsPrepareAllFrames) {
                expect(gifTools.videoDecoderPrepareAllFrames()).toBeTruthy();
            } else if (prepareFramesType == GifToolsPrepareFramesType.GifToolsPrepareFrames) {
                let offset = 0;
                let duration = gifTools.videoDecoderDurationSeconds();
                expect(gifTools.videoDecoderPrepareFrames(framesPerSecond, offset, duration)).toBeTruthy();
            }

            expect(gifTools.gifEncoderBegin(targetWidth, targetHeight, delay)).toBeTruthy();

            const duration = gifTools.videoDecoderDurationSeconds();
            for (var t = 0.0; t < duration; t += (1.0 / framesPerSecond)) {
                const frame = prepareFramesType == GifToolsPrepareFramesType.GifToolsDoNotPrepareFrames
                    ? gifTools.videoDecoderPickClosestVideoFrame(t)
                    : gifTools.videoDecoderPickClosestPreparedVideoFrame(t); 

                expect(frame).toBeTruthy();
                expect(frame!.imageId).toBeTruthy();
                
                const resizedImg = gifTools.imageResize(frame!.imageId, targetWidth, targetHeight);
                expect(resizedImg).toBeTruthy();

                expect(gifTools.gifEncoderAddImage(resizedImg, delay)).toBeTruthy();

                if (frame!.imageId != resizedImg) { gifTools.internalFreeObjIds(resizedImg); }
                gifTools.videoDecoderFreeVideoFrame(frame);
            }

            const gifBuffer = gifTools.gifEncoderEnd();
            expect(gifBuffer).toBeTruthy();

            const prefix = "dump_";
            const fileName = path.basename(videoFilePath, path.extname(videoFilePath));
            const id = videoResolutionId + "_" + targetResolutionId;
            const resultName = prefix + fileName + "_" + id + ".gif";
            const actualResultPath = actualResultsDirPath + '/' + resultName;
            const expectedResultPath = expectedResultsDirPath + '/' + resultName;

            fs.writeFileSync(actualResultPath, gifBuffer, { encoding: 'binary' });

            if (fs.existsSync(expectedResultPath)) {
                const expectedResult: Uint8Array = fs.readFileSync(expectedResultPath);
                expect(expectedResult).toBeTruthy();
                expect(areEqual(gifBuffer, expectedResult)).toBe(true);
            } else {
                console.warn("Test/GifTools/FFmpeg: No expected result!");
            }

            gifTools.deinit();
            done();
        });
    };

    test('GIF-FFMPEG-FHD-360P-RATE-MOV', done => {
        const width = 640;
        const height = 360;
        baseGifToolsFFmpegTest(done, width, height, '360p', 'IMG_2041.MOV', '360p_rate', GifToolsPrepareFramesType.GifToolsPrepareFrames, 1.0);
    });

    test('GIF-FFMPEG-360P-RATE', done => {
        const width = 360;
        const height = 640;
        baseGifToolsFFmpegTest(done, width, height, '360p', 'roborock.mp4', '360p_rate', GifToolsPrepareFramesType.GifToolsPrepareFrames, 1.0);
    });

    test('GIF-FFMPEG-360P', done => {
        const width = 0;
        const height = 0;
        baseGifToolsFFmpegTest(done, width, height, 'default', 'VID_20200503_154756_360P.mp4', '360p', GifToolsPrepareFramesType.GifToolsDoNotPrepareFrames, 1.0);
    });

    test('GIF-FFMPEG-FHD-360P-RATE', done => {
        const width = 640;
        const height = 360;
        baseGifToolsFFmpegTest(done, width, height, '360p', 'VID_20200521_193627_FHD.mp4', '360p_rate', GifToolsPrepareFramesType.GifToolsPrepareFrames, 1.0);
    });

    test('GIF-FFMPEG-UHD-360P-RATE', done => {
        const width = 640;
        const height = 360;
        baseGifToolsFFmpegTest(done, width, height, '360p', 'VID_20200521_193627_UHD.mp4', '360p_rate', GifToolsPrepareFramesType.GifToolsPrepareFrames, 1.0);
    });
});
