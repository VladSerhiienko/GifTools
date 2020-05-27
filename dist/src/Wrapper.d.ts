export declare class GifToolsValidationError extends Error {
    constructor(message: string);
}
export declare class GifToolsVideoFrame {
    frameId: number;
    imageId: number;
    timeSeconds: number;
}
/**
 * GifTools class wraps GifTools WebAssembly module.
 * It mostly operates object ids (numbers), rather than actual objects.
 * All previously allocated objects must be manually freed.
 * This class takes care about memory, although it's up to users to deinit it.
 */
export declare class GifTools {
    static invalidObjId: number;
    static isValidObj(objId: number): boolean;
    private static vmStatusNull;
    private static vmStatusPending;
    private static vmStatusFulfilled;
    private static assert;
    vm: any;
    private vmStatus;
    private vmObjIds;
    private currentGifBuilderId;
    private currentGifBuilderWidth;
    private currentGifBuilderHeight;
    private currentInputStreamBufferId;
    private currentInputStreamId;
    private currentVideoStreamId;
    /**
     * Initializes this instance.
     */
    init(): Promise<boolean>;
    /**
     * Finalizes this instance, frees all dangling objects.
     * Make sure all encoders are done.
     */
    deinit(): void;
    module(): any;
    /**
     * Appends object ids to the free list.
     * @param objIds Object ids (numbers).
     */
    internalAddObjIds(...objIds: number[]): void;
    /**
     * Removes object ids from the free list and frees associated memory.
     * If object ids are not provided, previously added objects are freed.
     * @param objIds Optional object ids (numbers).
     */
    internalFreeObjIds(...objIds: number[]): void;
    bufferToBase64(bufferId: number): string;
    /**
     * Loads provided file buffer into an image object, returns its id.
     * @param fileBuffer File memory stored in Uint8Array instance.
     */
    imageLoadFromFileBuffer(fileBuffer: Uint8Array): number;
    /**
     * Resizes provided image, or returns the same image id in case extents match.
     * @param imageId Image object id.
     * @param width Desired image width.
     * @param height Desired image height.
     */
    imageResize(imageId: number, width: number, height: number): number;
    videoDecoderOpenVideoStream(fileBuffer: Uint8Array): boolean;
    videoDecoderWidth(): number;
    videoDecoderHeight(): number;
    videoDecoderDurationSeconds(): number;
    videoDecoderFrameDurationSeconds(): number;
    videoDecoderPrepareAllFrames(): number;
    videoDecoderPrepareFrames(framesPerSecond: number): number;
    videoDecoderPickClosestPreparedVideoFrame(durationSeconds: number): GifToolsVideoFrame | null;
    videoDecoderPickClosestVideoFrame(durationSeconds: number): GifToolsVideoFrame | null;
    videoDecoderFreeVideoFrame(frame: (GifToolsVideoFrame | null)): void;
    videoDecoderCloseVideoStream(): void;
    /**
     * Starts GIF encoding.
     * Make sure to call @function gifEncoderEnd() after GIF encoding is done.
     * @param width Desired GIF width.
     * @param height Desired GIF height.
     * @param delay Desired GIF delay between loops in ms, or 0 to a single loop.
     */
    gifEncoderBegin(width: number, height: number, delay: number): boolean;
    /**
     * Appends image to GIF.
     * Image is not needed further and can be freed after this functions returns.
     * @param imageId Image object id.
     * @param delay Desired GIF delay between frames in ms.
     */
    gifEncoderAddImage(imageId: number, delay: number): boolean;
    /**
     * Finalizes GIF encoding.
     * Returns @class Uint8Array GIF file buffer.
     */
    gifEncoderEnd(): Uint8Array;
}
