// Worker.ts
const ctx: Worker = self as any;

export function expensive(time : number) {
    console.log('received 1', time);
    return time * time;
}

// Post data to parent thread
ctx.postMessage({ foo: "foo" });

// Respond to message from parent thread
ctx.addEventListener("message", (event) => {
    ctx.postMessage(expensive(event.data));
});
