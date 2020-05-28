import * as GifToolsFactory from '../bin/web_amalgamated_ffmpeg_worker/GifTools'; // @ts-ignore
// import * as GifToolsFactory from '../bin/web_amalgamated_ffmpeg/GifTools'; // @ts-ignore

//
// Subscribe to messages from client.
//

const worker: Worker = self as any;
worker.addEventListener("message", (messageEvent) => {
    GifToolsWorker.get().receiveMessage(messageEvent);
});

//
// Process messages from client and send responses.
//

class GifToolsWorker {
    private static instance = new GifToolsWorker();
    public static get(): GifToolsWorker { return GifToolsWorker.instance; }

    worker: Worker = worker;
    vm: any = null;

    postMessage(message: any) {
        console.log('GifToolsWorker.postMessage: message=', message);
        
        return this.worker.postMessage(message);
    }

    receiveMessage(messageEvent: MessageEvent) {
        let payload = messageEvent.data;

        console.log('GifToolsWorker.receiveMessage: payload=', payload);

        if(!payload.hasOwnProperty('msgType')) { return; }
        if(!payload.hasOwnProperty('msgId')) { return; }

        let msgType = payload.msgType;
        let msgId = payload.msgId;

        console.log('GifToolsAsync.receiveMessage: msgType=', msgType);
        console.log('GifToolsAsync.receiveMessage: msgId=', msgId);

        if (msgType === 'MSG_TYPE_SET_VM') {
            GifToolsFactory().then((vm: any) => {
                this.vm = vm;
                this.postMessage({msgType : 'MSG_TYPE_SET_VM_SUCCEEDED', msgId: msgId});
            }, () => {
                this.vm = null;
                this.postMessage({msgType : 'MSG_TYPE_SET_VM_FAILED', msgId: msgId});
            });
        }
    }
};
