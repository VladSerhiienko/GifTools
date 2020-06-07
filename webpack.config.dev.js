const path = require("path");
const HtmlWebPackPlugin = require('html-webpack-plugin');
const WorkerPlugin = require('worker-plugin');

module.exports = {
    entry: "./example/index.ts",
    module: {
        rules: [
            // {
            //     test: /Worker/,
            //     use: {
            //         loader: 'raw-loader',
            //         options: {
            //             esModule: false,
            //         },
            //     }
            // },
            {
                test: /\.ts?$/,
                use: {
                    loader: 'ts-loader',
                    options: {projectReferences: true },
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
    },
    plugins: [
        new HtmlWebPackPlugin({
            title: 'GifTools',
            template: 'example/index.html',
        })
    ],
    devServer: {
        contentBase: path.join(__dirname, 'example'),
        compress: true,
        port: 9000
    },
};