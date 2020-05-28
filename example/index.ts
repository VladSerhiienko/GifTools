import {GifToolsAsync} from '../src/GifToolsAsync'

GifToolsAsync.get().init().then((vmPro) => {
    console.log("index: GifToolsAsync.init: succeeded");
}, () => {
    console.log("index: GifToolsAsync.init: failed");
});
