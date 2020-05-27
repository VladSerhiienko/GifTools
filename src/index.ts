// @ts-ignore
// eslint-disable-next-line import/no-webpack-loader-syntax
const worker = new Worker('./Worker.ts', { type: 'module' });

worker.onmessage = event => {
    console.log('index.ts:worker.onmessage: event.data=', event.data);
}

export default function post(message: any) {
    console.log('index.ts:post: message=', message);
    worker.postMessage(message);
}

// let instance = wokrer123()

// console.log('inst', instance);

// instance.expensive(1000).then( (count: number) => {
//     console.log(`Ran ${count} loops`)
// })

// console.log(123);

// worker.postMessage({ a: 1 });
// worker.onmessage = (event) => {};

// worker.addEventListener("message", (event) => {});
