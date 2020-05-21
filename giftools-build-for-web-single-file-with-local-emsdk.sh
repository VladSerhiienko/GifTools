
source dependencies/emsdk/emsdk_env.sh
cmake -Bbuild_emscripten_web_amalgamated -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE
emmake make -C build_emscripten_web_amalgamated
mkdir -p bin/web_amalgamated
cp build_emscripten_web_amalgamated/GifTools.js bin/web_amalgamated/GifTools.js

cmake -Bbuild_emscripten_web_amalgamated_ffmpeg -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_FFMPEG=TRUE
emmake make -C build_emscripten_web_amalgamated_ffmpeg
mkdir -p bin/web_amalgamated_ffmpeg
cp build_emscripten_web_amalgamated_ffmpeg/GifTools.js bin/web_amalgamated_ffmpeg/GifTools.js
cp build_emscripten_web_amalgamated_ffmpeg/GifTools.wasm.map bin/web_amalgamated_ffmpeg/GifTools.wasm.map

cmake -Bbuild_emscripten_web_amalgamated_ffmpegd -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_FFMPEG=TRUE
emmake make -C build_emscripten_web_amalgamated_ffmpegd
mkdir -p bin/web_amalgamated_ffmpegd
cp build_emscripten_web_amalgamated_ffmpegd/GifTools.js bin/web_amalgamated_ffmpegd/GifTools.js
cp build_emscripten_web_amalgamated_ffmpegd/GifTools.wasm.map bin/web_amalgamated_ffmpegd/GifTools.wasm.map
# emcc:WARNING: Wasm source map won't be usable in a browser without --source-map-base

