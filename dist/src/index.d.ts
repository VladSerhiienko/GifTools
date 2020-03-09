export default class GifToolsWrapper {
    module: any;
    gifBuilderId: any;
    initGif(width: number, height: number, delay: number): void;
    addImage(fileBuffer: Uint8Array, width: number, height: number, delay: number): void;
    loadImageFromFileAndResize(fileBuffer: Uint8Array, width: number, height: number): any;
    save(): Uint8Array;
}
