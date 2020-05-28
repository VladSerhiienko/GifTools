import {GifToolsAsync} from '../src/GifToolsAsync'

GifToolsAsync.get().init().then(() => {
    console.log("index: GifToolsAsync.init: succeeded");
}, () => {
    console.log("index: GifToolsAsync.init: failed");
});

const result = document.getElementById('result');
if (result) {
    const button = document.getElementById('myFile');
    if (button) {
        button.onclick = event => {
            if (!event) { return; }
            if (!event.target) { return; }

            const target = <HTMLInputElement>event.target;
            if (!target) { return; }
            if (!target.files) { return; }
            if (!target.files[0]) { return; }

            const file = (<HTMLInputElement>event!.target)!.files![0];
            console.log('file ?', file);

            setTimeout(() => {
                result.setAttribute('src', 'https://itlab.am/images/How-to/gif-man-melting.gif');
            }, 3000);
        }
    }
}


