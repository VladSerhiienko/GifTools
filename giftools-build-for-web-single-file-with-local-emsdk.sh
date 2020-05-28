
#
#
# Configure emcc, make it visible.
#
#

source dependencies/emsdk/emsdk_env.sh

#
#
# Ship.
#
#

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

#
#
#

cmake -Bbuild_emscripten_web_amalgamated_ffmpeg_worker -H. \
    -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
    -DGIFTOOLS_BUILD_EMSCRIPTEN:BOOL=TRUE \
    -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN:BOOL=TRUE \
    -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN:BOOL=TRUE \
    -DGIFTOOLS_BUILD_WORKER:BOOL=TRUE \
    -DGIFTOOLS_BUILD_FFMPEG:BOOL=TRUE
emmake make -C build_emscripten_web_amalgamated_ffmpeg_worker
mkdir -p bin/web_amalgamated_ffmpeg_worker
cp build_emscripten_web_amalgamated_ffmpeg_worker/GifTools.js bin/web_amalgamated_ffmpeg_worker/GifTools.js
cp build_emscripten_web_amalgamated_ffmpeg_worker/GifTools.wasm.map bin/web_amalgamated_ffmpeg_worker/GifTools.wasm.map

#
#
# Build without FFmpeg.
#
#

# cmake -Bbuild_emscripten_web_amalgamated -H. \
#     -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
#     -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE
# emmake make -C build_emscripten_web_amalgamated
# mkdir -p bin/web_amalgamated
# cp build_emscripten_web_amalgamated/GifTools.js bin/web_amalgamated/GifTools.js

#
#
# Build for debugging.
#
#

# cmake -Bbuild_emscripten_web_amalgamated_ffmpegd -H. \
#     -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
#     -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_WEB_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN=TRUE \
#     -DGIFTOOLS_BUILD_FFMPEG=TRUE
# emmake make -C build_emscripten_web_amalgamated_ffmpegd
# mkdir -p bin/web_amalgamated_ffmpegd
# cp build_emscripten_web_amalgamated_ffmpegd/GifTools.js bin/web_amalgamated_ffmpegd/GifTools.js
# cp build_emscripten_web_amalgamated_ffmpegd/GifTools.wasm.map bin/web_amalgamated_ffmpegd/GifTools.wasm.map
# # emcc:WARNING: Wasm source map won't be usable in a browser without --source-map-base

#
#
# Run tests implicitly.
#
#

# npm run build
# npm run test
