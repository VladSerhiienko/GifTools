
source dependencies/emsdk/emsdk_env.sh
cmake -Bbuild_emscripten -H. -DCMAKE_TOOLCHAIN_FILE=$(pwd)/dependencies/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DGIFTOOLS_BUILD_EMSCRIPTEN=TRUE
emmake make -C build_emscripten
cp build_emscripten/GifTools.js bin/GifTools.js
cp build_emscripten/GifTools.wasm bin/GifTools.wasm

