
import * as GifToolsModule from '../bin/web_amalgamated_ffmpeg/GifTools';

//
// TODO(vromanchak, vserhiienko): Promote to ES6.
//

// Module parse failed: Unexpected token (3:25)
// You may need an appropriate loader to handle this file type, currently no loaders are configured to process this file. See https://webpack.js.org/concepts#loaders
// | 
// | var GifToolsModularized = (function() {
// >   var _scriptDir = import.meta.url;
// |   
// |   return (
//  @ ./src/index.ts 3:21-74
// import * as GifToolsModule from '../bin/web_es6_amalgamated_ffmpeg/GifTools';

class GifToolsValidationError extends Error {
    constructor(message: string) {
      super(message); 
      this.name = "GifToolsValidationError";
    }
}

/**
 * GifTools class wraps GifTools WebAssembly module.
 * It mostly operates object ids (numbers), rather than actual objects.
 * All previously allocated objects must be manually freed.
 * This class takes care about memory, although it's up to users to deinit it.
 */
export default class GifTools {
    static invalidObjId : number = 0;
    static isValidObj(objId : number) : boolean { return objId > 0; }
    static assert(cond: boolean, msg: string) {
        if (!cond) { debugger; throw new GifToolsValidationError(msg); }
    }

    vm: any;
    vmObjIds: Array<number> = [];
    
    currentGifBuilderId: number = 0;
    currentGifBuilderWidth: number = 0;
    currentGifBuilderHeight: number = 0;

    currentInputStreamId: number = 0;
    currentVideoStreamId: number = 0;

    /**
     * Initializes this instance.
     */
    init() : boolean {
        this.vm = GifToolsModule();
        return this.vm != null;
    }

    /**
     * Finalizes this instance, frees all dangling objects.
     * Make sure all encoders are done.
     */
    deinit() {
        GifTools.assert(!GifTools.isValidObj(this.currentGifBuilderId), "Caught active GIF encoder.");
        this.freeObjIds();
        this.vm = null;
    }

    /**
     * Appends object ids to the free list.
     * @param objIds Object ids (numbers).
     */
    addObjIds(...objIds : number[]) {
        if (objIds == undefined || objIds == null) { return; }
        objIds.forEach(objId => this.vmObjIds.push(objId));
    }

    /**
     * Removes object ids from the free list and frees associated memory.
     * If object ids are not provided, previously added objects are freed.
     * @param objIds Optional object ids (numbers).
     */
    freeObjIds(...objIds : number[]) {
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
        this.addObjIds(base64BufferId);
        var bufferArray = this.vm.bufferToUint8Array(base64BufferId);
        var textDecoder = new TextDecoder("utf-8");
        var base64String = textDecoder.decode(bufferArray);
        this.freeObjIds(base64BufferId);
        return base64String;
    }

    /**
     * Loads provided file buffer into an image object, returns its id.
     * @param fileBuffer File memory stored in Uint8Array instance.
     */
    loadImageFromFileBuffer(fileBuffer : Uint8Array) : number {
        const bufferId = this.vm.bufferFromUint8Array(fileBuffer);
        if (!GifTools.isValidObj(bufferId)) { return GifTools.invalidObjId; }

        this.addObjIds(bufferId);
        const imageId = this.vm.imageLoadFromBuffer(bufferId);
        if (!GifTools.isValidObj(bufferId)) {
            this.freeObjIds(bufferId);
            return GifTools.invalidObjId;
        }

        this.addObjIds(imageId);
        return imageId;
    }

    /**
     * Resizes provided image, or returns the same image id in case extents match.
     * @param imageId Image object id.
     * @param width Desired image width.
     * @param height Desired image height.
     */
    resizeImage(imageId : number, width: number, height: number) : number {
        if ([imageId, width, height].some(n => n <= 0)) { return imageId };

        if (width == this.vm.imageWidth(imageId) && height == this.vm.imageHeight(imageId)) { return imageId; }

        const resizedImageId = this.vm.imageResizeOrClone(imageId, width, height);
        if (!GifTools.isValidObj(resizedImageId)) { return imageId; }

        this.addObjIds(resizedImageId);
        return resizedImageId;
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
        this.addObjIds(this.currentGifBuilderId);
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
        return this.vm.gifBuilderAddImage(this.currentGifBuilderId, imageId, delay);
    }

    /**
     * Finalizes GIF encoding.
     * Returns @class Uint8Array GIF file buffer.
     */
    gifEncoderEnd() : Uint8Array {
        GifTools.assert(GifTools.isValidObj(this.currentGifBuilderId), "Caught null GIF encoder.");
        var gifBufferId = this.vm.gifBuilderFinalize(this.currentGifBuilderId);

        this.freeObjIds(this.currentGifBuilderId);
        this.currentGifBuilderId = GifTools.invalidObjId;
        this.currentGifBuilderWidth = 0;
        this.currentGifBuilderHeight = 0;

        var gifBufferArray = this.vm.bufferToUint8Array(gifBufferId);
        this.freeObjIds(gifBufferId);
        return gifBufferArray;
    }
}
