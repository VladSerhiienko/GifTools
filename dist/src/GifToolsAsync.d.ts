declare type GifToolsRunProgressCallback = (progress: number) => void;
export declare class GifToolsRunConfig {
    width: number;
    height: number;
    startTimeSeconds: number;
    endTimeSeconds: number;
    framesPerSecond: number;
    frameDelaySeconds: number;
    loop: boolean;
    boomerang: boolean;
    outputType: String;
    progressCallback: GifToolsRunProgressCallback;
}
export declare class GifToolsRunOutput {
    output: (ArrayBuffer | String | null);
}
export declare class GifToolsSession {
    width: number;
    height: number;
    frameCount: number;
    durationSeconds: number;
    frameDurationSeconds: number;
    run(runConfig: GifToolsRunConfig): Promise<GifToolsRunOutput>;
    close(): Promise<void>;
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
    private lastProgressCallback;
    private constructor();
    private nextMsgId;
    private schedule;
    private fulfill;
    private resolve;
    private reject;
    init(): Promise<void>;
    cancel(): boolean;
    openSession(fileBuffer: Uint8Array): Promise<GifToolsSession>;
    run(runConfig: GifToolsRunConfig): Promise<GifToolsRunOutput>;
    closeSession(): Promise<void>;
    private receiveMessage;
    private postMessage;
}
export {};
