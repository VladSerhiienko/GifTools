
// Module parse failed: Unexpected token (3:25)
// You may need an appropriate loader to handle this file type, currently no loaders are configured to process this file. See https://webpack.js.org/concepts#loaders
// | 
// | var GifToolsModularized = (function() {
// >   var _scriptDir = import.meta.url;
// |   
// |   return (
//  @ ./src/index.ts 3:21-74
// import * as GifToolsModule from '../bin/web_es6_amalgamated_ffmpeg/GifTools';

import * as GifToolsModule from '../bin/web_amalgamated_ffmpeg/GifTools';
// import * as GifToolsModule from '../bin/web_amalgamated/GifTools';

export default class GifToolsWrapper {
    module = GifToolsModule();
    gifBuilderId: any;

    initGif(width: number, height: number, delay: number) {
        console.log(this.module);
        this.gifBuilderId = this.module.gifBuilderInitialize(width, height, delay);
    }

    addImage(fileBuffer: Uint8Array, width: number, height: number, delay: number) {
        const id = this.loadImageFromFileAndResize(fileBuffer, width, height);
        this.module.gifBuilderAddImage(this.gifBuilderId, id, delay);
    }

    loadImageFromFileAndResize(fileBuffer: Uint8Array, width: number, height: number): any {
        const bufferId = this.module.bufferFromUint8Array(fileBuffer);
        console.log('buffer', 'id', bufferId, 'size', this.module.bufferSize(bufferId));

        const imageId = this.module.imageLoadFromBuffer(bufferId);
        console.log('image',
                    'id',
                    imageId,
                    'w',
                    this.module.imageWidth(imageId),
                    'h',
                    this.module.imageHeight(imageId),
                    'f',
                    this.module.imageFormat(imageId));

        this.module.objectFree(bufferId);

        const smallImageId = this.module.imageResizeOrClone(imageId, width, height);
        console.log('resized image',
                    'id',
                    smallImageId,
                    'w',
                    this.module.imageWidth(smallImageId),
                    'h',
                    this.module.imageHeight(smallImageId),
                    'f',
                    this.module.imageFormat(smallImageId));

        this.module.objectFree(imageId);
        return smallImageId;
    }

    save(): Uint8Array {
        var gifBufferId = this.module.gifBuilderFinalize(this.gifBuilderId);

        // TODO: cleanup
        // GifToolsModule._objectFree(smallImageIds[0]);
        // GifToolsModule._objectFree(smallImageIds[1]);
        // GifToolsModule._objectFree(smallImageIds[2]);
        // GifToolsModule._objectFree(smallImageIds[3]);

        return this.module.bufferToUint8Array(gifBufferId);
    }
}
