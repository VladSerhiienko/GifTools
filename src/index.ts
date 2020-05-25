import Worker from "worker-loader!./Worker";

export default class GifTools {
    private worker: Worker;

    constructor() {
        const worker = new Worker();

        worker.postMessage({ a: 1 });
        worker.onmessage = (event) => {};

        worker.addEventListener("message", (event) => {});
    }
}
