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
                type: "javascript/auto",
                loader: "file-loader",
                options: {
                    publicPath: "dist/"
                }
            }
        ]
    },
    resolve: {
        extensions: [ '.tsx', '.ts', '.js' ],
    },
    output: {
        filename: "bundle.js",
        path: path.resolve(__dirname, "dist")
    },
};