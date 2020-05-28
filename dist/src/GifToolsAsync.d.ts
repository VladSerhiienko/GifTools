export declare class GifToolsAsync {
    private static worker;
    private static instance;
    static get(): GifToolsAsync;
    resolves: any[];
    rejects: any[];
    messageCounter: number;
    init(): Promise<void>;
    receiveMessage(message: MessageEvent): void;
    postMessage(messagePayload: any): void;
}
