import {GifToolsAsync, GifToolsSession} from '../src/GifToolsAsync'
import { rejects } from 'assert';

GifToolsAsync.get().init().then(() => {
    console.log("index: GifToolsAsync.init: succeeded");
}, () => {
    console.log("index: GifToolsAsync.init: failed");
});

function arrayBufferFromFile(file: File) {
    console.log('arrayBufferFromFile: file.arrayBuffer', file.arrayBuffer);

    // Newer stuff that is not available in Safari.
    // https://gist.github.com/hanayashiki/8dac237671343e7f0b15de617b0051bd
    if (!file.arrayBuffer) {
        return new Promise((resolve, reject) => {
            console.log('arrayBufferFromFile: promise');

            let fileReader = new FileReader();
            console.log('arrayBufferFromFile: fileReader', fileReader);
            
            fileReader.onloadend = () => { console.log('arrayBufferFromFile: resolved'); resolve(fileReader.result); };
            fileReader.onerror = () => { console.log('arrayBufferFromFile: rejected'); reject(); };
            fileReader.onabort = () => { console.log('arrayBufferFromFile: aborted'); reject(); };
            fileReader.onprogress = () => { console.log('arrayBufferFromFile: progress'); };

            console.log('arrayBufferFromFile: readAsArrayBuffer', fileReader);
            fileReader.readAsArrayBuffer(this);
        });
    }

    return file.arrayBuffer();
}

const result = document.getElementById('result');
console.log('result ?', result);
if (result) {
    const button = document.getElementById('myFile');
    console.log('button ?', button);
    if (button) {
        button.onchange = event => {
            console.log('button.onclick!');

            if (!event) { return; }
            if (!event.target) { return; }

            const target = <HTMLInputElement>event.target;
            console.log('button.onclick! target', target);
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
            }, () => {
                console.log('Failed to open sesion from', file);
            });

            // setTimeout(() => {
            //     result.setAttribute('src', 'https://media.giphy.com/media/xTk9ZvMnbIiIew7IpW/giphy.gif');
            // }, 3000);
        }
    }
}


