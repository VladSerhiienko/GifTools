import {GifToolsAsync, GifToolsSession, GifToolsRunConfig, GifToolsRunOutput} from '../src/GifToolsAsync'

GifToolsAsync.get().init().then(() => {
    console.log("index: GifToolsAsync.init: succeeded");
}, () => {
    console.log("index: GifToolsAsync.init: failed");
});

function arrayBufferFromFile(file: File) {
    // Newer stuff that is not available in Safari.
    // https://gist.github.com/hanayashiki/8dac237671343e7f0b15de617b0051bd
    if (!file.arrayBuffer) {
        return new Promise((resolve, reject) => {
            let reader = new FileReader();

            reader.addEventListener('load', (event) => { resolve(event.target!.result); });
            reader.addEventListener('error', () => { reject(); });
            reader.addEventListener('progress', (event) => { console.log('progress: ', event.loaded, '/', event.total); });

            reader.readAsArrayBuffer(file);
        });
    }

    return file.arrayBuffer();
}

const result = document.getElementById('result');
console.log('result ?', result);
if (result) {
    const button = document.getElementById('myFile');
    if (button) {
        button.addEventListener('change', (event) => {
            console.log('button.onclick!');

            if (!event) { return; }
            if (!event.target) { return; }

            const target = <HTMLInputElement>event.target;
            if (!target) { return; }
            if (!target.files) { return; }
            if (!target.files[0]) { return; }

            const file = (<HTMLInputElement>event!.target)!.files![0];
            console.log('button.onclick! file', file);

            arrayBufferFromFile(file).then((fileArrayBuffer: ArrayBuffer) => {
                console.log('fileArrayBuffer', fileArrayBuffer);
                let fileBuffer = new Uint8Array(fileArrayBuffer);
                console.log('fileBuffer', fileBuffer);
                return GifToolsAsync.get().openSession(fileBuffer);
            }, () => {
                console.log('Failed to retrieve fileBuffer from', file);
            }).then((session: GifToolsSession) => {
                console.log('session', session);

                const runConfig = new GifToolsRunConfig();
                runConfig.width = session.width;
                runConfig.height = session.height;
                runConfig.startTimeSeconds = 0;
                runConfig.endTimeSeconds = session.durationSeconds;
                runConfig.framesPerSecond = 1;
                runConfig.frameDelaySeconds = 1;
                runConfig.loop = true;
                runConfig.boomerang = true;

                return GifToolsAsync.get().run(runConfig);
            }, () => {
                console.log('Failed to open session from', file);
            }).then((output: GifToolsRunOutput) => {
                console.log('output', output);
                result!.setAttribute('src', 'data:image/gif;base64,' + output.gifBase64);
            }, () => {
                console.log('Failed to creat GIF');
            });;
        });
    }
}


