import { GifTools, GifToolsVideoFrame } from '../src';
import * as fs from 'fs';

describe('GifTools', () => {
    var actualResultsDirPath = "./tests/bin/results_actual";
    if (fs.existsSync(actualResultsDirPath)){
        fs.rmdirSync(actualResultsDirPath, { recursive: true });
    }

    fs.mkdirSync(actualResultsDirPath);

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
