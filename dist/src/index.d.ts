export default class GifToolsWrapper {
    module: any;
    gifBuilderId: any;
    initGif(width: number, height: number, delay: number): void;
    addImage(imageId: any, delay: number): void;
    makeGif(): void;
    loadImageFromFileAndResize(fileBuffer: Uint8Array, width: number, height: number): any;
}
