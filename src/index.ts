// @ts-ignore
// eslint-disable-next-line import/no-webpack-loader-syntax
const worker = new Worker('./worker.ts', { type: 'module' });

worker.onmessage = data => {
    console.log(data);
}

worker.postMessage(10);

console.log(worker);

// let instance = wokrer123()

// console.log('inst', instance);

// instance.expensive(1000).then( (count: number) => {
//     console.log(`Ran ${count} loops`)
// })

// console.log(123);

// worker.postMessage({ a: 1 });
// worker.onmessage = (event) => {};

// worker.addEventListener("message", (event) => {});
