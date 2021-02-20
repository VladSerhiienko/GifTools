const fs = require('fs');

require('child_process').exec('git rev-parse HEAD', function(err, stdout) {
    console.log('Setting GifTools version to', stdout);
    let contents = 'export function GifToolsVersion() { return \'' + stdout.trim() + '\'; }\n';
    fs.writeFileSync('./../src/GifToolsVersion.ts', contents);
});