// import workerScript from './GifToolsWorker';
const Worker = require('worker-loader?inline&fallback=false!./GifToolsWorker');

type GifToolsRunProgressCallback = (progress: number) => void;

export class GifToolsRunConfig {
    width: number;
    height: number;
    startTimeSeconds: number;
    endTimeSeconds: number;
    framesPerSecond: number;
    frameDelaySeconds: number;
    loop: boolean;
    boomerang: boolean;
    progressCallback: GifToolsRunProgressCallback;
}

export class GifToolsRunOutput {
    gifBuffer: (Uint8Array | null);
    gifBase64: (string | null);
}

export class GifToolsSession {
    width: number;
    height: number;
    frameCount: number;
    durationSeconds: number;
    frameDurationSeconds: number;

    async run(runConfig: GifToolsRunConfig) : Promise<GifToolsRunOutput> {
        return GifToolsAsync.get().run(runConfig);
    }

    async close() : Promise<void> {
        return GifToolsAsync.get().closeSession();
    }
};

export class GifToolsAsync {
    private static worker = new Worker();
    private static instance = new GifToolsAsync()

    static get() { return GifToolsAsync.instance; }

    private messageCounter: number = 0;
    private resolves: any = {};
    private rejects: any = {};

    private cancellationBuffer = new ArrayBuffer(4);
    private cancellationToken = new Uint32Array(this.cancellationBuffer);
    private lastProgressCallback: (GifToolsRunProgressCallback|null) = null;

    private constructor() {
        GifToolsAsync.worker.onmessage = (event: any) => {
            this.receiveMessage(event);
        }
    }

    private nextMsgId() {
        return this.messageCounter++;
    }

    private schedule(msgId: number, resolve: any, reject: any) {
        this.resolves[msgId] = resolve;
        this.rejects[msgId] = reject;
    }

    private fulfill(msgId: number) {
        delete this.resolves[msgId];
        delete this.rejects[msgId];
    }

    private resolve(msgId: number, ...args: any[]) {
        let promiseResolveFn = this.resolves[msgId];
        promiseResolveFn(...args);
        this.fulfill(msgId);
    }

    private reject(msgId: number, ...args: any[]) {
        let promiseRejectFn = this.rejects[msgId];
        promiseRejectFn(...args);
        this.fulfill(msgId);
    }

    async init(): Promise<void> {
        const msgId = this.nextMsgId();

        return new Promise((resolve, reject) => {
            this.schedule(msgId, resolve, reject);

            let payload = {
                msgType: 'MSG_TYPE_SET_VM',
                msgId: msgId,
                cancellationToken: this.cancellationToken
            };

            this.postMessage(payload, [this.cancellationBuffer]);
        });
    }

    cancel() : boolean {
        if (this.cancellationToken == null) { return false; }
        console.log('GifToolsAsync.cancel');
        Atomics.store(this.cancellationToken, 0, 1);
        return true;
    }

    async openSession(fileBuffer: Uint8Array): Promise<GifToolsSession> {
        const msgId = this.nextMsgId();
        return new Promise((resolve, reject) => {
            this.schedule(msgId, resolve, reject);
            let payload = { msgType: 'MSG_TYPE_OPEN_SESSION', msgId: msgId, fileBuffer: fileBuffer.buffer };
            this.postMessage(payload, [fileBuffer.buffer]);
        });
    }

    async run(runConfig: GifToolsRunConfig) : Promise<GifToolsRunOutput> {
        const msgId = this.nextMsgId();
        return new Promise((resolve, reject) => {
            this.schedule(msgId, resolve, reject);
            this.lastProgressCallback = runConfig.progressCallback;
            delete runConfig.progressCallback;
            let payload = { msgType: 'MSG_TYPE_RUN', msgId: msgId, runConfig: runConfig };
            this.postMessage(payload);
        });
    }

    async closeSession(): Promise<void> {
        const msgId = this.nextMsgId();
        return new Promise((resolve, reject) => {
            this.schedule(msgId, resolve, reject);
            let payload = { msgType: 'MSG_TYPE_CLOSE_SESSION', msgId: msgId };
            this.postMessage(payload);
        });
    }

    private receiveMessage(message: MessageEvent) {
        let payload = message.data;
        console.log('GifToolsAsync.receiveMessage: payload=', payload);

        if (!payload.hasOwnProperty('msgType')) { return; }
        if (!payload.hasOwnProperty('msgId')) { return; }

        let msgType = payload.msgType;
        let msgId = payload.msgId;

        console.log('GifToolsAsync.receiveMessage: msgType=', msgType);
        console.log('GifToolsAsync.receiveMessage: msgId=', msgId);

        if (msgType === 'MSG_TYPE_SET_VM_SUCCEEDED') {
            this.resolve(msgId);
        } else if (msgType === 'MSG_TYPE_SET_VM_FAILED') {
            this.reject(msgId);
        } else if (msgType === 'MSG_TYPE_OPEN_SESSION_SUCCEEDED') {

            if (!payload.hasOwnProperty('session')) { return; }
            if (!payload.session.hasOwnProperty('width')) { return; }
            if (!payload.session.hasOwnProperty('height')) { return; }
            if (!payload.session.hasOwnProperty('frameCount')) { return; }
            if (!payload.session.hasOwnProperty('durationSeconds')) { return; }
            if (!payload.session.hasOwnProperty('frameDurationSeconds')) { return; }

            let session = new GifToolsSession();
            session.width = payload.session.width;
            session.height = payload.session.height;
            session.frameCount = payload.session.frameCount;
            session.durationSeconds = payload.session.durationSeconds;
            session.frameDurationSeconds = payload.session.frameDurationSeconds;

            this.resolve(msgId, session);
        } else if (msgType === 'MSG_TYPE_OPEN_SESSION_FAILED') {
            this.reject(msgId);
        } else if (msgType === 'MSG_TYPE_CLOSE_SESSION_SUCCEEDED') {
            this.resolve(msgId);
        } else if (msgType === 'MSG_TYPE_CLOSE_SESSION_FAILED') {
            this.reject(msgId);
        } else if (msgType === 'MSG_TYPE_RUN_SUCCEEDED') {
            let output = new GifToolsRunOutput();
            output.gifBuffer = null; // payload.gifBuffer as Uint8Array;
            output.gifBase64 = payload.gifBase64 as string;
            this.resolve(msgId, output);
        } else if (msgType === 'MSG_TYPE_RUN_FAILED') {
            this.reject(msgId);
        } else if (msgType === 'MSG_TYPE_REPORT_PROGRESS') {
            if (!payload.hasOwnProperty('progress')) { return; }
            if (this.lastProgressCallback) { this.lastProgressCallback!(payload.progress * 100); }
            console.log('GifToolsAsync.onReportedProgress: progress=', payload.progress * 100, '%');
        }
    }

    private postMessage(messagePayload: any, transfer?: Transferable[]) {
        console.log('GifToolsAsync.postMessage: messagePayload=', messagePayload, ', transfer=', transfer);

        if (transfer) { return GifToolsAsync.worker.postMessage(messagePayload, transfer); }
        return GifToolsAsync.worker.postMessage(messagePayload);
    }
};

/*
// 1
const gifTools = new GifTools();

// 2
gifTools.openSession(buffer)
 .then(session => {
  session.width
  session.height
  session.frames
  session.duration

  // start
  // end
  // fps = 10
  // delay = 1 / fps

  // loop = true / false
  // boomerang = true/false

  // output = ‘uint8array’ / ‘base64string’;
  // uint8array

  // config 1
  session.run({start, end, fps… })
   .then(result => { });

  // config 2
  session.run({start, end, fps… })
   .then(result => { });

  //
  session.close();
 });

*/