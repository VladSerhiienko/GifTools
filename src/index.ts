export default class GitTools {
    worker = new Worker('./worker');

    addImage(data: Uint8Array) {
        this.worker.postMessage('addImage', [data]);
    }
}