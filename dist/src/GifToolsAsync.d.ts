export declare class GifToolsSession {
    width: number;
    height: number;
    frameCount: number;
    durationSeconds: number;
    frameDurationSeconds: number;
}
export declare class GifToolsAsync {
    private static worker;
    private static instance;
    static get(): GifToolsAsync;
    private messageCounter;
    private resolves;
    private rejects;
    private cancellationBuffer;
    private cancellationToken;
    private constructor();
    private nextMsgId;
    private schedule;
    private fulfill;
    private resolve;
    private reject;
    init(): Promise<void>;
    cancel(): boolean;
    openSession(fileBuffer: Uint8Array): Promise<GifToolsSession>;
    private receiveMessage;
    private postMessage;
}
