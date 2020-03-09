import * as GifToolsModule from '../bin/web_amalgamated/GifTools';

export default class GifToolsWrapper {
    module = GifToolsModule();
    gifBuilderId: any;

    initGif(width: number, height: number, delay: number) {
        this.gifBuilderId = this.module._gifBuilderInitialize(width, height, delay);
    }

    addImage(imageId: any, delay: number) {
        this.module._gifBuilderAddImage(this.gifBuilderId, imageId, delay);
    }

    makeGif(): Uint8Array {
        var gifBufferId = this.module._gifBuilderFinalize(this.gifBuilderId);

        // TODO: cleanup
        // GifToolsModule._objectFree(smallImageIds[0]);
        // GifToolsModule._objectFree(smallImageIds[1]);
        // GifToolsModule._objectFree(smallImageIds[2]);
        // GifToolsModule._objectFree(smallImageIds[3]);
    
        return this.module.bufferToUint8Array(gifBufferId);
    }

    loadImageFromFileAndResize(fileBuffer: Uint8Array, width: number, height: number): any {
        const bufferId = this.module.bufferFromUint8Array(fileBuffer);
        console.log('buffer', 'id', bufferId, 'size', this.module._bufferSize(bufferId));

        const imageId = this.module._imageLoadFromBuffer(bufferId);
        console.log('image',
            'id',
            imageId,
            'w',
            this.module._imageWidth(imageId),
            'h',
            this.module._imageHeight(imageId),
            'f',
            this.module._imageFormat(imageId));

        this.module._objectFree(bufferId);

        const smallImageId = this.module._imageResizeOrClone(imageId, width, height);
        console.log('resized image',
            'id',
            smallImageId,
            'w',
            this.module._imageWidth(smallImageId),
            'h',
            this.module._imageHeight(smallImageId),
            'f',
            this.module._imageFormat(smallImageId));

        this.module._objectFree(imageId);
        return smallImageId;
    }
}
