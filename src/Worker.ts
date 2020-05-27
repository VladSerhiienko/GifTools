// Worker.ts
const ctx: Worker = self as any;

export function expensive(time : number) {
    console.log('Worker.ts:expensive: time=', time);
    return time * time;
}

// Post data to parent thread
console.log('Worker.ts:postMessage: foo:foo');
ctx.postMessage({ foo: "foo" });

// Respond to message from parent thread
ctx.addEventListener("message", (event) => {
    console.log('Worker.ts:addEventListener: event.data=', event.data);
    ctx.postMessage(expensive(event.data));
});
