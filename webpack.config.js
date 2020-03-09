const path = require("path");

module.exports = {
    entry: "./src/index.ts",
    module: {
        rules: [
            {
                test: /\.tsx?$/,
                use: 'ts-loader',
                exclude: /node_modules/,
            },
            {
                test: /GifTools\.js$/,
                loader: ['@open-wc/webpack-import-meta-loader', "exports-loader"]
            },
            {
                test: /Giftools\.wasm$/,
                loaders: ['wasm-loader']
            }
        ]
    },
    resolve: {
        extensions: [ '.tsx', '.ts', '.js', '.wasm' ],
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