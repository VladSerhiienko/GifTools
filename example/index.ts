import {GifToolsAsync} from '../src/GifToolsAsync'

GifToolsAsync.get().init().then(() => {
    console.log("index: GifToolsAsync.init: succeeded");
}, () => {
    console.log("index: GifToolsAsync.init: failed");
});

const result = document.getElementById('result');

const button = document.getElementById('myFile');
button.onclick = event => {
    const file = (<HTMLInputElement>event.target).files[0];
    console.log('file ?', file);

    setTimeout(() => {
        result.setAttribute('src', 'https://itlab.am/images/How-to/gif-man-melting.gif');
    }, 3000);
};


