var wasmBinaryFile = '../bin/GifTools.wasm';
var wasmJSFile = '../bin/GifTools.js';

describe('test add', () => {
    let GifToolsLoader;

    beforeEach(() => {
        GifToolsLoader = require(wasmJSFile);;
    })

    beforeAll(() => {
        jest.useFakeTimers();
    });

    describe('positive integers', () => {
        test('the data is peanut butter', done => {
            try {
                GifToolsLoader({wasmBinaryFile})
                    .then(GifToolsModule => {
                        var result = GifToolsModule._testAdd(49, 55);
                        // expect(result).toBe(100)
                        expect(result).toMatchSnapshot();
                        done();
                    });
            } catch (error) {
                done(error);
            }
        });
    });
});

