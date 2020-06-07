import * as GifToolsFactory from '../bin/web_amalgamated_ffmpeg_worker/GifTools'; // @ts-ignore

//
// Subscribe to messages from client.
//

const worker: Worker = self as any;
worker.addEventListener("message", (messageEvent) => {
    GifToolsWorker.get().receiveMessage(messageEvent);
});

class GifToolsValidationError extends Error {
    constructor(message: string) {
      super(message); 
      this.name = "GifToolsValidationError";
    }
}

class GifToolsVideoFrame {
    frameId : number;
    imageId : number;
    timeSeconds : number;
};

/**
 * GifTools class wraps GifTools WebAssembly module.
 * It mostly operates object ids (numbers), rather than actual objects.
 * All previously allocated objects must be manually freed.
 * This class takes care about memory, although it's up to users to deinit it.
 */
class GifTools {
    static invalidObjId : number = 0;
    static isValidObj(objId : number) : boolean { return objId > 0; }

    private static vmStatusNull: string = "null";
    private static vmStatusPending: string = "pending";
    private static vmStatusFulfilled: string = "fullfilled";

    private static assert(cond: boolean, msg: string) {
        if (!cond) { debugger; throw new GifToolsValidationError(msg); }
    }

    vm: any;
    private vmStatus: string = GifTools.vmStatusNull;
    private vmObjIds: Array<number> = [];
    
    private currentGifBuilderId: number = 0;
    private currentGifBuilderWidth: number = 0;
    private currentGifBuilderHeight: number = 0;

    private currentInputStreamBufferId: number = 0;
    private currentInputStreamId: number = 0;
    private currentVideoStreamId: number = 0;

    constructor() {
    }

    /**
     * Initializes this instance.
     */
    init(vm: any) {
        this.vm = vm;
        this.vmStatus = GifTools.vmStatusFulfilled;
    }

    /**
     * Finalizes this instance, frees all dangling objects.
     * Make sure all encoders are done.
     */
    deinit() {
        GifTools.assert(!GifTools.isValidObj(this.currentGifBuilderId), "Caught active GIF encoder.");
        this.internalFreeObjIds();
        this.vmStatus = "null";
        this.vm = null;
    }

    module() : any {
        return this.vm;
    }

    /**
     * Appends object ids to the free list.
     * @param objIds Object ids (numbers).
     */
    internalAddObjIds(...objIds : number[]) {
        if (objIds == undefined || objIds == null) { return; }
        objIds.forEach(objId => {
            if (this.vmObjIds.indexOf(objId) < 0) {
                this.vmObjIds.push(objId);
            }
        });
    }

    /**
     * Removes object ids from the free list and frees associated memory.
     * If object ids are not provided, previously added objects are freed.
     * @param objIds Optional object ids (numbers).
     */
    internalFreeObjIds(...objIds : number[]) {
        if (objIds == undefined || objIds == null) {
            this.vmObjIds.forEach(objId => {
                if (!GifTools.isValidObj(objId)) { return; }
                this.vm.objectFree(objId);
            });
            this.vmObjIds = [];
        } else {
            objIds.forEach(objId => {
                if (!GifTools.isValidObj(objId)) { return; }
                const objIndex = this.vmObjIds.indexOf(objId);
                if (objIndex >= 0) { this.vmObjIds.splice(objIndex, 1); }
                this.vm.objectFree(objId);
            });
        }
    }

    bufferToBase64(bufferId: number) : string {
        if (!GifTools.isValidObj(bufferId)) { return ""; }
        var base64BufferId = this.vm.bufferToBase64(bufferId);
        if (!GifTools.isValidObj(base64BufferId)) { return ""; }
        this.internalAddObjIds(base64BufferId);
        var bufferArray = this.vm.bufferToUint8Array(base64BufferId);
        var textDecoder = new TextDecoder("utf-8");
        var base64String = textDecoder.decode(bufferArray);
        this.internalFreeObjIds(base64BufferId);
        return base64String;
    }

    /**
     * Loads provided file buffer into an image object, returns its id.
     * @param fileBuffer File memory stored in Uint8Array instance.
     */
    imageLoadFromFileBuffer(fileBuffer : Uint8Array) : number {
        if (fileBuffer == null || fileBuffer == undefined) {
            return GifTools.invalidObjId;
        }

        const bufferId = this.vm.bufferFromUint8Array(fileBuffer);
        if (!GifTools.isValidObj(bufferId)) { return GifTools.invalidObjId; }

        this.internalAddObjIds(bufferId);
        const imageId = this.vm.imageLoadFromBuffer(bufferId);
        if (!GifTools.isValidObj(bufferId)) {
            this.internalFreeObjIds(bufferId);
            return GifTools.invalidObjId;
        }

        this.internalAddObjIds(imageId);
        return imageId;
    }

    /**
     * Resizes provided image, or returns the same image id in case extents match.
     * @param imageId Image object id.
     * @param width Desired image width.
     * @param height Desired image height.
     */
    imageResize(imageId : number, width: number, height: number) : number {
        if ([imageId, width, height].some(n => n <= 0)) { return imageId };

        if (width == this.vm.imageWidth(imageId) && height == this.vm.imageHeight(imageId)) { return imageId; }

        const resizedImageId = this.vm.imageResizeOrClone(imageId, width, height);
        if (!GifTools.isValidObj(resizedImageId)) { return imageId; }

        this.internalAddObjIds(resizedImageId);
        return resizedImageId;
    }

    videoDecoderOpenVideoStream(fileBuffer : Uint8Array) : boolean {
        if (fileBuffer == null || fileBuffer == undefined) { return false; }

        this.currentInputStreamBufferId = this.vm.bufferFromUint8Array(fileBuffer);
        if (!GifTools.isValidObj(this.currentInputStreamBufferId)) { return false; }

        this.internalAddObjIds(this.currentInputStreamBufferId);

        this.currentInputStreamId = this.vm.ffmpegInputStreamLoadFromBuffer(this.currentInputStreamBufferId);
        if (!GifTools.isValidObj(this.currentInputStreamBufferId)) {
            this.internalFreeObjIds(this.currentInputStreamBufferId);
            return false;
        }

        this.internalAddObjIds(this.currentInputStreamId);

        try {
            this.currentVideoStreamId = this.vm.ffmpegVideoStreamOpen(this.currentInputStreamId);
            if (GifTools.isValidObj(this.currentVideoStreamId)) { return true; }
        } catch (e) {
            console.log(e);
        }

        this.internalFreeObjIds(this.currentInputStreamBufferId);
        this.internalFreeObjIds(this.currentInputStreamId);
        return false;
    }

    videoDecoderWidth() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamWidth(this.currentVideoStreamId);
    }

    videoDecoderHeight() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamHeight(this.currentVideoStreamId);
    }

    videoDecoderFrameCount() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamFrameCount(this.currentVideoStreamId);
    }

    videoDecoderDurationSeconds() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamDurationSeconds(this.currentVideoStreamId);
    }

    videoDecoderFrameDurationSeconds() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamFrameDurationSeconds(this.currentVideoStreamId);
    }

    videoDecoderPrepareAllFrames() : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamPrepareAllFrames(this.currentVideoStreamId);
    }

    videoDecoderPrepareFrames(framesPerSecond: number, offsetSeconds: number, durationSeconds: number) : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamPrepareFrames(this.currentVideoStreamId, framesPerSecond, offsetSeconds, durationSeconds);
    }

    videoDecoderPickClosestPreparedVideoFrame(durationSeconds: number): GifToolsVideoFrame | null {
        GifTools.assert(GifTools.isValidObj(this.currentVideoStreamId), "Caught null video stream.");

        var frameId = this.vm.ffmpegVideoStreamPickBestPreparedFrame(this.currentVideoStreamId, durationSeconds);
        if (!GifTools.isValidObj(frameId)) { return null; }

        this.internalAddObjIds(frameId);

        var frame = new GifToolsVideoFrame();
        frame.frameId = frameId;
        frame.imageId = this.vm.ffmpegVideoFrameImage(frameId);
        frame.timeSeconds = this.vm.ffmpegVideoFrameTimeSeconds(frameId);
        return frame;
    }

    videoDecoderPickClosestVideoFrame(durationSeconds: number): GifToolsVideoFrame | null {
        GifTools.assert(GifTools.isValidObj(this.currentVideoStreamId), "Caught null video stream.");

        var frameId = this.vm.ffmpegVideoStreamPickBestFrame(this.currentVideoStreamId, durationSeconds);
        if (!GifTools.isValidObj(frameId)) { return null; }

        this.internalAddObjIds(frameId);

        var frame = new GifToolsVideoFrame();
        frame.frameId = frameId;
        frame.imageId = this.vm.ffmpegVideoFrameImage(frameId);
        frame.timeSeconds = this.vm.ffmpegVideoFrameTimeSeconds(frameId);
        return frame;
    }

    videoDecoderFreeVideoFrame(frame: (GifToolsVideoFrame | null)) {
        if (frame == null) { return; }

        GifTools.assert(GifTools.isValidObj(this.currentVideoStreamId), "Caught null video stream.");
        this.internalFreeObjIds(frame!.frameId);
    }

    videoDecoderCloseVideoStream() {
        GifTools.assert(GifTools.isValidObj(this.currentVideoStreamId), "Caught null video stream.");
        this.internalFreeObjIds(this.currentInputStreamId, this.currentVideoStreamId);
    }

    /**
     * Starts GIF encoding.
     * Make sure to call @function gifEncoderEnd() after GIF encoding is done.
     * @param width Desired GIF width.
     * @param height Desired GIF height.
     * @param delay Desired GIF delay between loops in ms, or 0 to a single loop.
     */
    gifEncoderBegin(width: number, height: number, delay: number) : boolean {
        GifTools.assert(!GifTools.isValidObj(this.currentGifBuilderId), "Caught existing GIF encoder.");
        this.currentGifBuilderId = this.vm.gifBuilderInitialize(width, height, delay);
        if (!GifTools.isValidObj(this.currentGifBuilderId)) { return false; }
        this.internalAddObjIds(this.currentGifBuilderId);
        GifTools.assert(width > 0, "Caught invalid extent.");
        GifTools.assert(height > 0, "Caught invalid extent.");
        this.currentGifBuilderWidth = width;
        this.currentGifBuilderHeight = height;
        return true;
    }

    /**
     * Appends image to GIF.
     * Image is not needed further and can be freed after this functions returns.
     * @param imageId Image object id.
     * @param delay Desired GIF delay between frames in ms.
     */
    gifEncoderAddImage(imageId : number, delay : number) : boolean {
        GifTools.assert(GifTools.isValidObj(this.currentGifBuilderId), "Caught null GIF encoder.");
        GifTools.assert(GifTools.isValidObj(imageId), "Caught null image.");
        GifTools.assert(this.vm.imageWidth(imageId) == this.currentGifBuilderWidth, "Caught invalid image.");
        GifTools.assert(this.vm.imageHeight(imageId) == this.currentGifBuilderHeight, "Caught invalid image.");
        return this.vm.gifBuilderAddImage(this.currentGifBuilderId, imageId, delay);
    }

    /**
     * Finalizes GIF encoding.
     * Returns @class Uint8Array GIF file buffer.
     */
    gifEncoderEnd() : Uint8Array {
        GifTools.assert(GifTools.isValidObj(this.currentGifBuilderId), "Caught null GIF encoder.");
        var gifBufferId = this.vm.gifBuilderFinalize(this.currentGifBuilderId);
        this.internalAddObjIds(gifBufferId);
        // this.internalFreeObjIds(gifBufferId);

        var gifBufferArray = this.vm.bufferToUint8Array(gifBufferId);

        this.internalFreeObjIds(this.currentGifBuilderId);
        this.currentGifBuilderId = GifTools.invalidObjId;
        this.currentGifBuilderWidth = 0;
        this.currentGifBuilderHeight = 0;
        return gifBufferArray;
    }
}

//
// Process messages from client and send responses.
//

class GifToolsWorker {
    private static instance = new GifToolsWorker();
    public static get(): GifToolsWorker { return GifToolsWorker.instance; }
    
    worker: Worker = worker;
    gifTools = new GifTools();
    cancellationToken: Uint32Array;
    lastMsgId = -1;

    postMessage(message: any, transferable?:Transferable[]) {
        console.log('GifToolsWorker.postMessage: message=', message, ', transferable=', transferable);
        
        if (transferable) { return this.worker.postMessage(message, transferable); }
        return this.worker.postMessage(message);
    }

    shouldCancel() : boolean {
        return this.cancellationToken ? this.cancellationToken[0] == 1 : false;
    }

    onReportedProgress(progress: number) {
        // console.log('GifToolsWorker.onReportedProgress: progress=', progress);
        this.postMessage({msgType : 'MSG_TYPE_REPORT_PROGRESS', msgId: this.lastMsgId, progress: progress});
    }

    toBase64(arr: Uint8Array) : string {
        return this.gifTools.vm.uint8ArrayToBase64String(arr);
    }

    run(width: number,
        height: number,
        startTimeSeconds: number,
        endTimeSeconds: number,
        framesPerSecond: number,
        frameDelaySeconds: number,
        loop: boolean,
        boomerang: boolean) : (string|null) {
    
        if (!this.gifTools) { return null; }

        if (width <= 0) { return null; }
        if (height <= 0) { return null; }
        if (framesPerSecond <= 0) { return null; }
        if (frameDelaySeconds <= 0) { return null; }
        if (frameDelaySeconds <= 0) { return null; }
        if (startTimeSeconds === endTimeSeconds) { return null; }

        const totalDuration = this.gifTools.videoDecoderDurationSeconds();

        if (startTimeSeconds < 0) { return null; }
        if (startTimeSeconds >= totalDuration) { return null; }
        if (endTimeSeconds < 0) { return null; }
        if (endTimeSeconds > totalDuration) { return null; }

        this.onReportedProgress(0);

        if (!this.gifTools.module().progressTokenSetProgressReporter({reportProgress: (progress: number) => this.onReportedProgress(progress * 0.4) })) {
            console.log('GifToolsAsync.receiveMessage: failed to set progress reporter.');
        }

        const shouldResize = width != this.gifTools.videoDecoderWidth() || height != this.gifTools.videoDecoderHeight();

        if (!this.gifTools.videoDecoderPrepareFrames(framesPerSecond, startTimeSeconds, endTimeSeconds - startTimeSeconds)) {
            return null;
        }

        const imgIds: number[] = [];
        const dt = 1.0 / framesPerSecond;
        const duration = endTimeSeconds - startTimeSeconds;
        const frames = duration / dt;

        let dp = 1 / frames * 0.3;
        let p = 0.4;

        if (!this.gifTools.module().progressTokenSetProgressReporter({reportProgress: (progress: number) => { /* noop */ } })) {
            console.log('GifToolsAsync.receiveMessage: failed to set progress reporter.');
        }

        for (var t = startTimeSeconds; t <= endTimeSeconds; t += dt) {
            console.log('cycle, t=', t);

            const frame = this.gifTools.videoDecoderPickClosestPreparedVideoFrame(t); 
            if (!frame) { return null; }
            if (!frame!.imageId) { return null; }

            if (shouldResize) {
                const resizedImg = this.gifTools.imageResize(frame!.imageId, width, height);
                if (!resizedImg) { return null; }
                imgIds.push(resizedImg);
            } else {
                const clonedImg = this.gifTools.vm.imageClone(frame!.imageId);
                if (!clonedImg) { return null; }
                imgIds.push(clonedImg);
            }
            
            this.gifTools.videoDecoderFreeVideoFrame(frame);

            p += dp;
            this.onReportedProgress(p);
        }

        console.log('imgIds=', imgIds);

        const imgIndices: number[] = [];
        for (let i = 0; i < imgIds.length; ++i) {
            imgIndices.push(i);
        }

        console.log('imgIndices=', imgIndices);

        if (boomerang) {
            for (let i = 1; i < imgIds.length; ++i) {
                imgIndices.push(imgIds.length - i - 1);
            }

            console.log('imgIndices=', imgIndices);
        }

        console.log('gif begin');
        if (!this.gifTools.gifEncoderBegin(width, height, loop ? frameDelaySeconds * 100 : 0)) {
            return null;
        }

        dp = 1.0 / imgIndices.length * 0.25;
        p = 0.7;
        
        for (let i = 0; i < imgIndices.length; ++i) {
            console.log('gif, i=', i, ', img=', imgIds[imgIndices[i]]);
            this.gifTools.gifEncoderAddImage(imgIds[imgIndices[i]], frameDelaySeconds * 100);

            p += dp;
            this.onReportedProgress(p);
        }

        console.log('free imgs');
        for (let i = 0; i < imgIds.length; ++i) {
            this.gifTools.internalFreeObjIds(imgIds[i]);
        }

        const gifBuffer = this.gifTools.gifEncoderEnd();
        if (!gifBuffer) { this.onReportedProgress(1); return ""; }

        const gifBase64 = this.toBase64(gifBuffer);
        return gifBase64;
    }

    receiveMessage(messageEvent: MessageEvent) {
        let payload = messageEvent.data;

        console.log('GifToolsWorker.receiveMessage: payload=', payload);

        if(!payload.hasOwnProperty('msgType')) { return; }
        if(!payload.hasOwnProperty('msgId')) { return; }

        let msgType = payload.msgType;
        let msgId = payload.msgId;

        this.lastMsgId = msgId;

        console.log('GifToolsAsync.receiveMessage: msgType=', msgType);
        console.log('GifToolsAsync.receiveMessage: msgId=', msgId);

        if (msgType === 'MSG_TYPE_SET_VM') {
            this.cancellationToken = payload.cancellationToken as Uint32Array;
            console.log('GifToolsAsync.receiveMessage: this.cancellationToken=', this.cancellationToken);

            let rejectGifTools = () => {
                this.gifTools.init(null);
                this.postMessage({msgType : 'MSG_TYPE_SET_VM_FAILED', msgId: msgId});
            };

            GifToolsFactory().then((vm: any) => {
                console.log('GifToolsAsync.receiveMessage: vm=', vm);
                if (!vm) { rejectGifTools(); return; }

                if (!vm.progressTokenSetProgressReporter({reportProgress: (progress: number) => { /* noop */ } })) {
                    console.log('GifToolsAsync.receiveMessage: failed to set progress reporter.');
                }
                if (!vm.cancellationTokenSetCancellationSource({shouldCancel: () => this.shouldCancel() })) {
                    console.log('GifToolsAsync.receiveMessage: failed to set cancellation source.');
                }

                this.gifTools.init(vm);
                this.postMessage({msgType : 'MSG_TYPE_SET_VM_SUCCEEDED', msgId: msgId});

            }, rejectGifTools);
        } else if (msgType === 'MSG_TYPE_OPEN_SESSION') {
            if(!payload.hasOwnProperty('fileBuffer')) { return; }

            if (!this.gifTools.module().progressTokenSetProgressReporter({reportProgress: (progress: number) => this.onReportedProgress(progress) })) {
                console.log('GifToolsAsync.receiveMessage: failed to set progress reporter.');
            }

            let fileArrayBuffer = payload.fileBuffer as (ArrayBuffer | SharedArrayBuffer);
            if(!fileArrayBuffer) {
                this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_FAILED', msgId: msgId, error: 'Caught invalid or null file buffer.'});
                return;
            }

            if (!this.gifTools.module()) {
                this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_FAILED', msgId: msgId, error: 'Caught uninitialized module.'});
                return;
            }

            let fileBuffer = new Uint8Array(fileArrayBuffer);
            if(!fileBuffer) {
                this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_FAILED', msgId: msgId, error: 'Caught invalid or null file buffer.'});
                return;
            }

            this.onReportedProgress(0);

            if (!this.gifTools.videoDecoderOpenVideoStream(fileBuffer)) {
                this.onReportedProgress(1);
                this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_FAILED', msgId: msgId, error: 'Failed to open video stream from provided file buffer.'});
                return;
            }

            this.onReportedProgress(1);
            this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_SUCCEEDED', msgId: msgId, session: {
                width: this.gifTools.videoDecoderWidth(),
                height: this.gifTools.videoDecoderHeight(),
                frameCount: this.gifTools.videoDecoderFrameCount(),
                durationSeconds: this.gifTools.videoDecoderDurationSeconds(),
                frameDurationSeconds: this.gifTools.videoDecoderFrameDurationSeconds(),
            }});
        } else if (msgType === 'MSG_TYPE_RUN') {
            if(!payload.hasOwnProperty('runConfig')) { return; }

            this.onReportedProgress(0);
            
            const width = payload.runConfig.width;
            const height = payload.runConfig.height;
            const startTimeSeconds = payload.runConfig.startTimeSeconds;
            const endTimeSeconds = payload.runConfig.endTimeSeconds;
            const framesPerSecond = payload.runConfig.framesPerSecond;
            const frameDelaySeconds = payload.runConfig.frameDelaySeconds;
            const loop = payload.runConfig.loop;
            const boomerang = payload.runConfig.boomerang;

            const gifBase64 = this.run(width, height, startTimeSeconds, endTimeSeconds, framesPerSecond, frameDelaySeconds, loop, boomerang);
            this.gifTools.internalAddObjIds();

            if (gifBase64 == null) {
                this.onReportedProgress(1);
                this.postMessage({msgType : 'MSG_TYPE_RUN_FAILED', msgId: msgId});
                return;
            }

            this.onReportedProgress(1);
            this.postMessage({msgType : 'MSG_TYPE_RUN_SUCCEEDED', msgId: msgId, gifBase64: gifBase64});
        } else if (msgType === 'MSG_TYPE_CLOSE_SESSION') {
            if (!this.gifTools) {
                this.postMessage({msgType : 'MSG_TYPE_CLOSE_SESSION_FAILED', msgId: msgId, error: 'Caught null GifTools instance.'});
                return;
            }

            this.gifTools.videoDecoderCloseVideoStream();
            this.postMessage({msgType : 'MSG_TYPE_CLOSE_SESSION_SUCCEEDED', msgId: msgId});
        }
    }
};

// export default null as any;
