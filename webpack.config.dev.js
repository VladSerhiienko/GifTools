const path = require("path");
const HtmlWebPackPlugin = require('html-webpack-plugin');
const WorkerPlugin = require('worker-plugin');


module.exports = {
    entry: "./example/index.ts",
    module: {
        rules: [
            {
                test: /\.ts?$/,
                use: {
                    loader: 'ts-loader',
                },
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
        new HtmlWebPackPlugin({
            title: 'GifTools',
            template: 'example/index.html',
        }),
        new WorkerPlugin()
    ],
    devServer: {
        contentBase: path.join(__dirname, 'example'),
        compress: true,
        port: 9000
    },
};