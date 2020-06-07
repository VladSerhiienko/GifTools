const path = require("path");

module.exports = {
    entry: "./src/index.ts",
    module: {
        rules: [
            {
                test: /\.ts?$/,
                use: 'ts-loader',
                exclude: /node_modules/,
            }
        ]
    },
    output: {
        path: path.resolve(__dirname, "dist"),	
        filename: "[name].js",	
        libraryTarget: 'umd',	
        library: 'GifTools',	
        umdNamedDefine: true
    },
    mode: process.env.NODE_ENV || 'development',
    resolve: {
        extensions: [ '.tsx', '.ts', '.js' ],
    },
    devtool: 'source-map',
};