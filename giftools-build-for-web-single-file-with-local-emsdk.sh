
source dependencies/emsdk/emsdk_env.sh

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

cmake -Bbuild_emscripten_web_amalgamated_ffmpeg_bug -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE \
    -DGIFTOOLS_ENABLE_ERROR_EMSCRIPTEN_11208=TRUE \
    -DGIFTOOLS_BUILD_FFMPEG=TRUE
emmake make -C build_emscripten_web_amalgamated_ffmpeg_bug
mkdir -p bin/web_amalgamated_ffmpeg_bug
cp build_emscripten_web_amalgamated_ffmpeg_bug/GifTools.js bin/web_amalgamated_ffmpeg_bug/GifTools.js
cp build_emscripten_web_amalgamated_ffmpeg_bug/GifTools.wasm.map bin/web_amalgamated_ffmpeg_bug/GifTools.wasm.map
