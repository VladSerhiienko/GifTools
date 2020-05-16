
source dependencies/emsdk/emsdk_env.sh
cmake -Bbuild_emscripten_node -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE
emmake make -C build_emscripten_node
mkdir -p bin/node
cp build_emscripten_node/GifTools.js bin/node/GifTools.js
cp build_emscripten_node/GifTools.wasm bin/node/GifTools.wasm

cmake -Bbuild_emscripten_node_ffmpeg -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_FFMPEG=TRUE
emmake make -C build_emscripten_node_ffmpeg
mkdir -p bin/node_ffmpeg
cp build_emscripten_node_ffmpeg/GifTools.js bin/node_ffmpeg/GifTools.js
cp build_emscripten_node_ffmpeg/GifTools.wasm bin/node_ffmpeg/GifTools.wasm

