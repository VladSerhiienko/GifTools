const path = require("path");

module.exports = {
    entry: "./src/index.ts",
    node: {
        fs: 'empty'
    },
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                use: 'ts-loader',
                exclude: /node_modules/,
            },
            {
                test: /GifTools\.js$/,
                loader: "exports-loader"
            },
            {
                test: /Giftools\.wasm$/,
                loaders: ['wasm-loader']
            }
        ]
    },
    resolve: {
        extensions: [ '.tsx', '.ts', '.js' ],
    },
    devtool: 'source-map',
    output: {
        path: path.resolve(__dirname, "dist"),
        filename: "[name].js",
        libraryTarget: 'umd',
        library: 'GifTools',
        umdNamedDefine: true
    },
};