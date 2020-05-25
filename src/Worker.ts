export function expensive(time : number) {
    console.log('received', time);
    return time * time;
}

addEventListener('message', event => {
    postMessage(expensive(event.data));
});