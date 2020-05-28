import * as GifToolsFactory from '../bin/web_amalgamated_ffmpeg_worker/GifTools'; // @ts-ignore
// import * as GifToolsFactory from '../bin/web_amalgamated_ffmpeg/GifTools'; // @ts-ignore

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

    videoDecoderPrepareFrames(framesPerSecond: number) : number {
        if (!GifTools.isValidObj(this.currentVideoStreamId)) { return 0; }
        return this.vm.ffmpegVideoStreamPrepareFrames(this.currentVideoStreamId, framesPerSecond);
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

    postMessage(message: any) {
        console.log('GifToolsWorker.postMessage: message=', message);
        
        return this.worker.postMessage(message);
    }

    receiveMessage(messageEvent: MessageEvent) {
        let payload = messageEvent.data;

        console.log('GifToolsWorker.receiveMessage: payload=', payload);

        if(!payload.hasOwnProperty('msgType')) { return; }
        if(!payload.hasOwnProperty('msgId')) { return; }

        let msgType = payload.msgType;
        let msgId = payload.msgId;

        console.log('GifToolsAsync.receiveMessage: msgType=', msgType);
        console.log('GifToolsAsync.receiveMessage: msgId=', msgId);

        if (msgType === 'MSG_TYPE_SET_VM') {
            GifToolsFactory().then((vm: any) => {
                if (vm) {
                    this.gifTools.init(vm);
                    this.postMessage({msgType : 'MSG_TYPE_SET_VM_SUCCEEDED', msgId: msgId});
                } else {
                    this.gifTools.init(null);
                    this.postMessage({msgType : 'MSG_TYPE_SET_VM_FAILED', msgId: msgId});
                }
            }, () => {
                this.gifTools.init(null);
                this.postMessage({msgType : 'MSG_TYPE_SET_VM_FAILED', msgId: msgId});
            });
        } else if (msgType === 'MSG_TYPE_OPEN_SESSION') {
            if(!payload.hasOwnProperty('fileBuffer')) { return; }

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

            if (!this.gifTools.videoDecoderOpenVideoStream(fileBuffer)) {
                this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_FAILED', msgId: msgId, error: 'Failed to open video stream from provided file buffer.'});
                return;
            }

            this.postMessage({msgType : 'MSG_TYPE_OPEN_SESSION_SUCCEEDED', msgId: msgId, session: {
                width: this.gifTools.videoDecoderWidth(),
                height: this.gifTools.videoDecoderHeight(),
                durationSeconds: this.gifTools.videoDecoderDurationSeconds(),
                frameDurationSeconds: this.gifTools.videoDecoderFrameDurationSeconds(),
            }});
        }
    }
};
