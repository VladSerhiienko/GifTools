const path = require("path");
const HtmlWebPackPlugin = require('html-webpack-plugin')

module.exports = {
    entry: {
        bundle: "./example/index.ts",
        worker: "./src/worker.ts",
    },
    module: {
        rules: [
            { test: /\.worker\.ts$/, loader: 'worker-loader' },
            {
                test: /\.ts?$/,
                use: 'ts-loader',
                exclude: /node_modules/,
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
        umdNamedDefine: true
    },
    plugins: [
        new HtmlWebPackPlugin()
    ],
    devServer: {
        contentBase: path.join(__dirname, 'example'),
        compress: true,
        port: 9000
    },
};