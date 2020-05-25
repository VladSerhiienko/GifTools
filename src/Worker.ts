export function expensive(time : number) {
    return time * time;
}

addEventListener('message', event => {
    postMessage(expensive(event.data));
});