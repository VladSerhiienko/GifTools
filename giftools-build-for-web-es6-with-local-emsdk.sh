
source dependencies/emsdk/emsdk_env.sh
cmake -Bbuild_emscripten_web_es6 -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_ES6_EMSCRIPTEN=TRUE
emmake make -C build_emscripten_web_es6
mkdir -p bin/web_es6
cp build_emscripten_web_es6/GifTools.js bin/web_es6/GifTools.js
cp build_emscripten_web_es6/GifTools.wasm bin/web_es6/GifTools.wasm

