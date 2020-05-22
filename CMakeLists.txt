
cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

if(NOT DEFINED GIFTOOLS_BUILD_EMSCRIPTEN)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
    set(GIFTOOLS_BUILD_EMSCRIPTEN FALSE)
endif()

project(GifTools)

message(STATUS "CMAKE_SYSTEM_INFO_FILE = ${CMAKE_SYSTEM_INFO_FILE}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SYSTEM = ${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")

if(NOT ${GIFTOOLS_BUILD_EMSCRIPTEN})
    message(STATUS "Adding Google Test externally.")
    
    set(default_cmake_args -G "Xcode")

    ExternalProject_Add(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        # "git@github.com:google/googletest.git"
        GIT_TAG "release-1.10.0"
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/googletest"
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CMAKE_ARGS ${default_cmake_args}
        TEST_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
    )

    ExternalProject_Get_Property(googletest SOURCE_DIR)
    ExternalProject_Get_Property(googletest BINARY_DIR)
    set(googletest_source_dir ${SOURCE_DIR})
    set(googletest_binary_dir ${BINARY_DIR})
    message(STATUS "googletest_source_dir = ${googletest_source_dir}")
    message(STATUS "googletest_binary_dir = ${googletest_binary_dir}")

endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(GIFTOOLS_USE_FFMPEG FALSE)
set(GIFTOOLS_USE_EMSDK FALSE)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=1")

if(NOT DEFINED GIFTOOLS_BUILD_FFMPEG)
    set(GIFTOOLS_BUILD_FFMPEG FALSE)
endif()

message(STATUS "Seeing GIFTOOLS_BUILD_EMSCRIPTEN=${GIFTOOLS_BUILD_EMSCRIPTEN}")

if(${GIFTOOLS_BUILD_EMSCRIPTEN})
    set(GIFTOOLS_USE_EMSDK TRUE)
endif()

if(${GIFTOOLS_BUILD_FFMPEG})
    set(GIFTOOLS_USE_FFMPEG TRUE)
endif()

message(STATUS "Seeing GIFTOOLS_USE_EMSDK=${GIFTOOLS_USE_EMSDK}")

if(NOT DEFINED GIFTOOLS_EMSDK_FOLDER)
    set(GIFTOOLS_EMSDK_FOLDER "${CMAKE_SOURCE_DIR}/dependencies/emsdk")
endif()

message(STATUS "Setting GIFTOOLS_EMSDK_FOLDER=${GIFTOOLS_EMSDK_FOLDER}")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)

if(${GIFTOOLS_USE_FFMPEG})
    message(STATUS "Seeing GIFTOOLS_USE_FFMPEG=TRUE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIFTOOLS_USE_FFMPEG=1")
endif()

# https://developers.google.com/web/updates/2019/01/emscripten-npm
if(${GIFTOOLS_USE_EMSDK})
    if(NOT DEFINED GIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN)
        set(GIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN FALSE)
    endif()
    if(NOT DEFINED GIFTOOLS_BUILD_ES6_EMSCRIPTEN)
        set(GIFTOOLS_BUILD_ES6_EMSCRIPTEN FALSE)
    endif()
    if(NOT DEFINED GIFTOOLS_BUILD_WEB_EMSCRIPTEN)
        set(GIFTOOLS_BUILD_WEB_EMSCRIPTEN FALSE)
    endif()
    if(NOT DEFINED GIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN)
        set(GIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN FALSE)
    endif()

    message(STATUS "Seeing GIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN=${GIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN}")

    # Enable Emscripten, enable embind.
    # -s STRICT=1 drops support for all deprecated build options.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIFTOOLS_EMSCRIPTEN=1 -s STRICT=1")

    # Enable WASM, enable functioning as library.
    # Enable dynamic memory allocations, use emmalloc (small and fast).
    # FILESYSTEM=0: Force Emscripten to not include emulated file system to the code.

    # --profiling
    # --profiling-funcs
    # --tracing
    # --emit-symbol-map
    # --js-opts=1
    # --llvm-opts=-O3
    # --llvm-lto=2
    # -s MALLOC=emmalloc \
    # -s INITIAL_MEMORY=50331648 \

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        --bind \
        --no-entry \
        --profiling \
        --profiling-funcs \
        --tracing \
        --emit-symbol-map \
        -s WASM=1 \
        -s FILESYSTEM=0 \
        -g4 \
        -s DEMANGLE_SUPPORT=1 \
        -s NO_EXIT_RUNTIME=1 \
        -s ABORTING_MALLOC=0 \
        -s MALLOC=emmalloc \
        -s ALLOW_MEMORY_GROWTH=1")

    # Set optimization flags
    if(${GIFTOOLS_BUILD_SMALLEST_EMSCRIPTEN})
        # -Os: Optimize aggressively for performance and size as a secondary criterion.
        message(STATUS "Building smallest WASM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -O3 \
            --js-opts 1 \
            --llvm-opts -O3 \
            -s ASSERTIONS=0 \
            -s STACK_OVERFLOW_CHECK=0 \
            -s SAFE_HEAP=0")
    else()
        # https://emscripten.org/docs/porting/Debugging.html
        # -O0: Don't do any optimization. No dead code is eliminated, and Emscripten does not minify the JavaScript code it emits, either. Good for debugging.
        # -g4: Include debugging information in the .wasm and also emit a source maps file for the wasm module.
        # ASSERTIONS=1: Enable runtime checks for common memory allocation errors.
        # SAFE_HEAP=1: Add additional memory access checks, and give clear errors for problems like dereferencing 0 and memory alignment issues.
        # STACK_OVERFLOW_CHECK=2: Verify that the user code does not accidentally write past the end of the stack.
        # DEMANGLE_SUPPORT=1: Automatically demangle stack traces.
        message(STATUS "Building debug/diagnostics WASM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -O0 \
            -s ASSERTIONS=1 \
            -s STACK_OVERFLOW_CHECK=2 \
            -s SAFE_HEAP_LOG=1 \
            -s SAFE_HEAP=1")
    endif()

    if(${GIFTOOLS_BUILD_ES6_EMSCRIPTEN})
        message(STATUS "Building web WASM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s EXPORT_ES6=1")
    endif()

    if(${GIFTOOLS_BUILD_WEB_EMSCRIPTEN})
        message(STATUS "Building web WASM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s ENVIRONMENT='web'")
    endif()

    # merges JavaScript and WebAssembly code in the single output file output
    # (in base64) to produce only one file for deployment.
    if(${GIFTOOLS_BUILD_SINGLE_FILE_EMSCRIPTEN})
        message(STATUS "Building single file WASM")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s SINGLE_FILE=1")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
        -s MODULARIZE=1 \
        -s 'EXPORT_NAME=\"GifToolsModularized\"'")
endif()

include_directories(
    FFmpeg
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/lib/zlib
)

set(GIFTOOLS_FFMPEG_SOURCES )

if (${GIFTOOLS_USE_FFMPEG})
    set(GIFTOOLS_FFMPEG_SOURCES
        ./lib/zlib/inflate.c
        ./lib/zlib/compress.c
        ./lib/zlib/deflate.c
        ./lib/zlib/crc32.c
        ./lib/zlib/infback.c
        ./lib/zlib/zutil.c
        ./lib/zlib/inftrees.c
        ./lib/zlib/uncompr.c
        ./lib/zlib/trees.c
        ./lib/zlib/inffast.c
        ./lib/zlib/adler32.c
        # ./lib/zlib/gzwrite.c
        # ./lib/zlib/gzread.c
        # ./lib/zlib/gzlib.c
        # ./lib/zlib/gzclose.c
        ./FFmpeg/compat/atomics/pthread/stdatomic.c
        ./FFmpeg/compat/getopt.c
        ./FFmpeg/compat/strtod.c
        # ./FFmpeg/compat/msvcrt/snprintf.c
        # ./FFmpeg/doc/examples/avio_dir_cmd.c
        # ./FFmpeg/doc/examples/avio_reading.c
        # ./FFmpeg/doc/examples/decode_audio.c
        # ./FFmpeg/doc/examples/decode_video.c
        # ./FFmpeg/doc/examples/demuxing_decoding.c
        # ./FFmpeg/doc/examples/encode_audio.c
        # ./FFmpeg/doc/examples/encode_video.c
        # ./FFmpeg/doc/examples/extract_mvs.c
        # ./FFmpeg/doc/examples/filter_audio.c
        # ./FFmpeg/doc/examples/filtering_audio.c
        # ./FFmpeg/doc/examples/filtering_video.c
        # ./FFmpeg/doc/examples/http_multiclient.c
        # ./FFmpeg/doc/examples/hw_decode.c
        # ./FFmpeg/doc/examples/metadata.c
        # ./FFmpeg/doc/examples/muxing.c
        # ./FFmpeg/doc/examples/qsvdec.c
        # ./FFmpeg/doc/examples/remuxing.c
        # ./FFmpeg/doc/examples/resampling_audio.c
        # ./FFmpeg/doc/examples/scaling_video.c
        # ./FFmpeg/doc/examples/transcode_aac.c
        # ./FFmpeg/doc/examples/transcoding.c
        # ./FFmpeg/doc/examples/vaapi_encode.c
        # ./FFmpeg/doc/examples/vaapi_transcode.c
        # ./FFmpeg/doc/print_options.c
        # ./FFmpeg/fftools/cmdutils.c

        ## Cmd line tool
        # ./FFmpeg/fftools/ffmpeg.c
        # ./FFmpeg/fftools/ffmpeg_cuvid.c
        # ./FFmpeg/fftools/ffmpeg_filter.c
        # ./FFmpeg/fftools/ffmpeg_hw.c
        # ./FFmpeg/fftools/ffmpeg_opt.c
        # ./FFmpeg/fftools/ffmpeg_qsv.c
        # ./FFmpeg/fftools/ffmpeg_videotoolbox.c
        # ./FFmpeg/fftools/ffplay.c
        # ./FFmpeg/fftools/ffprobe.c

        ./FFmpeg/libavcodec/012v.c
        ./FFmpeg/libavcodec/4xm.c
        ./FFmpeg/libavcodec/8bps.c
        ./FFmpeg/libavcodec/8svx.c
        ./FFmpeg/libavcodec/a64multienc.c
        ./FFmpeg/libavcodec/aac_ac3_parser.c
        ./FFmpeg/libavcodec/aac_adtstoasc_bsf.c
        ./FFmpeg/libavcodec/aac_parser.c
        ./FFmpeg/libavcodec/aaccoder.c
        ./FFmpeg/libavcodec/aacdec.c
        ./FFmpeg/libavcodec/aacdec_fixed.c
        # ./FFmpeg/libavcodec/aacdec_template.c
        ./FFmpeg/libavcodec/aacenc.c
        ./FFmpeg/libavcodec/aacenc_is.c
        ./FFmpeg/libavcodec/aacenc_ltp.c
        ./FFmpeg/libavcodec/aacenc_pred.c
        ./FFmpeg/libavcodec/aacenc_tns.c
        ./FFmpeg/libavcodec/aacenctab.c
        ./FFmpeg/libavcodec/aacps.c
        ./FFmpeg/libavcodec/aacps_fixed.c
        # ./FFmpeg/libavcodec/aacps_fixed_tablegen.c
        # ./FFmpeg/libavcodec/aacps_float.c
        # ./FFmpeg/libavcodec/aacps_tablegen.c
        # ./FFmpeg/libavcodec/aacps_tablegen_template.c
        # ./FFmpeg/libavcodec/aacpsdata.c
        ./FFmpeg/libavcodec/aacpsdsp_fixed.c
        ./FFmpeg/libavcodec/aacpsdsp_float.c
        # ./FFmpeg/libavcodec/aacpsdsp_template.c
        ./FFmpeg/libavcodec/aacpsy.c
        ./FFmpeg/libavcodec/aacsbr.c
        ./FFmpeg/libavcodec/aacsbr_fixed.c
        # ./FFmpeg/libavcodec/aacsbr_template.c
        ./FFmpeg/libavcodec/aactab.c
        ./FFmpeg/libavcodec/aandcttab.c
        # ./FFmpeg/libavcodec/aarch64/aacpsdsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/fft_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/fmtconvert_init.c
        # ./FFmpeg/libavcodec/aarch64/h264chroma_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/h264dsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/h264pred_init.c
        # ./FFmpeg/libavcodec/aarch64/h264qpel_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/hpeldsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/idctdsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/mpegaudiodsp_init.c
        # ./FFmpeg/libavcodec/aarch64/neontest.c
        # ./FFmpeg/libavcodec/aarch64/rv40dsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/sbrdsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/synth_filter_init.c
        # ./FFmpeg/libavcodec/aarch64/vc1dsp_init_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/videodsp_init.c
        # ./FFmpeg/libavcodec/aarch64/vorbisdsp_init.c
        # ./FFmpeg/libavcodec/aarch64/vp9dsp_init_10bpp_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/vp9dsp_init_12bpp_aarch64.c
        # ./FFmpeg/libavcodec/aarch64/vp9dsp_init_16bpp_aarch64_template.c
        # ./FFmpeg/libavcodec/aarch64/vp9dsp_init_aarch64.c
        ./FFmpeg/libavcodec/aasc.c
        ./FFmpeg/libavcodec/ac3.c
        ./FFmpeg/libavcodec/ac3_parser.c
        # ./FFmpeg/libavcodec/ac3dec.c
        ./FFmpeg/libavcodec/ac3dec_data.c
        ./FFmpeg/libavcodec/ac3dec_fixed.c
        ./FFmpeg/libavcodec/ac3dec_float.c
        ./FFmpeg/libavcodec/ac3dsp.c
        ./FFmpeg/libavcodec/ac3enc.c
        ./FFmpeg/libavcodec/ac3enc_fixed.c
        ./FFmpeg/libavcodec/ac3enc_float.c
        # ./FFmpeg/libavcodec/ac3enc_opts_template.c
        # ./FFmpeg/libavcodec/ac3enc_template.c
        ./FFmpeg/libavcodec/ac3tab.c
        ./FFmpeg/libavcodec/acelp_filters.c
        ./FFmpeg/libavcodec/acelp_pitch_delay.c
        ./FFmpeg/libavcodec/acelp_vectors.c
        ./FFmpeg/libavcodec/adpcm.c
        ./FFmpeg/libavcodec/adpcm_data.c
        ./FFmpeg/libavcodec/adpcmenc.c
        ./FFmpeg/libavcodec/adts_header.c
        ./FFmpeg/libavcodec/adts_parser.c
        ./FFmpeg/libavcodec/adx.c
        ./FFmpeg/libavcodec/adx_parser.c
        ./FFmpeg/libavcodec/adxdec.c
        ./FFmpeg/libavcodec/adxenc.c
        ./FFmpeg/libavcodec/aic.c
        ./FFmpeg/libavcodec/alac.c
        ./FFmpeg/libavcodec/alac_data.c
        ./FFmpeg/libavcodec/alacdsp.c
        ./FFmpeg/libavcodec/alacenc.c
        ./FFmpeg/libavcodec/aliaspixdec.c
        ./FFmpeg/libavcodec/aliaspixenc.c
        ./FFmpeg/libavcodec/allcodecs.c
        # ./FFmpeg/libavcodec/alpha/blockdsp_alpha.c
        # ./FFmpeg/libavcodec/alpha/hpeldsp_alpha.c
        # ./FFmpeg/libavcodec/alpha/idctdsp_alpha.c
        # ./FFmpeg/libavcodec/alpha/me_cmp_alpha.c
        # ./FFmpeg/libavcodec/alpha/mpegvideo_alpha.c
        # ./FFmpeg/libavcodec/alpha/pixblockdsp_alpha.c
        # ./FFmpeg/libavcodec/alpha/simple_idct_alpha.c
        ./FFmpeg/libavcodec/alsdec.c
        # ./FFmpeg/libavcodec/amfenc.c
        # ./FFmpeg/libavcodec/amfenc_h264.c
        # ./FFmpeg/libavcodec/amfenc_hevc.c
        ./FFmpeg/libavcodec/amrnbdec.c
        ./FFmpeg/libavcodec/amrwbdec.c
        ./FFmpeg/libavcodec/anm.c
        ./FFmpeg/libavcodec/ansi.c
        ./FFmpeg/libavcodec/apedec.c
        ./FFmpeg/libavcodec/aptx.c
        # ./FFmpeg/libavcodec/arm/aacpsdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/ac3dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/audiodsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/audiodsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/blockdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/blockdsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/fft_fixed_init_arm.c
        # ./FFmpeg/libavcodec/arm/fft_init_arm.c
        # ./FFmpeg/libavcodec/arm/flacdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/fmtconvert_init_arm.c
        # ./FFmpeg/libavcodec/arm/g722dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/h264chroma_init_arm.c
        # ./FFmpeg/libavcodec/arm/h264dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/h264pred_init_arm.c
        # ./FFmpeg/libavcodec/arm/h264qpel_init_arm.c
        # ./FFmpeg/libavcodec/arm/hevcdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/hevcdsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/hpeldsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/hpeldsp_init_armv6.c
        # ./FFmpeg/libavcodec/arm/hpeldsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/idctdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/idctdsp_init_armv5te.c
        # ./FFmpeg/libavcodec/arm/idctdsp_init_armv6.c
        # ./FFmpeg/libavcodec/arm/idctdsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/lossless_audiodsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/me_cmp_init_arm.c
        # ./FFmpeg/libavcodec/arm/mlpdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/mpegaudiodsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/mpegvideo_arm.c
        # ./FFmpeg/libavcodec/arm/mpegvideo_armv5te.c
        # ./FFmpeg/libavcodec/arm/mpegvideoencdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/neontest.c
        # ./FFmpeg/libavcodec/arm/pixblockdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/rdft_init_arm.c
        # ./FFmpeg/libavcodec/arm/rv34dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/rv40dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/sbcdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/sbrdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/synth_filter_init_arm.c
        # ./FFmpeg/libavcodec/arm/vc1dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/vc1dsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/videodsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/videodsp_init_armv5te.c
        # ./FFmpeg/libavcodec/arm/vorbisdsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/vp3dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/vp6dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/vp8dsp_init_arm.c
        # ./FFmpeg/libavcodec/arm/vp8dsp_init_armv6.c
        # ./FFmpeg/libavcodec/arm/vp8dsp_init_neon.c
        # ./FFmpeg/libavcodec/arm/vp9dsp_init_10bpp_arm.c
        # ./FFmpeg/libavcodec/arm/vp9dsp_init_12bpp_arm.c
        # ./FFmpeg/libavcodec/arm/vp9dsp_init_16bpp_arm_template.c
        # ./FFmpeg/libavcodec/arm/vp9dsp_init_arm.c
        ./FFmpeg/libavcodec/ass.c
        ./FFmpeg/libavcodec/ass_split.c
        ./FFmpeg/libavcodec/assdec.c
        ./FFmpeg/libavcodec/assenc.c
        ./FFmpeg/libavcodec/asv.c
        ./FFmpeg/libavcodec/asvdec.c
        ./FFmpeg/libavcodec/asvenc.c
        ./FFmpeg/libavcodec/atrac.c
        ./FFmpeg/libavcodec/atrac1.c
        ./FFmpeg/libavcodec/atrac3.c
        ./FFmpeg/libavcodec/atrac3plus.c
        ./FFmpeg/libavcodec/atrac3plusdec.c
        ./FFmpeg/libavcodec/atrac3plusdsp.c
        ./FFmpeg/libavcodec/atrac9dec.c
        ./FFmpeg/libavcodec/audio_frame_queue.c
        ./FFmpeg/libavcodec/audiodsp.c
        # ./FFmpeg/libavcodec/audiotoolboxdec.c
        # ./FFmpeg/libavcodec/audiotoolboxenc.c
        ./FFmpeg/libavcodec/aura.c
        ./FFmpeg/libavcodec/av1_metadata_bsf.c
        ./FFmpeg/libavcodec/av1_parse.c
        ./FFmpeg/libavcodec/av1_parser.c
        ./FFmpeg/libavcodec/avdct.c
        ./FFmpeg/libavcodec/avfft.c
        ./FFmpeg/libavcodec/avpacket.c
        ./FFmpeg/libavcodec/avpicture.c
        ./FFmpeg/libavcodec/avrndec.c
        ./FFmpeg/libavcodec/avs.c
        ./FFmpeg/libavcodec/avs2_parser.c
        ./FFmpeg/libavcodec/avuidec.c
        ./FFmpeg/libavcodec/avuienc.c
        ./FFmpeg/libavcodec/bethsoftvideo.c
        ./FFmpeg/libavcodec/bfi.c
        ./FFmpeg/libavcodec/bgmc.c
        ./FFmpeg/libavcodec/bink.c
        ./FFmpeg/libavcodec/binkaudio.c
        ./FFmpeg/libavcodec/binkdsp.c
        ./FFmpeg/libavcodec/bintext.c
        ./FFmpeg/libavcodec/bit_depth_template.c
        ./FFmpeg/libavcodec/bitpacked.c
        ./FFmpeg/libavcodec/bitstream.c
        ./FFmpeg/libavcodec/bitstream_filter.c
        ./FFmpeg/libavcodec/bitstream_filters.c
        ./FFmpeg/libavcodec/blockdsp.c
        ./FFmpeg/libavcodec/bmp.c
        ./FFmpeg/libavcodec/bmp_parser.c
        ./FFmpeg/libavcodec/bmpenc.c
        ./FFmpeg/libavcodec/bmvaudio.c
        ./FFmpeg/libavcodec/bmvvideo.c
        ./FFmpeg/libavcodec/brenderpix.c
        ./FFmpeg/libavcodec/bsf.c
        # ./FFmpeg/libavcodec/bsf_list.c
        ./FFmpeg/libavcodec/bswapdsp.c
        ./FFmpeg/libavcodec/c93.c
        ./FFmpeg/libavcodec/cabac.c
        ./FFmpeg/libavcodec/canopus.c
        ./FFmpeg/libavcodec/cavs.c
        ./FFmpeg/libavcodec/cavs_parser.c
        ./FFmpeg/libavcodec/cavsdata.c
        ./FFmpeg/libavcodec/cavsdec.c
        ./FFmpeg/libavcodec/cavsdsp.c
        ./FFmpeg/libavcodec/cbrt_data.c
        ./FFmpeg/libavcodec/cbrt_data_fixed.c
        # ./FFmpeg/libavcodec/cbrt_fixed_tablegen.c
        # ./FFmpeg/libavcodec/cbrt_tablegen.c
        # ./FFmpeg/libavcodec/cbrt_tablegen_template.c
        ./FFmpeg/libavcodec/cbs.c
        ./FFmpeg/libavcodec/cbs_av1.c
        # ./FFmpeg/libavcodec/cbs_av1_syntax_template.c
        ./FFmpeg/libavcodec/cbs_h2645.c
        # ./FFmpeg/libavcodec/cbs_h264_syntax_template.c
        # ./FFmpeg/libavcodec/cbs_h265_syntax_template.c
        ./FFmpeg/libavcodec/cbs_jpeg.c
        # ./FFmpeg/libavcodec/cbs_jpeg_syntax_template.c
        ./FFmpeg/libavcodec/cbs_mpeg2.c
        # ./FFmpeg/libavcodec/cbs_mpeg2_syntax_template.c
        ./FFmpeg/libavcodec/cbs_vp9.c
        # ./FFmpeg/libavcodec/cbs_vp9_syntax_template.c
        ./FFmpeg/libavcodec/ccaption_dec.c
        ./FFmpeg/libavcodec/cdgraphics.c
        ./FFmpeg/libavcodec/cdxl.c
        ./FFmpeg/libavcodec/celp_filters.c
        ./FFmpeg/libavcodec/celp_math.c
        ./FFmpeg/libavcodec/cfhd.c
        ./FFmpeg/libavcodec/cfhddata.c
        ./FFmpeg/libavcodec/cga_data.c
        ./FFmpeg/libavcodec/chomp_bsf.c
        ./FFmpeg/libavcodec/cinepak.c
        ./FFmpeg/libavcodec/cinepakenc.c
        ./FFmpeg/libavcodec/clearvideo.c
        ./FFmpeg/libavcodec/cljrdec.c
        ./FFmpeg/libavcodec/cljrenc.c
        ./FFmpeg/libavcodec/cllc.c
        ./FFmpeg/libavcodec/cngdec.c
        ./FFmpeg/libavcodec/cngenc.c
        # ./FFmpeg/libavcodec/codec2utils.c
        ./FFmpeg/libavcodec/codec_desc.c
        # ./FFmpeg/libavcodec/codec_list.c
        ./FFmpeg/libavcodec/cook.c
        ./FFmpeg/libavcodec/cook_parser.c
        # ./FFmpeg/libavcodec/cos_tablegen.c
        ./FFmpeg/libavcodec/cpia.c
        # ./FFmpeg/libavcodec/crystalhd.c
        ./FFmpeg/libavcodec/cscd.c
        # ./FFmpeg/libavcodec/cuviddec.c
        ./FFmpeg/libavcodec/cyuv.c
        ./FFmpeg/libavcodec/d3d11va.c
        ./FFmpeg/libavcodec/dca.c
        ./FFmpeg/libavcodec/dca_core.c
        ./FFmpeg/libavcodec/dca_core_bsf.c
        ./FFmpeg/libavcodec/dca_exss.c
        ./FFmpeg/libavcodec/dca_lbr.c
        ./FFmpeg/libavcodec/dca_parser.c
        ./FFmpeg/libavcodec/dca_xll.c
        ./FFmpeg/libavcodec/dcaadpcm.c
        ./FFmpeg/libavcodec/dcadata.c
        ./FFmpeg/libavcodec/dcadct.c
        ./FFmpeg/libavcodec/dcadec.c
        ./FFmpeg/libavcodec/dcadsp.c
        ./FFmpeg/libavcodec/dcaenc.c
        ./FFmpeg/libavcodec/dcahuff.c
        ./FFmpeg/libavcodec/dct.c
        ./FFmpeg/libavcodec/dct32_fixed.c
        ./FFmpeg/libavcodec/dct32_float.c
        # ./FFmpeg/libavcodec/dct32_template.c
        ./FFmpeg/libavcodec/dctref.c
        ./FFmpeg/libavcodec/dds.c
        ./FFmpeg/libavcodec/decode.c
        ./FFmpeg/libavcodec/dfa.c
        ./FFmpeg/libavcodec/dirac.c
        # ./FFmpeg/libavcodec/dirac_arith.c
        # ./FFmpeg/libavcodec/dirac_dwt.c
        # ./FFmpeg/libavcodec/dirac_dwt_template.c
        # ./FFmpeg/libavcodec/dirac_parser.c
        # ./FFmpeg/libavcodec/dirac_vlc.c
        # ./FFmpeg/libavcodec/diracdec.c
        # ./FFmpeg/libavcodec/diracdsp.c
        ./FFmpeg/libavcodec/diractab.c
        ./FFmpeg/libavcodec/dnxhd_parser.c
        ./FFmpeg/libavcodec/dnxhddata.c
        ./FFmpeg/libavcodec/dnxhddec.c
        ./FFmpeg/libavcodec/dnxhdenc.c
        ./FFmpeg/libavcodec/dolby_e.c
        ./FFmpeg/libavcodec/dpcm.c
        ./FFmpeg/libavcodec/dpx.c
        ./FFmpeg/libavcodec/dpx_parser.c
        ./FFmpeg/libavcodec/dpxenc.c
        ./FFmpeg/libavcodec/dsd.c
        ./FFmpeg/libavcodec/dsddec.c
        ./FFmpeg/libavcodec/dsicinaudio.c
        ./FFmpeg/libavcodec/dsicinvideo.c
        ./FFmpeg/libavcodec/dss_sp.c
        ./FFmpeg/libavcodec/dstdec.c
        ./FFmpeg/libavcodec/dump_extradata_bsf.c
        ./FFmpeg/libavcodec/dv.c
        ./FFmpeg/libavcodec/dv_profile.c
        # ./FFmpeg/libavcodec/dv_tablegen.c
        ./FFmpeg/libavcodec/dvaudio_parser.c
        ./FFmpeg/libavcodec/dvaudiodec.c
        ./FFmpeg/libavcodec/dvbsub.c
        ./FFmpeg/libavcodec/dvbsub_parser.c
        ./FFmpeg/libavcodec/dvbsubdec.c
        ./FFmpeg/libavcodec/dvd_nav_parser.c
        ./FFmpeg/libavcodec/dvdata.c
        ./FFmpeg/libavcodec/dvdec.c
        ./FFmpeg/libavcodec/dvdsub_parser.c
        ./FFmpeg/libavcodec/dvdsubdec.c
        ./FFmpeg/libavcodec/dvdsubenc.c
        ./FFmpeg/libavcodec/dvenc.c
        ./FFmpeg/libavcodec/dxa.c
        ./FFmpeg/libavcodec/dxtory.c
        # ./FFmpeg/libavcodec/dxv.c
        # ./FFmpeg/libavcodec/dxva2.c
        # ./FFmpeg/libavcodec/dxva2_h264.c
        # ./FFmpeg/libavcodec/dxva2_hevc.c
        # ./FFmpeg/libavcodec/dxva2_mpeg2.c
        # ./FFmpeg/libavcodec/dxva2_vc1.c
        # ./FFmpeg/libavcodec/dxva2_vp9.c
        ./FFmpeg/libavcodec/eac3_core_bsf.c
        ./FFmpeg/libavcodec/eac3_data.c
        ./FFmpeg/libavcodec/eac3dec.c
        ./FFmpeg/libavcodec/eac3enc.c
        ./FFmpeg/libavcodec/eacmv.c
        ./FFmpeg/libavcodec/eaidct.c
        ./FFmpeg/libavcodec/eamad.c
        ./FFmpeg/libavcodec/eatgq.c
        ./FFmpeg/libavcodec/eatgv.c
        ./FFmpeg/libavcodec/eatqi.c
        ./FFmpeg/libavcodec/elbg.c
        ./FFmpeg/libavcodec/elsdec.c
        ./FFmpeg/libavcodec/encode.c
        ./FFmpeg/libavcodec/error_resilience.c
        ./FFmpeg/libavcodec/escape124.c
        ./FFmpeg/libavcodec/escape130.c
        ./FFmpeg/libavcodec/evrcdec.c
        ./FFmpeg/libavcodec/exif.c
        ./FFmpeg/libavcodec/exr.c
        ./FFmpeg/libavcodec/exrdsp.c
        ./FFmpeg/libavcodec/extract_extradata_bsf.c
        ./FFmpeg/libavcodec/faandct.c
        ./FFmpeg/libavcodec/faanidct.c
        ./FFmpeg/libavcodec/faxcompr.c
        ./FFmpeg/libavcodec/fdctdsp.c
        # ./FFmpeg/libavcodec/ffjni.c
        ./FFmpeg/libavcodec/fft_fixed.c
        ./FFmpeg/libavcodec/fft_fixed_32.c
        ./FFmpeg/libavcodec/fft_float.c
        ./FFmpeg/libavcodec/fft_init_table.c
        # ./FFmpeg/libavcodec/fft_template.c
        ./FFmpeg/libavcodec/ffv1.c
        # ./FFmpeg/libavcodec/ffv1_template.c
        ./FFmpeg/libavcodec/ffv1dec.c
        # ./FFmpeg/libavcodec/ffv1dec_template.c
        ./FFmpeg/libavcodec/ffv1enc.c
        # ./FFmpeg/libavcodec/ffv1enc_template.c
        ./FFmpeg/libavcodec/ffwavesynth.c
        ./FFmpeg/libavcodec/fic.c
        # ./FFmpeg/libavcodec/file_open.c
        ./FFmpeg/libavcodec/filter_units_bsf.c
        ./FFmpeg/libavcodec/fits.c
        ./FFmpeg/libavcodec/fitsdec.c
        ./FFmpeg/libavcodec/fitsenc.c
        ./FFmpeg/libavcodec/flac.c
        ./FFmpeg/libavcodec/flac_parser.c
        ./FFmpeg/libavcodec/flacdata.c
        ./FFmpeg/libavcodec/flacdec.c
        ./FFmpeg/libavcodec/flacdsp.c
        # ./FFmpeg/libavcodec/flacdsp_lpc_template.c
        # ./FFmpeg/libavcodec/flacdsp_template.c
        ./FFmpeg/libavcodec/flacenc.c
        ./FFmpeg/libavcodec/flashsv.c
        ./FFmpeg/libavcodec/flashsv2enc.c
        ./FFmpeg/libavcodec/flashsvenc.c
        ./FFmpeg/libavcodec/flicvideo.c
        ./FFmpeg/libavcodec/flvdec.c
        ./FFmpeg/libavcodec/flvenc.c
        ./FFmpeg/libavcodec/fmtconvert.c
        ./FFmpeg/libavcodec/fmvc.c
        ./FFmpeg/libavcodec/frame_thread_encoder.c
        ./FFmpeg/libavcodec/fraps.c
        ./FFmpeg/libavcodec/frwu.c
        ./FFmpeg/libavcodec/g2meet.c
        ./FFmpeg/libavcodec/g722.c
        ./FFmpeg/libavcodec/g722dec.c
        ./FFmpeg/libavcodec/g722dsp.c
        ./FFmpeg/libavcodec/g722enc.c
        ./FFmpeg/libavcodec/g723_1.c
        ./FFmpeg/libavcodec/g723_1dec.c
        ./FFmpeg/libavcodec/g723_1enc.c
        ./FFmpeg/libavcodec/g726.c
        ./FFmpeg/libavcodec/g729_parser.c
        ./FFmpeg/libavcodec/g729dec.c
        ./FFmpeg/libavcodec/g729postfilter.c
        ./FFmpeg/libavcodec/gdv.c
        ./FFmpeg/libavcodec/gif.c
        ./FFmpeg/libavcodec/gifdec.c
        ./FFmpeg/libavcodec/golomb.c
        ./FFmpeg/libavcodec/gsm_parser.c
        ./FFmpeg/libavcodec/gsmdec.c
        ./FFmpeg/libavcodec/gsmdec_data.c
        # ./FFmpeg/libavcodec/gsmdec_template.c
        ./FFmpeg/libavcodec/h261.c
        ./FFmpeg/libavcodec/h261_parser.c
        ./FFmpeg/libavcodec/h261data.c
        ./FFmpeg/libavcodec/h261dec.c
        ./FFmpeg/libavcodec/h261enc.c
        ./FFmpeg/libavcodec/h263.c
        ./FFmpeg/libavcodec/h263_parser.c
        ./FFmpeg/libavcodec/h263data.c
        ./FFmpeg/libavcodec/h263dec.c
        ./FFmpeg/libavcodec/h263dsp.c
        ./FFmpeg/libavcodec/h2645_parse.c
        ./FFmpeg/libavcodec/h264_cabac.c
        ./FFmpeg/libavcodec/h264_cavlc.c
        ./FFmpeg/libavcodec/h264_direct.c
        ./FFmpeg/libavcodec/h264_levels.c
        ./FFmpeg/libavcodec/h264_loopfilter.c
        ./FFmpeg/libavcodec/h264_mb.c
        # ./FFmpeg/libavcodec/h264_mb_template.c
        # ./FFmpeg/libavcodec/h264_mc_template.c
        ./FFmpeg/libavcodec/h264_metadata_bsf.c
        ./FFmpeg/libavcodec/h264_mp4toannexb_bsf.c
        ./FFmpeg/libavcodec/h264_parse.c
        ./FFmpeg/libavcodec/h264_parser.c
        ./FFmpeg/libavcodec/h264_picture.c
        ./FFmpeg/libavcodec/h264_ps.c
        ./FFmpeg/libavcodec/h264_redundant_pps_bsf.c
        ./FFmpeg/libavcodec/h264_refs.c
        ./FFmpeg/libavcodec/h264_sei.c
        ./FFmpeg/libavcodec/h264_slice.c
        # ./FFmpeg/libavcodec/h264addpx_template.c
        ./FFmpeg/libavcodec/h264chroma.c
        # ./FFmpeg/libavcodec/h264chroma_template.c
        ./FFmpeg/libavcodec/h264data.c
        ./FFmpeg/libavcodec/h264dec.c
        ./FFmpeg/libavcodec/h264dsp.c
        # ./FFmpeg/libavcodec/h264dsp_template.c
        ./FFmpeg/libavcodec/h264idct.c
        # ./FFmpeg/libavcodec/h264idct_template.c
        ./FFmpeg/libavcodec/h264pred.c
        # ./FFmpeg/libavcodec/h264pred_template.c
        ./FFmpeg/libavcodec/h264qpel.c
        # ./FFmpeg/libavcodec/h264qpel_template.c
        ./FFmpeg/libavcodec/h265_metadata_bsf.c
        ./FFmpeg/libavcodec/h265_profile_level.c
        ./FFmpeg/libavcodec/hap.c
        # ./FFmpeg/libavcodec/hapdec.c
        # ./FFmpeg/libavcodec/hapenc.c
        ./FFmpeg/libavcodec/hapqa_extract_bsf.c
        ./FFmpeg/libavcodec/hevc_cabac.c
        ./FFmpeg/libavcodec/hevc_data.c
        ./FFmpeg/libavcodec/hevc_filter.c
        ./FFmpeg/libavcodec/hevc_mp4toannexb_bsf.c
        ./FFmpeg/libavcodec/hevc_mvs.c
        ./FFmpeg/libavcodec/hevc_parse.c
        ./FFmpeg/libavcodec/hevc_parser.c
        ./FFmpeg/libavcodec/hevc_ps.c
        ./FFmpeg/libavcodec/hevc_ps_enc.c
        ./FFmpeg/libavcodec/hevc_refs.c
        ./FFmpeg/libavcodec/hevc_sei.c
        ./FFmpeg/libavcodec/hevcdec.c
        ./FFmpeg/libavcodec/hevcdsp.c
        # ./FFmpeg/libavcodec/hevcdsp_template.c
        ./FFmpeg/libavcodec/hevcpred.c
        # ./FFmpeg/libavcodec/hevcpred_template.c
        ./FFmpeg/libavcodec/hnm4video.c
        # ./FFmpeg/libavcodec/hpel_template.c
        ./FFmpeg/libavcodec/hpeldsp.c
        ./FFmpeg/libavcodec/hq_hqa.c
        ./FFmpeg/libavcodec/hq_hqadata.c
        ./FFmpeg/libavcodec/hq_hqadsp.c
        ./FFmpeg/libavcodec/hqx.c
        ./FFmpeg/libavcodec/hqxdsp.c
        ./FFmpeg/libavcodec/hqxvlc.c
        ./FFmpeg/libavcodec/htmlsubtitles.c
        ./FFmpeg/libavcodec/huffman.c
        ./FFmpeg/libavcodec/huffyuv.c
        ./FFmpeg/libavcodec/huffyuvdec.c
        ./FFmpeg/libavcodec/huffyuvdsp.c
        ./FFmpeg/libavcodec/huffyuvenc.c
        ./FFmpeg/libavcodec/huffyuvencdsp.c
        ./FFmpeg/libavcodec/idcinvideo.c
        ./FFmpeg/libavcodec/idctdsp.c
        ./FFmpeg/libavcodec/iff.c
        ./FFmpeg/libavcodec/iirfilter.c
        # ./FFmpeg/libavcodec/ilbcdec.c
        ./FFmpeg/libavcodec/imc.c
        ./FFmpeg/libavcodec/imgconvert.c
        ./FFmpeg/libavcodec/imm4.c
        ./FFmpeg/libavcodec/imx_dump_header_bsf.c
        ./FFmpeg/libavcodec/indeo2.c
        ./FFmpeg/libavcodec/indeo3.c
        ./FFmpeg/libavcodec/indeo4.c
        ./FFmpeg/libavcodec/indeo5.c
        ./FFmpeg/libavcodec/intelh263dec.c
        ./FFmpeg/libavcodec/interplayacm.c
        ./FFmpeg/libavcodec/interplayvideo.c
        ./FFmpeg/libavcodec/intrax8.c
        ./FFmpeg/libavcodec/intrax8dsp.c
        ./FFmpeg/libavcodec/ituh263dec.c
        ./FFmpeg/libavcodec/ituh263enc.c
        ./FFmpeg/libavcodec/ivi.c
        ./FFmpeg/libavcodec/ivi_dsp.c
        ./FFmpeg/libavcodec/j2kenc.c
        ./FFmpeg/libavcodec/jacosubdec.c
        ./FFmpeg/libavcodec/jfdctfst.c
        ./FFmpeg/libavcodec/jfdctint.c
        # ./FFmpeg/libavcodec/jfdctint_template.c
        # ./FFmpeg/libavcodec/jni.c
        ./FFmpeg/libavcodec/jpeg2000.c
        ./FFmpeg/libavcodec/jpeg2000dec.c
        ./FFmpeg/libavcodec/jpeg2000dsp.c
        ./FFmpeg/libavcodec/jpeg2000dwt.c
        ./FFmpeg/libavcodec/jpegls.c
        ./FFmpeg/libavcodec/jpeglsdec.c
        ./FFmpeg/libavcodec/jpeglsenc.c
        ./FFmpeg/libavcodec/jpegtables.c
        ./FFmpeg/libavcodec/jrevdct.c
        ./FFmpeg/libavcodec/jvdec.c
        ./FFmpeg/libavcodec/kbdwin.c
        ./FFmpeg/libavcodec/kgv1dec.c
        ./FFmpeg/libavcodec/kmvc.c
        ./FFmpeg/libavcodec/lagarith.c
        ./FFmpeg/libavcodec/lagarithrac.c
        ./FFmpeg/libavcodec/latm_parser.c
        ./FFmpeg/libavcodec/lcldec.c
        ./FFmpeg/libavcodec/lclenc.c
        # ./FFmpeg/libavcodec/libaomdec.c
        # ./FFmpeg/libavcodec/libaomenc.c
        # ./FFmpeg/libavcodec/libcelt_dec.c
        # ./FFmpeg/libavcodec/libcodec2.c
        # ./FFmpeg/libavcodec/libdavs2.c
        # ./FFmpeg/libavcodec/libfdk-aacdec.c
        # ./FFmpeg/libavcodec/libfdk-aacenc.c
        # ./FFmpeg/libavcodec/libgsmdec.c
        # ./FFmpeg/libavcodec/libgsmenc.c
        # ./FFmpeg/libavcodec/libilbc.c
        # ./FFmpeg/libavcodec/libkvazaar.c
        # ./FFmpeg/libavcodec/libmp3lame.c
        ./FFmpeg/libavcodec/libopencore-amr.c
        # ./FFmpeg/libavcodec/libopenh264.c
        # ./FFmpeg/libavcodec/libopenh264dec.c
        # ./FFmpeg/libavcodec/libopenh264enc.c
        # ./FFmpeg/libavcodec/libopenjpegdec.c
        # ./FFmpeg/libavcodec/libopenjpegenc.c
        # ./FFmpeg/libavcodec/libopus.c
        # ./FFmpeg/libavcodec/libopusdec.c
        # ./FFmpeg/libavcodec/libopusenc.c
        # ./FFmpeg/libavcodec/librsvgdec.c
        # ./FFmpeg/libavcodec/libshine.c
        # ./FFmpeg/libavcodec/libspeexdec.c
        # ./FFmpeg/libavcodec/libspeexenc.c
        # ./FFmpeg/libavcodec/libtheoraenc.c
        # ./FFmpeg/libavcodec/libtwolame.c
        # ./FFmpeg/libavcodec/libvo-amrwbenc.c
        # ./FFmpeg/libavcodec/libvorbisdec.c
        # ./FFmpeg/libavcodec/libvorbisenc.c
        # ./FFmpeg/libavcodec/libvpx.c
        # ./FFmpeg/libavcodec/libvpxdec.c
        # ./FFmpeg/libavcodec/libvpxenc.c
        # ./FFmpeg/libavcodec/libwavpackenc.c
        # ./FFmpeg/libavcodec/libwebpenc.c
        # ./FFmpeg/libavcodec/libwebpenc_animencoder.c
        # ./FFmpeg/libavcodec/libwebpenc_common.c
        # ./FFmpeg/libavcodec/libx264.c
        # ./FFmpeg/libavcodec/libx265.c
        # ./FFmpeg/libavcodec/libxavs.c
        # ./FFmpeg/libavcodec/libxavs2.c
        # ./FFmpeg/libavcodec/libxvid.c
        # ./FFmpeg/libavcodec/libzvbi-teletextdec.c
        ./FFmpeg/libavcodec/ljpegenc.c
        ./FFmpeg/libavcodec/loco.c
        # ./FFmpeg/libavcodec/log2_tab.c
        ./FFmpeg/libavcodec/lossless_audiodsp.c
        ./FFmpeg/libavcodec/lossless_videodsp.c
        ./FFmpeg/libavcodec/lossless_videoencdsp.c
        ./FFmpeg/libavcodec/lpc.c
        ./FFmpeg/libavcodec/lsp.c
        ./FFmpeg/libavcodec/lzf.c
        ./FFmpeg/libavcodec/lzw.c
        ./FFmpeg/libavcodec/lzwenc.c
        ./FFmpeg/libavcodec/m101.c
        ./FFmpeg/libavcodec/mace.c
        ./FFmpeg/libavcodec/magicyuv.c
        ./FFmpeg/libavcodec/magicyuvenc.c
        ./FFmpeg/libavcodec/mathtables.c
        ./FFmpeg/libavcodec/mdct15.c
        ./FFmpeg/libavcodec/mdct_fixed.c
        ./FFmpeg/libavcodec/mdct_fixed_32.c
        ./FFmpeg/libavcodec/mdct_float.c
        # ./FFmpeg/libavcodec/mdct_template.c
        ./FFmpeg/libavcodec/mdec.c
        ./FFmpeg/libavcodec/me_cmp.c
        # ./FFmpeg/libavcodec/mediacodec.c
        # ./FFmpeg/libavcodec/mediacodec_surface.c
        # ./FFmpeg/libavcodec/mediacodec_sw_buffer.c
        # ./FFmpeg/libavcodec/mediacodec_wrapper.c
        # ./FFmpeg/libavcodec/mediacodecdec.c
        # ./FFmpeg/libavcodec/mediacodecdec_common.c
        ./FFmpeg/libavcodec/metasound.c
        ./FFmpeg/libavcodec/metasound_data.c
        ./FFmpeg/libavcodec/microdvddec.c
        ./FFmpeg/libavcodec/mimic.c
        # ./FFmpeg/libavcodec/mips/aaccoder_mips.c
        # ./FFmpeg/libavcodec/mips/aacdec_mips.c
        # ./FFmpeg/libavcodec/mips/aacpsdsp_mips.c
        # ./FFmpeg/libavcodec/mips/aacsbr_mips.c
        # ./FFmpeg/libavcodec/mips/ac3dsp_mips.c
        # ./FFmpeg/libavcodec/mips/acelp_filters_mips.c
        # ./FFmpeg/libavcodec/mips/acelp_vectors_mips.c
        # ./FFmpeg/libavcodec/mips/amrwbdec_mips.c
        # ./FFmpeg/libavcodec/mips/blockdsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/blockdsp_mmi.c
        # ./FFmpeg/libavcodec/mips/blockdsp_msa.c
        # ./FFmpeg/libavcodec/mips/celp_filters_mips.c
        # ./FFmpeg/libavcodec/mips/celp_math_mips.c
        # ./FFmpeg/libavcodec/mips/constants.c
        # ./FFmpeg/libavcodec/mips/fft_mips.c
        # ./FFmpeg/libavcodec/mips/fmtconvert_mips.c
        # ./FFmpeg/libavcodec/mips/h263dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/h263dsp_msa.c
        # ./FFmpeg/libavcodec/mips/h264chroma_init_mips.c
        # ./FFmpeg/libavcodec/mips/h264chroma_mmi.c
        # ./FFmpeg/libavcodec/mips/h264chroma_msa.c
        # ./FFmpeg/libavcodec/mips/h264dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/h264dsp_mmi.c
        # ./FFmpeg/libavcodec/mips/h264dsp_msa.c
        # ./FFmpeg/libavcodec/mips/h264idct_msa.c
        # ./FFmpeg/libavcodec/mips/h264pred_init_mips.c
        # ./FFmpeg/libavcodec/mips/h264pred_mmi.c
        # ./FFmpeg/libavcodec/mips/h264pred_msa.c
        # ./FFmpeg/libavcodec/mips/h264qpel_init_mips.c
        # ./FFmpeg/libavcodec/mips/h264qpel_mmi.c
        # ./FFmpeg/libavcodec/mips/h264qpel_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_idct_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_lpf_sao_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_mc_bi_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_mc_biw_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_mc_uni_msa.c
        # ./FFmpeg/libavcodec/mips/hevc_mc_uniw_msa.c
        # ./FFmpeg/libavcodec/mips/hevcdsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/hevcdsp_msa.c
        # ./FFmpeg/libavcodec/mips/hevcpred_init_mips.c
        # ./FFmpeg/libavcodec/mips/hevcpred_msa.c
        # ./FFmpeg/libavcodec/mips/hpeldsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/hpeldsp_mmi.c
        # ./FFmpeg/libavcodec/mips/hpeldsp_msa.c
        # ./FFmpeg/libavcodec/mips/idctdsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/idctdsp_mmi.c
        # ./FFmpeg/libavcodec/mips/idctdsp_msa.c
        # ./FFmpeg/libavcodec/mips/iirfilter_mips.c
        # ./FFmpeg/libavcodec/mips/me_cmp_init_mips.c
        # ./FFmpeg/libavcodec/mips/me_cmp_msa.c
        # ./FFmpeg/libavcodec/mips/mpegaudiodsp_mips_fixed.c
        # ./FFmpeg/libavcodec/mips/mpegaudiodsp_mips_float.c
        # ./FFmpeg/libavcodec/mips/mpegvideo_init_mips.c
        # ./FFmpeg/libavcodec/mips/mpegvideo_mmi.c
        # ./FFmpeg/libavcodec/mips/mpegvideo_msa.c
        # ./FFmpeg/libavcodec/mips/mpegvideoencdsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/mpegvideoencdsp_msa.c
        # ./FFmpeg/libavcodec/mips/pixblockdsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/pixblockdsp_mmi.c
        # ./FFmpeg/libavcodec/mips/pixblockdsp_msa.c
        # ./FFmpeg/libavcodec/mips/qpeldsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/qpeldsp_msa.c
        # ./FFmpeg/libavcodec/mips/sbrdsp_mips.c
        # ./FFmpeg/libavcodec/mips/simple_idct_mmi.c
        # ./FFmpeg/libavcodec/mips/simple_idct_msa.c
        # ./FFmpeg/libavcodec/mips/vc1dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/vc1dsp_mmi.c
        # ./FFmpeg/libavcodec/mips/videodsp_init.c
        # ./FFmpeg/libavcodec/mips/vp8_idct_msa.c
        # ./FFmpeg/libavcodec/mips/vp8_lpf_msa.c
        # ./FFmpeg/libavcodec/mips/vp8_mc_msa.c
        # ./FFmpeg/libavcodec/mips/vp8dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/vp8dsp_mmi.c
        # ./FFmpeg/libavcodec/mips/vp9_idct_msa.c
        # ./FFmpeg/libavcodec/mips/vp9_intra_msa.c
        # ./FFmpeg/libavcodec/mips/vp9_lpf_msa.c
        # ./FFmpeg/libavcodec/mips/vp9_mc_msa.c
        # ./FFmpeg/libavcodec/mips/vp9dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/wmv2dsp_init_mips.c
        # ./FFmpeg/libavcodec/mips/wmv2dsp_mmi.c
        # ./FFmpeg/libavcodec/mips/xvid_idct_mmi.c
        # ./FFmpeg/libavcodec/mips/xvididct_init_mips.c
        ./FFmpeg/libavcodec/mjpeg2jpeg_bsf.c
        ./FFmpeg/libavcodec/mjpeg_parser.c
        ./FFmpeg/libavcodec/mjpega_dump_header_bsf.c
        ./FFmpeg/libavcodec/mjpegbdec.c
        ./FFmpeg/libavcodec/mjpegdec.c
        ./FFmpeg/libavcodec/mjpegenc.c
        ./FFmpeg/libavcodec/mjpegenc_common.c
        ./FFmpeg/libavcodec/mjpegenc_huffman.c
        ./FFmpeg/libavcodec/mlp.c
        ./FFmpeg/libavcodec/mlp_parser.c
        ./FFmpeg/libavcodec/mlpdec.c
        ./FFmpeg/libavcodec/mlpdsp.c
        ./FFmpeg/libavcodec/mlpenc.c
        ./FFmpeg/libavcodec/mlz.c
        # ./FFmpeg/libavcodec/mmaldec.c
        ./FFmpeg/libavcodec/mmvideo.c
        ./FFmpeg/libavcodec/motion_est.c
        # ./FFmpeg/libavcodec/motion_est_template.c
        ./FFmpeg/libavcodec/motionpixels.c
        # ./FFmpeg/libavcodec/motionpixels_tablegen.c
        ./FFmpeg/libavcodec/movsub_bsf.c
        ./FFmpeg/libavcodec/movtextdec.c
        ./FFmpeg/libavcodec/movtextenc.c
        ./FFmpeg/libavcodec/mp3_header_decompress_bsf.c
        ./FFmpeg/libavcodec/mpc.c
        ./FFmpeg/libavcodec/mpc7.c
        ./FFmpeg/libavcodec/mpc8.c
        ./FFmpeg/libavcodec/mpeg12.c
        ./FFmpeg/libavcodec/mpeg12data.c
        ./FFmpeg/libavcodec/mpeg12dec.c
        ./FFmpeg/libavcodec/mpeg12enc.c
        ./FFmpeg/libavcodec/mpeg12framerate.c
        ./FFmpeg/libavcodec/mpeg2_metadata_bsf.c
        ./FFmpeg/libavcodec/mpeg4_unpack_bframes_bsf.c
        ./FFmpeg/libavcodec/mpeg4audio.c
        ./FFmpeg/libavcodec/mpeg4video.c
        ./FFmpeg/libavcodec/mpeg4video_parser.c
        ./FFmpeg/libavcodec/mpeg4videodec.c
        ./FFmpeg/libavcodec/mpeg4videoenc.c
        ./FFmpeg/libavcodec/mpeg_er.c
        ./FFmpeg/libavcodec/mpegaudio.c
        ./FFmpeg/libavcodec/mpegaudio_parser.c
        # ./FFmpeg/libavcodec/mpegaudio_tablegen.c
        ./FFmpeg/libavcodec/mpegaudiodata.c
        ./FFmpeg/libavcodec/mpegaudiodec_fixed.c
        ./FFmpeg/libavcodec/mpegaudiodec_float.c
        # ./FFmpeg/libavcodec/mpegaudiodec_template.c
        ./FFmpeg/libavcodec/mpegaudiodecheader.c
        ./FFmpeg/libavcodec/mpegaudiodsp.c
        ./FFmpeg/libavcodec/mpegaudiodsp_data.c
        ./FFmpeg/libavcodec/mpegaudiodsp_fixed.c
        ./FFmpeg/libavcodec/mpegaudiodsp_float.c
        # ./FFmpeg/libavcodec/mpegaudiodsp_template.c
        ./FFmpeg/libavcodec/mpegaudioenc_fixed.c
        ./FFmpeg/libavcodec/mpegaudioenc_float.c
        # ./FFmpeg/libavcodec/mpegaudioenc_template.c
        ./FFmpeg/libavcodec/mpegpicture.c
        ./FFmpeg/libavcodec/mpegutils.c
        ./FFmpeg/libavcodec/mpegvideo.c
        ./FFmpeg/libavcodec/mpegvideo_enc.c
        ./FFmpeg/libavcodec/mpegvideo_motion.c
        ./FFmpeg/libavcodec/mpegvideo_parser.c
        # ./FFmpeg/libavcodec/mpegvideo_xvmc.c
        ./FFmpeg/libavcodec/mpegvideodata.c
        ./FFmpeg/libavcodec/mpegvideodsp.c
        ./FFmpeg/libavcodec/mpegvideoencdsp.c
        ./FFmpeg/libavcodec/mpl2dec.c
        ./FFmpeg/libavcodec/mqc.c
        ./FFmpeg/libavcodec/mqcdec.c
        ./FFmpeg/libavcodec/mqcenc.c
        ./FFmpeg/libavcodec/mscc.c
        ./FFmpeg/libavcodec/msgsmdec.c
        ./FFmpeg/libavcodec/msmpeg4.c
        ./FFmpeg/libavcodec/msmpeg4data.c
        ./FFmpeg/libavcodec/msmpeg4dec.c
        ./FFmpeg/libavcodec/msmpeg4enc.c
        ./FFmpeg/libavcodec/msrle.c
        ./FFmpeg/libavcodec/msrledec.c
        ./FFmpeg/libavcodec/mss1.c
        ./FFmpeg/libavcodec/mss12.c
        ./FFmpeg/libavcodec/mss2.c
        ./FFmpeg/libavcodec/mss2dsp.c
        ./FFmpeg/libavcodec/mss3.c
        ./FFmpeg/libavcodec/mss34dsp.c
        ./FFmpeg/libavcodec/mss4.c
        ./FFmpeg/libavcodec/msvideo1.c
        ./FFmpeg/libavcodec/msvideo1enc.c
        ./FFmpeg/libavcodec/mvcdec.c
        ./FFmpeg/libavcodec/mwsc.c
        ./FFmpeg/libavcodec/mxpegdec.c
        ./FFmpeg/libavcodec/nellymoser.c
        ./FFmpeg/libavcodec/nellymoserdec.c
        ./FFmpeg/libavcodec/nellymoserenc.c
        # ./FFmpeg/libavcodec/neon/mpegvideo.c
        ./FFmpeg/libavcodec/noise_bsf.c
        ./FFmpeg/libavcodec/null_bsf.c
        ./FFmpeg/libavcodec/nuv.c
        # ./FFmpeg/libavcodec/nvdec.c
        # ./FFmpeg/libavcodec/nvdec_h264.c
        # ./FFmpeg/libavcodec/nvdec_hevc.c
        # ./FFmpeg/libavcodec/nvdec_mjpeg.c
        # ./FFmpeg/libavcodec/nvdec_mpeg12.c
        # ./FFmpeg/libavcodec/nvdec_mpeg4.c
        # ./FFmpeg/libavcodec/nvdec_vc1.c
        # ./FFmpeg/libavcodec/nvdec_vp8.c
        # ./FFmpeg/libavcodec/nvdec_vp9.c
        # ./FFmpeg/libavcodec/nvenc.c
        # ./FFmpeg/libavcodec/nvenc_h264.c
        # ./FFmpeg/libavcodec/nvenc_hevc.c
        # ./FFmpeg/libavcodec/omx.c
        ./FFmpeg/libavcodec/on2avc.c
        ./FFmpeg/libavcodec/on2avcdata.c
        ./FFmpeg/libavcodec/options.c
        ./FFmpeg/libavcodec/opus.c
        ./FFmpeg/libavcodec/opus_celt.c
        ./FFmpeg/libavcodec/opus_parser.c
        ./FFmpeg/libavcodec/opus_pvq.c
        ./FFmpeg/libavcodec/opus_rc.c
        ./FFmpeg/libavcodec/opus_silk.c
        ./FFmpeg/libavcodec/opusdec.c
        ./FFmpeg/libavcodec/opusenc.c
        ./FFmpeg/libavcodec/opusenc_psy.c
        ./FFmpeg/libavcodec/opustab.c
        ./FFmpeg/libavcodec/pafaudio.c
        ./FFmpeg/libavcodec/pafvideo.c
        ./FFmpeg/libavcodec/pamenc.c
        ./FFmpeg/libavcodec/parser.c
        # ./FFmpeg/libavcodec/parser_list.c
        ./FFmpeg/libavcodec/parsers.c
        ./FFmpeg/libavcodec/pcm-bluray.c
        ./FFmpeg/libavcodec/pcm-dvd.c
        ./FFmpeg/libavcodec/pcm.c
        # ./FFmpeg/libavcodec/pcm_tablegen.c
        ./FFmpeg/libavcodec/pcx.c
        ./FFmpeg/libavcodec/pcxenc.c
        # ./FFmpeg/libavcodec/pel_template.c
        ./FFmpeg/libavcodec/pgssubdec.c
        ./FFmpeg/libavcodec/pictordec.c
        ./FFmpeg/libavcodec/pixblockdsp.c
        ./FFmpeg/libavcodec/pixlet.c
        ./FFmpeg/libavcodec/png.c
        ./FFmpeg/libavcodec/png_parser.c
        ./FFmpeg/libavcodec/pngdec.c
        ./FFmpeg/libavcodec/pngdsp.c
        ./FFmpeg/libavcodec/pngenc.c
        ./FFmpeg/libavcodec/pnm.c
        ./FFmpeg/libavcodec/pnm_parser.c
        ./FFmpeg/libavcodec/pnmdec.c
        ./FFmpeg/libavcodec/pnmenc.c
        # ./FFmpeg/libavcodec/ppc/audiodsp.c
        # ./FFmpeg/libavcodec/ppc/blockdsp.c
        # ./FFmpeg/libavcodec/ppc/fdctdsp.c
        # ./FFmpeg/libavcodec/ppc/fft_init.c
        # ./FFmpeg/libavcodec/ppc/fft_vsx.c
        # ./FFmpeg/libavcodec/ppc/fmtconvert_altivec.c
        # ./FFmpeg/libavcodec/ppc/h264chroma_init.c
        # ./FFmpeg/libavcodec/ppc/h264chroma_template.c
        # ./FFmpeg/libavcodec/ppc/h264dsp.c
        # ./FFmpeg/libavcodec/ppc/h264qpel.c
        # ./FFmpeg/libavcodec/ppc/h264qpel_template.c
        # ./FFmpeg/libavcodec/ppc/hevcdsp.c
        # ./FFmpeg/libavcodec/ppc/hevcdsp_template.c
        # ./FFmpeg/libavcodec/ppc/hpeldsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/idctdsp.c
        # ./FFmpeg/libavcodec/ppc/lossless_audiodsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/lossless_videodsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/me_cmp.c
        # ./FFmpeg/libavcodec/ppc/mpegaudiodsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/mpegvideo_altivec.c
        # ./FFmpeg/libavcodec/ppc/mpegvideodsp.c
        # ./FFmpeg/libavcodec/ppc/mpegvideoencdsp.c
        # ./FFmpeg/libavcodec/ppc/pixblockdsp.c
        # ./FFmpeg/libavcodec/ppc/svq1enc_altivec.c
        # ./FFmpeg/libavcodec/ppc/vc1dsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/videodsp.c
        # ./FFmpeg/libavcodec/ppc/vorbisdsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/vp3dsp_altivec.c
        # ./FFmpeg/libavcodec/ppc/vp8dsp_altivec.c
        ./FFmpeg/libavcodec/profiles.c
        ./FFmpeg/libavcodec/proresdata.c
        ./FFmpeg/libavcodec/proresdec2.c
        ./FFmpeg/libavcodec/proresdsp.c
        ./FFmpeg/libavcodec/proresenc_anatoliy.c
        ./FFmpeg/libavcodec/proresenc_kostya.c
        ./FFmpeg/libavcodec/prosumer.c
        ./FFmpeg/libavcodec/psd.c
        ./FFmpeg/libavcodec/psymodel.c
        ./FFmpeg/libavcodec/pthread.c
        ./FFmpeg/libavcodec/pthread_frame.c
        ./FFmpeg/libavcodec/pthread_slice.c
        ./FFmpeg/libavcodec/ptx.c
        ./FFmpeg/libavcodec/qcelpdec.c
        ./FFmpeg/libavcodec/qdm2.c
        # ./FFmpeg/libavcodec/qdm2_tablegen.c
        ./FFmpeg/libavcodec/qdmc.c
        ./FFmpeg/libavcodec/qdrw.c
        ./FFmpeg/libavcodec/qpeg.c
        # ./FFmpeg/libavcodec/qpel_template.c
        ./FFmpeg/libavcodec/qpeldsp.c
        # ./FFmpeg/libavcodec/qsv.c
        # ./FFmpeg/libavcodec/qsv_api.c
        # ./FFmpeg/libavcodec/qsvdec.c
        # ./FFmpeg/libavcodec/qsvdec_h2645.c
        # ./FFmpeg/libavcodec/qsvdec_other.c
        # ./FFmpeg/libavcodec/qsvenc.c
        # ./FFmpeg/libavcodec/qsvenc_h264.c
        # ./FFmpeg/libavcodec/qsvenc_hevc.c
        # ./FFmpeg/libavcodec/qsvenc_jpeg.c
        # ./FFmpeg/libavcodec/qsvenc_mpeg2.c
        ./FFmpeg/libavcodec/qtrle.c
        ./FFmpeg/libavcodec/qtrleenc.c
        ./FFmpeg/libavcodec/r210dec.c
        ./FFmpeg/libavcodec/r210enc.c
        ./FFmpeg/libavcodec/ra144.c
        ./FFmpeg/libavcodec/ra144dec.c
        ./FFmpeg/libavcodec/ra144enc.c
        ./FFmpeg/libavcodec/ra288.c
        ./FFmpeg/libavcodec/ralf.c
        ./FFmpeg/libavcodec/rangecoder.c
        ./FFmpeg/libavcodec/rasc.c
        ./FFmpeg/libavcodec/ratecontrol.c
        ./FFmpeg/libavcodec/raw.c
        ./FFmpeg/libavcodec/rawdec.c
        ./FFmpeg/libavcodec/rawenc.c
        ./FFmpeg/libavcodec/rdft.c
        ./FFmpeg/libavcodec/realtextdec.c
        ./FFmpeg/libavcodec/remove_extradata_bsf.c
        # ./FFmpeg/libavcodec/reverse.c
        # ./FFmpeg/libavcodec/rkmppdec.c
        ./FFmpeg/libavcodec/rl.c
        ./FFmpeg/libavcodec/rl2.c
        ./FFmpeg/libavcodec/rle.c
        ./FFmpeg/libavcodec/roqaudioenc.c
        ./FFmpeg/libavcodec/roqvideo.c
        ./FFmpeg/libavcodec/roqvideodec.c
        ./FFmpeg/libavcodec/roqvideoenc.c
        ./FFmpeg/libavcodec/rpza.c
        ./FFmpeg/libavcodec/rscc.c
        ./FFmpeg/libavcodec/rtjpeg.c
        ./FFmpeg/libavcodec/rv10.c
        ./FFmpeg/libavcodec/rv10enc.c
        ./FFmpeg/libavcodec/rv20enc.c
        ./FFmpeg/libavcodec/rv30.c
        ./FFmpeg/libavcodec/rv30dsp.c
        ./FFmpeg/libavcodec/rv34.c
        ./FFmpeg/libavcodec/rv34_parser.c
        ./FFmpeg/libavcodec/rv34dsp.c
        ./FFmpeg/libavcodec/rv40.c
        ./FFmpeg/libavcodec/rv40dsp.c
        ./FFmpeg/libavcodec/s302m.c
        ./FFmpeg/libavcodec/s302menc.c
        ./FFmpeg/libavcodec/samidec.c
        ./FFmpeg/libavcodec/sanm.c
        ./FFmpeg/libavcodec/sbc.c
        ./FFmpeg/libavcodec/sbc_parser.c
        ./FFmpeg/libavcodec/sbcdec.c
        ./FFmpeg/libavcodec/sbcdec_data.c
        ./FFmpeg/libavcodec/sbcdsp.c
        ./FFmpeg/libavcodec/sbcdsp_data.c
        ./FFmpeg/libavcodec/sbcenc.c
        ./FFmpeg/libavcodec/sbrdsp.c
        ./FFmpeg/libavcodec/sbrdsp_fixed.c
        # ./FFmpeg/libavcodec/sbrdsp_template.c
        ./FFmpeg/libavcodec/scpr.c
        ./FFmpeg/libavcodec/screenpresso.c
        ./FFmpeg/libavcodec/sgidec.c
        ./FFmpeg/libavcodec/sgienc.c
        ./FFmpeg/libavcodec/sgirledec.c
        ./FFmpeg/libavcodec/sheervideo.c
        ./FFmpeg/libavcodec/shorten.c
        ./FFmpeg/libavcodec/simple_idct.c
        # ./FFmpeg/libavcodec/simple_idct_template.c
        ./FFmpeg/libavcodec/sinewin.c
        ./FFmpeg/libavcodec/sinewin_fixed.c
        # ./FFmpeg/libavcodec/sinewin_fixed_tablegen.c
        # ./FFmpeg/libavcodec/sinewin_tablegen.c
        # ./FFmpeg/libavcodec/sinewin_tablegen_template.c
        ./FFmpeg/libavcodec/sipr.c
        ./FFmpeg/libavcodec/sipr16k.c
        ./FFmpeg/libavcodec/sipr_parser.c
        ./FFmpeg/libavcodec/smacker.c
        ./FFmpeg/libavcodec/smc.c
        ./FFmpeg/libavcodec/smvjpegdec.c
        ./FFmpeg/libavcodec/snappy.c
        ./FFmpeg/libavcodec/snow.c
        ./FFmpeg/libavcodec/snow_dwt.c
        ./FFmpeg/libavcodec/snowdec.c
        ./FFmpeg/libavcodec/snowenc.c
        ./FFmpeg/libavcodec/sonic.c
        ./FFmpeg/libavcodec/sp5xdec.c
        ./FFmpeg/libavcodec/speedhq.c
        ./FFmpeg/libavcodec/srtdec.c
        ./FFmpeg/libavcodec/srtenc.c
        ./FFmpeg/libavcodec/startcode.c
        ./FFmpeg/libavcodec/subviewerdec.c
        ./FFmpeg/libavcodec/sunrast.c
        ./FFmpeg/libavcodec/sunrastenc.c
        ./FFmpeg/libavcodec/svq1.c
        ./FFmpeg/libavcodec/svq13.c
        ./FFmpeg/libavcodec/svq1dec.c
        ./FFmpeg/libavcodec/svq1enc.c
        ./FFmpeg/libavcodec/svq3.c
        ./FFmpeg/libavcodec/synth_filter.c
        ./FFmpeg/libavcodec/tak.c
        ./FFmpeg/libavcodec/tak_parser.c
        ./FFmpeg/libavcodec/takdec.c
        ./FFmpeg/libavcodec/takdsp.c
        ./FFmpeg/libavcodec/targa.c
        ./FFmpeg/libavcodec/targa_y216dec.c
        ./FFmpeg/libavcodec/targaenc.c
        ./FFmpeg/libavcodec/tdsc.c
        # ./FFmpeg/libavcodec/tests/aarch64/dct.c
        # ./FFmpeg/libavcodec/tests/arm/dct.c
        # ./FFmpeg/libavcodec/tests/avfft.c
        # ./FFmpeg/libavcodec/tests/avpacket.c
        # ./FFmpeg/libavcodec/tests/cabac.c
        # ./FFmpeg/libavcodec/tests/celp_math.c
        # ./FFmpeg/libavcodec/tests/codec_desc.c
        # ./FFmpeg/libavcodec/tests/dct.c
        # ./FFmpeg/libavcodec/tests/fft-fixed.c
        # ./FFmpeg/libavcodec/tests/fft-fixed32.c
        # ./FFmpeg/libavcodec/tests/fft.c
        # ./FFmpeg/libavcodec/tests/golomb.c
        # ./FFmpeg/libavcodec/tests/h264_levels.c
        # ./FFmpeg/libavcodec/tests/htmlsubtitles.c
        # ./FFmpeg/libavcodec/tests/iirfilter.c
        # ./FFmpeg/libavcodec/tests/imgconvert.c
        # ./FFmpeg/libavcodec/tests/jpeg2000dwt.c
        # ./FFmpeg/libavcodec/tests/mathops.c
        # ./FFmpeg/libavcodec/tests/mjpegenc_huffman.c
        # ./FFmpeg/libavcodec/tests/motion.c
        # ./FFmpeg/libavcodec/tests/mpeg12framerate.c
        # ./FFmpeg/libavcodec/tests/options.c
        # ./FFmpeg/libavcodec/tests/ppc/dct.c
        # ./FFmpeg/libavcodec/tests/rangecoder.c
        # ./FFmpeg/libavcodec/tests/snowenc.c
        # ./FFmpeg/libavcodec/tests/utils.c
        # ./FFmpeg/libavcodec/tests/x86/dct.c
        ./FFmpeg/libavcodec/textdec.c
        ./FFmpeg/libavcodec/texturedsp.c
        ./FFmpeg/libavcodec/texturedspenc.c
        ./FFmpeg/libavcodec/tiertexseqv.c
        # ./FFmpeg/libavcodec/tiff.c
        ./FFmpeg/libavcodec/tiff_common.c
        # ./FFmpeg/libavcodec/tiff_data.c
        # ./FFmpeg/libavcodec/tiffenc.c
        ./FFmpeg/libavcodec/tmv.c
        ./FFmpeg/libavcodec/tpeldsp.c
        ./FFmpeg/libavcodec/trace_headers_bsf.c
        ./FFmpeg/libavcodec/truemotion1.c
        ./FFmpeg/libavcodec/truemotion2.c
        ./FFmpeg/libavcodec/truemotion2rt.c
        ./FFmpeg/libavcodec/truespeech.c
        ./FFmpeg/libavcodec/tscc.c
        ./FFmpeg/libavcodec/tscc2.c
        ./FFmpeg/libavcodec/tta.c
        ./FFmpeg/libavcodec/ttadata.c
        ./FFmpeg/libavcodec/ttadsp.c
        ./FFmpeg/libavcodec/ttaenc.c
        ./FFmpeg/libavcodec/ttaencdsp.c
        ./FFmpeg/libavcodec/twinvq.c
        ./FFmpeg/libavcodec/twinvqdec.c
        ./FFmpeg/libavcodec/txd.c
        ./FFmpeg/libavcodec/ulti.c
        ./FFmpeg/libavcodec/utils.c
        ./FFmpeg/libavcodec/utvideo.c
        ./FFmpeg/libavcodec/utvideodec.c
        ./FFmpeg/libavcodec/utvideodsp.c
        ./FFmpeg/libavcodec/utvideoenc.c
        ./FFmpeg/libavcodec/v210dec.c
        ./FFmpeg/libavcodec/v210enc.c
        ./FFmpeg/libavcodec/v210x.c
        ./FFmpeg/libavcodec/v308dec.c
        ./FFmpeg/libavcodec/v308enc.c
        ./FFmpeg/libavcodec/v408dec.c
        ./FFmpeg/libavcodec/v408enc.c
        ./FFmpeg/libavcodec/v410dec.c
        ./FFmpeg/libavcodec/v410enc.c
        # ./FFmpeg/libavcodec/v4l2_buffers.c
        # ./FFmpeg/libavcodec/v4l2_context.c
        # ./FFmpeg/libavcodec/v4l2_fmt.c
        # ./FFmpeg/libavcodec/v4l2_m2m.c
        # ./FFmpeg/libavcodec/v4l2_m2m_dec.c
        # ./FFmpeg/libavcodec/v4l2_m2m_enc.c
        # ./FFmpeg/libavcodec/vaapi_decode.c
        # ./FFmpeg/libavcodec/vaapi_encode.c
        # ./FFmpeg/libavcodec/vaapi_encode_h264.c
        # ./FFmpeg/libavcodec/vaapi_encode_h265.c
        # ./FFmpeg/libavcodec/vaapi_encode_mjpeg.c
        # ./FFmpeg/libavcodec/vaapi_encode_mpeg2.c
        # ./FFmpeg/libavcodec/vaapi_encode_vp8.c
        # ./FFmpeg/libavcodec/vaapi_encode_vp9.c
        # ./FFmpeg/libavcodec/vaapi_h264.c
        # ./FFmpeg/libavcodec/vaapi_hevc.c
        # ./FFmpeg/libavcodec/vaapi_mjpeg.c
        # ./FFmpeg/libavcodec/vaapi_mpeg2.c
        # ./FFmpeg/libavcodec/vaapi_mpeg4.c
        # ./FFmpeg/libavcodec/vaapi_vc1.c
        # ./FFmpeg/libavcodec/vaapi_vp8.c
        # ./FFmpeg/libavcodec/vaapi_vp9.c
        ./FFmpeg/libavcodec/vb.c
        ./FFmpeg/libavcodec/vble.c
        ./FFmpeg/libavcodec/vc1.c
        ./FFmpeg/libavcodec/vc1_block.c
        ./FFmpeg/libavcodec/vc1_loopfilter.c
        ./FFmpeg/libavcodec/vc1_mc.c
        ./FFmpeg/libavcodec/vc1_parser.c
        ./FFmpeg/libavcodec/vc1_pred.c
        ./FFmpeg/libavcodec/vc1data.c
        ./FFmpeg/libavcodec/vc1dec.c
        ./FFmpeg/libavcodec/vc1dsp.c
        ./FFmpeg/libavcodec/vc2enc.c
        ./FFmpeg/libavcodec/vc2enc_dwt.c
        ./FFmpeg/libavcodec/vcr1.c
        # ./FFmpeg/libavcodec/vdpau.c
        # ./FFmpeg/libavcodec/vdpau_h264.c
        # ./FFmpeg/libavcodec/vdpau_hevc.c
        # ./FFmpeg/libavcodec/vdpau_mpeg12.c
        # ./FFmpeg/libavcodec/vdpau_mpeg4.c
        # ./FFmpeg/libavcodec/vdpau_vc1.c
        ./FFmpeg/libavcodec/videodsp.c
        # ./FFmpeg/libavcodec/videodsp_template.c
        # ./FFmpeg/libavcodec/videotoolbox.c
        # ./FFmpeg/libavcodec/videotoolboxenc.c
        ./FFmpeg/libavcodec/vima.c
        ./FFmpeg/libavcodec/vmdaudio.c
        ./FFmpeg/libavcodec/vmdvideo.c
        ./FFmpeg/libavcodec/vmnc.c
        ./FFmpeg/libavcodec/vorbis.c
        ./FFmpeg/libavcodec/vorbis_data.c
        ./FFmpeg/libavcodec/vorbis_parser.c
        ./FFmpeg/libavcodec/vorbisdec.c
        ./FFmpeg/libavcodec/vorbisdsp.c
        ./FFmpeg/libavcodec/vorbisenc.c
        ./FFmpeg/libavcodec/vp3.c
        ./FFmpeg/libavcodec/vp3_parser.c
        ./FFmpeg/libavcodec/vp3dsp.c
        ./FFmpeg/libavcodec/vp5.c
        ./FFmpeg/libavcodec/vp56.c
        ./FFmpeg/libavcodec/vp56data.c
        ./FFmpeg/libavcodec/vp56dsp.c
        ./FFmpeg/libavcodec/vp56rac.c
        ./FFmpeg/libavcodec/vp6.c
        ./FFmpeg/libavcodec/vp6dsp.c
        ./FFmpeg/libavcodec/vp8.c
        ./FFmpeg/libavcodec/vp8_parser.c
        ./FFmpeg/libavcodec/vp8dsp.c
        ./FFmpeg/libavcodec/vp9.c
        # ./FFmpeg/libavcodec/vp9_mc_template.c
        ./FFmpeg/libavcodec/vp9_metadata_bsf.c
        ./FFmpeg/libavcodec/vp9_parser.c
        ./FFmpeg/libavcodec/vp9_raw_reorder_bsf.c
        ./FFmpeg/libavcodec/vp9_superframe_bsf.c
        ./FFmpeg/libavcodec/vp9_superframe_split_bsf.c
        ./FFmpeg/libavcodec/vp9block.c
        ./FFmpeg/libavcodec/vp9data.c
        ./FFmpeg/libavcodec/vp9dsp.c
        ./FFmpeg/libavcodec/vp9dsp_10bpp.c
        ./FFmpeg/libavcodec/vp9dsp_12bpp.c
        ./FFmpeg/libavcodec/vp9dsp_8bpp.c
        # ./FFmpeg/libavcodec/vp9dsp_template.c
        ./FFmpeg/libavcodec/vp9lpf.c
        ./FFmpeg/libavcodec/vp9mvs.c
        ./FFmpeg/libavcodec/vp9prob.c
        ./FFmpeg/libavcodec/vp9recon.c
        ./FFmpeg/libavcodec/vqavideo.c
        ./FFmpeg/libavcodec/wavpack.c
        ./FFmpeg/libavcodec/wavpackenc.c
        ./FFmpeg/libavcodec/wcmv.c
        ./FFmpeg/libavcodec/webp.c
        ./FFmpeg/libavcodec/webvttdec.c
        ./FFmpeg/libavcodec/webvttenc.c
        ./FFmpeg/libavcodec/wma.c
        ./FFmpeg/libavcodec/wma_common.c
        ./FFmpeg/libavcodec/wma_freqs.c
        ./FFmpeg/libavcodec/wmadec.c
        ./FFmpeg/libavcodec/wmaenc.c
        ./FFmpeg/libavcodec/wmalosslessdec.c
        ./FFmpeg/libavcodec/wmaprodec.c
        ./FFmpeg/libavcodec/wmavoice.c
        ./FFmpeg/libavcodec/wmv2.c
        ./FFmpeg/libavcodec/wmv2data.c
        ./FFmpeg/libavcodec/wmv2dec.c
        ./FFmpeg/libavcodec/wmv2dsp.c
        ./FFmpeg/libavcodec/wmv2enc.c
        ./FFmpeg/libavcodec/wnv1.c
        ./FFmpeg/libavcodec/wrapped_avframe.c
        ./FFmpeg/libavcodec/ws-snd1.c
        # ./FFmpeg/libavcodec/x86/aacencdsp_init.c
        # ./FFmpeg/libavcodec/x86/aacpsdsp_init.c
        # ./FFmpeg/libavcodec/x86/ac3dsp_init.c
        # ./FFmpeg/libavcodec/x86/alacdsp_init.c
        # ./FFmpeg/libavcodec/x86/audiodsp_init.c
        # ./FFmpeg/libavcodec/x86/blockdsp_init.c
        # ./FFmpeg/libavcodec/x86/bswapdsp_init.c
        # ./FFmpeg/libavcodec/x86/cavsdsp.c
        # ./FFmpeg/libavcodec/x86/constants.c
        # ./FFmpeg/libavcodec/x86/dcadsp_init.c
        # ./FFmpeg/libavcodec/x86/dct_init.c
        # ./FFmpeg/libavcodec/x86/dirac_dwt_init.c
        # ./FFmpeg/libavcodec/x86/diracdsp_init.c
        # ./FFmpeg/libavcodec/x86/dnxhdenc_init.c
        # ./FFmpeg/libavcodec/x86/exrdsp_init.c
        # ./FFmpeg/libavcodec/x86/fdct.c
        # ./FFmpeg/libavcodec/x86/fdctdsp_init.c
        # ./FFmpeg/libavcodec/x86/fft_init.c
        # ./FFmpeg/libavcodec/x86/flacdsp_init.c
        # ./FFmpeg/libavcodec/x86/fmtconvert_init.c
        # ./FFmpeg/libavcodec/x86/g722dsp_init.c
        # ./FFmpeg/libavcodec/x86/h263dsp_init.c
        # ./FFmpeg/libavcodec/x86/h264_cabac.c
        # ./FFmpeg/libavcodec/x86/h264_intrapred_init.c
        # ./FFmpeg/libavcodec/x86/h264_qpel.c
        # ./FFmpeg/libavcodec/x86/h264chroma_init.c
        # ./FFmpeg/libavcodec/x86/h264dsp_init.c
        # ./FFmpeg/libavcodec/x86/hevcdsp_init.c
        # ./FFmpeg/libavcodec/x86/hpeldsp_init.c
        # # ./FFmpeg/libavcodec/x86/hpeldsp_rnd_template.c
        # ./FFmpeg/libavcodec/x86/hpeldsp_vp3_init.c
        # ./FFmpeg/libavcodec/x86/huffyuvdsp_init.c
        # ./FFmpeg/libavcodec/x86/huffyuvencdsp_init.c
        # ./FFmpeg/libavcodec/x86/idctdsp_init.c
        # ./FFmpeg/libavcodec/x86/jpeg2000dsp_init.c
        # ./FFmpeg/libavcodec/x86/lossless_audiodsp_init.c
        # ./FFmpeg/libavcodec/x86/lossless_videodsp_init.c
        # ./FFmpeg/libavcodec/x86/lossless_videoencdsp_init.c
        # ./FFmpeg/libavcodec/x86/lpc.c
        # ./FFmpeg/libavcodec/x86/mdct15_init.c
        # ./FFmpeg/libavcodec/x86/me_cmp_init.c
        # ./FFmpeg/libavcodec/x86/mlpdsp_init.c
        # ./FFmpeg/libavcodec/x86/mpegaudiodsp.c
        # ./FFmpeg/libavcodec/x86/mpegvideo.c
        # ./FFmpeg/libavcodec/x86/mpegvideodsp.c
        # ./FFmpeg/libavcodec/x86/mpegvideoenc.c
        # # ./FFmpeg/libavcodec/x86/mpegvideoenc_qns_template.c
        # # ./FFmpeg/libavcodec/x86/mpegvideoenc_template.c
        # ./FFmpeg/libavcodec/x86/mpegvideoencdsp_init.c
        # ./FFmpeg/libavcodec/x86/opus_dsp_init.c
        # ./FFmpeg/libavcodec/x86/pixblockdsp_init.c
        # ./FFmpeg/libavcodec/x86/pngdsp_init.c
        # ./FFmpeg/libavcodec/x86/proresdsp_init.c
        # ./FFmpeg/libavcodec/x86/qpeldsp_init.c
        # # ./FFmpeg/libavcodec/x86/rnd_template.c
        # ./FFmpeg/libavcodec/x86/rv34dsp_init.c
        # ./FFmpeg/libavcodec/x86/rv40dsp_init.c
        # ./FFmpeg/libavcodec/x86/sbcdsp_init.c
        # ./FFmpeg/libavcodec/x86/sbrdsp_init.c
        # ./FFmpeg/libavcodec/x86/snowdsp.c
        # ./FFmpeg/libavcodec/x86/svq1enc_init.c
        # ./FFmpeg/libavcodec/x86/synth_filter_init.c
        # ./FFmpeg/libavcodec/x86/takdsp_init.c
        # ./FFmpeg/libavcodec/x86/ttadsp_init.c
        # ./FFmpeg/libavcodec/x86/ttaencdsp_init.c
        # ./FFmpeg/libavcodec/x86/utvideodsp_init.c
        # ./FFmpeg/libavcodec/x86/v210-init.c
        # ./FFmpeg/libavcodec/x86/v210enc_init.c
        # ./FFmpeg/libavcodec/x86/vc1dsp_init.c
        # ./FFmpeg/libavcodec/x86/vc1dsp_mmx.c
        # ./FFmpeg/libavcodec/x86/videodsp_init.c
        # ./FFmpeg/libavcodec/x86/vorbisdsp_init.c
        # ./FFmpeg/libavcodec/x86/vp3dsp_init.c
        # ./FFmpeg/libavcodec/x86/vp6dsp_init.c
        # ./FFmpeg/libavcodec/x86/vp8dsp_init.c
        # ./FFmpeg/libavcodec/x86/vp9dsp_init.c
        # ./FFmpeg/libavcodec/x86/vp9dsp_init_10bpp.c
        # ./FFmpeg/libavcodec/x86/vp9dsp_init_12bpp.c
        # ./FFmpeg/libavcodec/x86/vp9dsp_init_16bpp.c
        # # ./FFmpeg/libavcodec/x86/vp9dsp_init_16bpp_template.c
        # ./FFmpeg/libavcodec/x86/w64xmmtest.c
        # ./FFmpeg/libavcodec/x86/xvididct_init.c
        ./FFmpeg/libavcodec/xan.c
        ./FFmpeg/libavcodec/xbmdec.c
        ./FFmpeg/libavcodec/xbmenc.c
        ./FFmpeg/libavcodec/xface.c
        ./FFmpeg/libavcodec/xfacedec.c
        ./FFmpeg/libavcodec/xfaceenc.c
        ./FFmpeg/libavcodec/xiph.c
        ./FFmpeg/libavcodec/xl.c
        ./FFmpeg/libavcodec/xma_parser.c
        ./FFmpeg/libavcodec/xpmdec.c
        ./FFmpeg/libavcodec/xsubdec.c
        ./FFmpeg/libavcodec/xsubenc.c
        ./FFmpeg/libavcodec/xvididct.c
        ./FFmpeg/libavcodec/xwddec.c
        ./FFmpeg/libavcodec/xwdenc.c
        ./FFmpeg/libavcodec/xxan.c
        ./FFmpeg/libavcodec/y41pdec.c
        ./FFmpeg/libavcodec/y41penc.c
        ./FFmpeg/libavcodec/ylc.c
        ./FFmpeg/libavcodec/yop.c
        ./FFmpeg/libavcodec/yuv4dec.c
        ./FFmpeg/libavcodec/yuv4enc.c
        ./FFmpeg/libavcodec/zerocodec.c
        ./FFmpeg/libavcodec/zmbv.c
        ./FFmpeg/libavcodec/zmbvenc.c
        # ./FFmpeg/libavdevice/alldevices.c
        # ./FFmpeg/libavdevice/alsa.c
        # ./FFmpeg/libavdevice/alsa_dec.c
        # ./FFmpeg/libavdevice/alsa_enc.c
        # ./FFmpeg/libavdevice/android_camera.c
        ./FFmpeg/libavdevice/avdevice.c
        # ./FFmpeg/libavdevice/bktr.c
        # ./FFmpeg/libavdevice/caca.c
        # ./FFmpeg/libavdevice/decklink_dec_c.c
        # ./FFmpeg/libavdevice/decklink_enc_c.c
        # ./FFmpeg/libavdevice/dshow.c
        # ./FFmpeg/libavdevice/dshow_common.c
        # ./FFmpeg/libavdevice/dshow_crossbar.c
        # ./FFmpeg/libavdevice/dshow_enummediatypes.c
        # ./FFmpeg/libavdevice/dshow_enumpins.c
        # ./FFmpeg/libavdevice/dshow_filter.c
        # ./FFmpeg/libavdevice/dshow_pin.c
        # ./FFmpeg/libavdevice/fbdev_common.c
        # ./FFmpeg/libavdevice/fbdev_dec.c
        # ./FFmpeg/libavdevice/fbdev_enc.c
        # ./FFmpeg/libavdevice/file_open.c
        # ./FFmpeg/libavdevice/gdigrab.c
        # ./FFmpeg/libavdevice/iec61883.c
        # ./FFmpeg/libavdevice/indev_list.c
        # ./FFmpeg/libavdevice/jack.c
        # ./FFmpeg/libavdevice/kmsgrab.c
        ./FFmpeg/libavdevice/lavfi.c
        # ./FFmpeg/libavdevice/libcdio.c
        # ./FFmpeg/libavdevice/libdc1394.c
        # ./FFmpeg/libavdevice/libndi_newtek_dec.c
        # ./FFmpeg/libavdevice/libndi_newtek_enc.c
        # ./FFmpeg/libavdevice/openal-dec.c
        # ./FFmpeg/libavdevice/opengl_enc.c
        # ./FFmpeg/libavdevice/oss.c
        # ./FFmpeg/libavdevice/oss_dec.c
        # ./FFmpeg/libavdevice/oss_enc.c
        # ./FFmpeg/libavdevice/outdev_list.c
        # ./FFmpeg/libavdevice/pulse_audio_common.c
        # ./FFmpeg/libavdevice/pulse_audio_dec.c
        # ./FFmpeg/libavdevice/pulse_audio_enc.c
        # ./FFmpeg/libavdevice/reverse.c
        # ./FFmpeg/libavdevice/sdl2.c
        # ./FFmpeg/libavdevice/sndio.c
        # ./FFmpeg/libavdevice/sndio_dec.c
        # ./FFmpeg/libavdevice/sndio_enc.c
        # ./FFmpeg/libavdevice/tests/timefilter.c
        ./FFmpeg/libavdevice/timefilter.c
        ./FFmpeg/libavdevice/utils.c
        # ./FFmpeg/libavdevice/v4l2-common.c
        # ./FFmpeg/libavdevice/v4l2.c
        # ./FFmpeg/libavdevice/v4l2enc.c
        # ./FFmpeg/libavdevice/vfwcap.c
        # ./FFmpeg/libavdevice/xcbgrab.c
        # ./FFmpeg/libavdevice/xv.c
        # ./FFmpeg/libavfilter/aarch64/vf_nlmeans_init.c
        ./FFmpeg/libavfilter/aeval.c
        ./FFmpeg/libavfilter/af_acontrast.c
        ./FFmpeg/libavfilter/af_acopy.c
        ./FFmpeg/libavfilter/af_acrossover.c
        ./FFmpeg/libavfilter/af_acrusher.c
        ./FFmpeg/libavfilter/af_adeclick.c
        ./FFmpeg/libavfilter/af_adelay.c
        ./FFmpeg/libavfilter/af_aderivative.c
        ./FFmpeg/libavfilter/af_aecho.c
        ./FFmpeg/libavfilter/af_aemphasis.c
        ./FFmpeg/libavfilter/af_afade.c
        ./FFmpeg/libavfilter/af_afftdn.c
        ./FFmpeg/libavfilter/af_afftfilt.c
        ./FFmpeg/libavfilter/af_afir.c
        ./FFmpeg/libavfilter/af_aformat.c
        ./FFmpeg/libavfilter/af_agate.c
        ./FFmpeg/libavfilter/af_aiir.c
        ./FFmpeg/libavfilter/af_alimiter.c
        ./FFmpeg/libavfilter/af_amerge.c
        ./FFmpeg/libavfilter/af_amix.c
        ./FFmpeg/libavfilter/af_amultiply.c
        ./FFmpeg/libavfilter/af_anequalizer.c
        ./FFmpeg/libavfilter/af_anull.c
        ./FFmpeg/libavfilter/af_apad.c
        ./FFmpeg/libavfilter/af_aphaser.c
        ./FFmpeg/libavfilter/af_apulsator.c
        ./FFmpeg/libavfilter/af_aresample.c
        ./FFmpeg/libavfilter/af_asetnsamples.c
        ./FFmpeg/libavfilter/af_asetrate.c
        ./FFmpeg/libavfilter/af_ashowinfo.c
        ./FFmpeg/libavfilter/af_astats.c
        ./FFmpeg/libavfilter/af_atempo.c
        ./FFmpeg/libavfilter/af_biquads.c
        # ./FFmpeg/libavfilter/af_bs2b.c
        ./FFmpeg/libavfilter/af_channelmap.c
        ./FFmpeg/libavfilter/af_channelsplit.c
        ./FFmpeg/libavfilter/af_chorus.c
        ./FFmpeg/libavfilter/af_compand.c
        ./FFmpeg/libavfilter/af_compensationdelay.c
        ./FFmpeg/libavfilter/af_crossfeed.c
        ./FFmpeg/libavfilter/af_crystalizer.c
        ./FFmpeg/libavfilter/af_dcshift.c
        ./FFmpeg/libavfilter/af_drmeter.c
        ./FFmpeg/libavfilter/af_dynaudnorm.c
        ./FFmpeg/libavfilter/af_earwax.c
        ./FFmpeg/libavfilter/af_extrastereo.c
        ./FFmpeg/libavfilter/af_firequalizer.c
        ./FFmpeg/libavfilter/af_flanger.c
        ./FFmpeg/libavfilter/af_haas.c
        ./FFmpeg/libavfilter/af_hdcd.c
        ./FFmpeg/libavfilter/af_headphone.c
        ./FFmpeg/libavfilter/af_join.c
        # ./FFmpeg/libavfilter/af_ladspa.c
        ./FFmpeg/libavfilter/af_loudnorm.c
        # ./FFmpeg/libavfilter/af_lv2.c
        ./FFmpeg/libavfilter/af_mcompand.c
        ./FFmpeg/libavfilter/af_pan.c
        ./FFmpeg/libavfilter/af_replaygain.c
        ./FFmpeg/libavfilter/af_resample.c
        # ./FFmpeg/libavfilter/af_rubberband.c
        ./FFmpeg/libavfilter/af_sidechaincompress.c
        ./FFmpeg/libavfilter/af_silencedetect.c
        ./FFmpeg/libavfilter/af_silenceremove.c
        # ./FFmpeg/libavfilter/af_sofalizer.c
        ./FFmpeg/libavfilter/af_stereotools.c
        ./FFmpeg/libavfilter/af_stereowiden.c
        ./FFmpeg/libavfilter/af_superequalizer.c
        ./FFmpeg/libavfilter/af_surround.c
        ./FFmpeg/libavfilter/af_tremolo.c
        ./FFmpeg/libavfilter/af_vibrato.c
        ./FFmpeg/libavfilter/af_volume.c
        ./FFmpeg/libavfilter/af_volumedetect.c
        ./FFmpeg/libavfilter/allfilters.c
        ./FFmpeg/libavfilter/asink_anullsink.c
        ./FFmpeg/libavfilter/asrc_anoisesrc.c
        ./FFmpeg/libavfilter/asrc_anullsrc.c
        # ./FFmpeg/libavfilter/asrc_flite.c
        ./FFmpeg/libavfilter/asrc_hilbert.c
        ./FFmpeg/libavfilter/asrc_sinc.c
        ./FFmpeg/libavfilter/asrc_sine.c
        ./FFmpeg/libavfilter/audio.c
        ./FFmpeg/libavfilter/avf_abitscope.c
        ./FFmpeg/libavfilter/avf_ahistogram.c
        ./FFmpeg/libavfilter/avf_aphasemeter.c
        ./FFmpeg/libavfilter/avf_avectorscope.c
        ./FFmpeg/libavfilter/avf_concat.c
        ./FFmpeg/libavfilter/avf_showcqt.c
        ./FFmpeg/libavfilter/avf_showfreqs.c
        ./FFmpeg/libavfilter/avf_showspectrum.c
        ./FFmpeg/libavfilter/avf_showvolume.c
        ./FFmpeg/libavfilter/avf_showwaves.c
        ./FFmpeg/libavfilter/avfilter.c
        ./FFmpeg/libavfilter/avfiltergraph.c
        ./FFmpeg/libavfilter/bbox.c
        ./FFmpeg/libavfilter/boxblur.c
        ./FFmpeg/libavfilter/buffersink.c
        ./FFmpeg/libavfilter/buffersrc.c
        ./FFmpeg/libavfilter/colorspace.c
        ./FFmpeg/libavfilter/colorspacedsp.c
        # ./FFmpeg/libavfilter/colorspacedsp_template.c
        # ./FFmpeg/libavfilter/colorspacedsp_yuv2yuv_template.c
        ./FFmpeg/libavfilter/dnn_backend_native.c
        # ./FFmpeg/libavfilter/dnn_backend_tf.c
        ./FFmpeg/libavfilter/dnn_interface.c
        ./FFmpeg/libavfilter/drawutils.c
        ./FFmpeg/libavfilter/ebur128.c
        ./FFmpeg/libavfilter/f_bench.c
        ./FFmpeg/libavfilter/f_cue.c
        ./FFmpeg/libavfilter/f_drawgraph.c
        ./FFmpeg/libavfilter/f_ebur128.c
        ./FFmpeg/libavfilter/f_graphmonitor.c
        ./FFmpeg/libavfilter/f_interleave.c
        ./FFmpeg/libavfilter/f_loop.c
        ./FFmpeg/libavfilter/f_metadata.c
        ./FFmpeg/libavfilter/f_perms.c
        ./FFmpeg/libavfilter/f_realtime.c
        ./FFmpeg/libavfilter/f_reverse.c
        ./FFmpeg/libavfilter/f_select.c
        ./FFmpeg/libavfilter/f_sendcmd.c
        ./FFmpeg/libavfilter/f_sidedata.c
        ./FFmpeg/libavfilter/f_streamselect.c
        # ./FFmpeg/libavfilter/f_zmq.c
        ./FFmpeg/libavfilter/fifo.c
        # ./FFmpeg/libavfilter/filter_list.c
        ./FFmpeg/libavfilter/formats.c
        ./FFmpeg/libavfilter/framepool.c
        ./FFmpeg/libavfilter/framequeue.c
        ./FFmpeg/libavfilter/framesync.c
        ./FFmpeg/libavfilter/generate_wave_table.c
        ./FFmpeg/libavfilter/graphdump.c
        ./FFmpeg/libavfilter/graphparser.c
        ./FFmpeg/libavfilter/lavfutils.c
        # ./FFmpeg/libavfilter/log2_tab.c
        ./FFmpeg/libavfilter/lswsutils.c
        ./FFmpeg/libavfilter/motion_estimation.c
        # ./FFmpeg/libavfilter/opencl.c
        ./FFmpeg/libavfilter/pthread.c
        # ./FFmpeg/libavfilter/qsvvpp.c
        ./FFmpeg/libavfilter/scale.c
        ./FFmpeg/libavfilter/setpts.c
        ./FFmpeg/libavfilter/settb.c
        ./FFmpeg/libavfilter/signature_lookup.c
        ./FFmpeg/libavfilter/split.c
        ./FFmpeg/libavfilter/src_movie.c
        # ./FFmpeg/libavfilter/tests/drawutils.c
        # ./FFmpeg/libavfilter/tests/filtfmts.c
        # ./FFmpeg/libavfilter/tests/formats.c
        # ./FFmpeg/libavfilter/tests/integral.c
        ./FFmpeg/libavfilter/transform.c
        ./FFmpeg/libavfilter/trim.c
        # ./FFmpeg/libavfilter/vaapi_vpp.c
        ./FFmpeg/libavfilter/vaf_spectrumsynth.c
        ./FFmpeg/libavfilter/vf_alphamerge.c
        ./FFmpeg/libavfilter/vf_amplify.c
        ./FFmpeg/libavfilter/vf_aspect.c
        ./FFmpeg/libavfilter/vf_atadenoise.c
        ./FFmpeg/libavfilter/vf_avgblur.c
        # ./FFmpeg/libavfilter/vf_avgblur_opencl.c
        ./FFmpeg/libavfilter/vf_bbox.c
        ./FFmpeg/libavfilter/vf_bitplanenoise.c
        ./FFmpeg/libavfilter/vf_blackdetect.c
        ./FFmpeg/libavfilter/vf_blackframe.c
        ./FFmpeg/libavfilter/vf_blend.c
        ./FFmpeg/libavfilter/vf_bm3d.c
        ./FFmpeg/libavfilter/vf_boxblur.c
        ./FFmpeg/libavfilter/vf_bwdif.c
        ./FFmpeg/libavfilter/vf_chromakey.c
        ./FFmpeg/libavfilter/vf_ciescope.c
        ./FFmpeg/libavfilter/vf_codecview.c
        ./FFmpeg/libavfilter/vf_colorbalance.c
        ./FFmpeg/libavfilter/vf_colorchannelmixer.c
        ./FFmpeg/libavfilter/vf_colorconstancy.c
        ./FFmpeg/libavfilter/vf_colorkey.c
        ./FFmpeg/libavfilter/vf_colorlevels.c
        ./FFmpeg/libavfilter/vf_colormatrix.c
        ./FFmpeg/libavfilter/vf_colorspace.c
        ./FFmpeg/libavfilter/vf_convolution.c
        # ./FFmpeg/libavfilter/vf_convolution_opencl.c
        ./FFmpeg/libavfilter/vf_convolve.c
        ./FFmpeg/libavfilter/vf_copy.c
        ./FFmpeg/libavfilter/vf_cover_rect.c
        ./FFmpeg/libavfilter/vf_crop.c
        ./FFmpeg/libavfilter/vf_cropdetect.c
        ./FFmpeg/libavfilter/vf_curves.c
        ./FFmpeg/libavfilter/vf_datascope.c
        ./FFmpeg/libavfilter/vf_dctdnoiz.c
        ./FFmpeg/libavfilter/vf_deband.c
        ./FFmpeg/libavfilter/vf_deblock.c
        ./FFmpeg/libavfilter/vf_decimate.c
        ./FFmpeg/libavfilter/vf_deflicker.c
        # ./FFmpeg/libavfilter/vf_deinterlace_qsv.c
        # ./FFmpeg/libavfilter/vf_deinterlace_vaapi.c
        ./FFmpeg/libavfilter/vf_dejudder.c
        ./FFmpeg/libavfilter/vf_delogo.c
        ./FFmpeg/libavfilter/vf_deshake.c
        ./FFmpeg/libavfilter/vf_despill.c
        ./FFmpeg/libavfilter/vf_detelecine.c
        ./FFmpeg/libavfilter/vf_displace.c
        ./FFmpeg/libavfilter/vf_drawbox.c
        # ./FFmpeg/libavfilter/vf_drawtext.c
        ./FFmpeg/libavfilter/vf_edgedetect.c
        ./FFmpeg/libavfilter/vf_elbg.c
        ./FFmpeg/libavfilter/vf_entropy.c
        ./FFmpeg/libavfilter/vf_eq.c
        ./FFmpeg/libavfilter/vf_extractplanes.c
        ./FFmpeg/libavfilter/vf_fade.c
        ./FFmpeg/libavfilter/vf_fftdnoiz.c
        ./FFmpeg/libavfilter/vf_fftfilt.c
        ./FFmpeg/libavfilter/vf_field.c
        ./FFmpeg/libavfilter/vf_fieldhint.c
        ./FFmpeg/libavfilter/vf_fieldmatch.c
        ./FFmpeg/libavfilter/vf_fieldorder.c
        ./FFmpeg/libavfilter/vf_fillborders.c
        ./FFmpeg/libavfilter/vf_find_rect.c
        ./FFmpeg/libavfilter/vf_floodfill.c
        ./FFmpeg/libavfilter/vf_format.c
        ./FFmpeg/libavfilter/vf_fps.c
        ./FFmpeg/libavfilter/vf_framepack.c
        ./FFmpeg/libavfilter/vf_framerate.c
        ./FFmpeg/libavfilter/vf_framestep.c
        # ./FFmpeg/libavfilter/vf_frei0r.c
        ./FFmpeg/libavfilter/vf_fspp.c
        ./FFmpeg/libavfilter/vf_gblur.c
        ./FFmpeg/libavfilter/vf_geq.c
        ./FFmpeg/libavfilter/vf_gradfun.c
        ./FFmpeg/libavfilter/vf_hflip.c
        ./FFmpeg/libavfilter/vf_histeq.c
        ./FFmpeg/libavfilter/vf_histogram.c
        ./FFmpeg/libavfilter/vf_hqdn3d.c
        ./FFmpeg/libavfilter/vf_hqx.c
        ./FFmpeg/libavfilter/vf_hue.c
        ./FFmpeg/libavfilter/vf_hwdownload.c
        ./FFmpeg/libavfilter/vf_hwmap.c
        ./FFmpeg/libavfilter/vf_hwupload.c
        # ./FFmpeg/libavfilter/vf_hwupload_cuda.c
        ./FFmpeg/libavfilter/vf_hysteresis.c
        ./FFmpeg/libavfilter/vf_idet.c
        ./FFmpeg/libavfilter/vf_il.c
        ./FFmpeg/libavfilter/vf_kerndeint.c
        ./FFmpeg/libavfilter/vf_lenscorrection.c
        # ./FFmpeg/libavfilter/vf_lensfun.c
        # ./FFmpeg/libavfilter/vf_libopencv.c
        # ./FFmpeg/libavfilter/vf_libvmaf.c
        ./FFmpeg/libavfilter/vf_limiter.c
        ./FFmpeg/libavfilter/vf_lumakey.c
        ./FFmpeg/libavfilter/vf_lut.c
        ./FFmpeg/libavfilter/vf_lut2.c
        ./FFmpeg/libavfilter/vf_lut3d.c
        ./FFmpeg/libavfilter/vf_maskedclamp.c
        ./FFmpeg/libavfilter/vf_maskedmerge.c
        ./FFmpeg/libavfilter/vf_mcdeint.c
        ./FFmpeg/libavfilter/vf_mergeplanes.c
        ./FFmpeg/libavfilter/vf_mestimate.c
        ./FFmpeg/libavfilter/vf_midequalizer.c
        ./FFmpeg/libavfilter/vf_minterpolate.c
        # ./FFmpeg/libavfilter/vf_misc_vaapi.c
        ./FFmpeg/libavfilter/vf_mix.c
        ./FFmpeg/libavfilter/vf_mpdecimate.c
        ./FFmpeg/libavfilter/vf_neighbor.c
        # ./FFmpeg/libavfilter/vf_neighbor_opencl.c
        ./FFmpeg/libavfilter/vf_nlmeans.c
        ./FFmpeg/libavfilter/vf_nnedi.c
        ./FFmpeg/libavfilter/vf_noise.c
        ./FFmpeg/libavfilter/vf_normalize.c
        ./FFmpeg/libavfilter/vf_null.c
        # ./FFmpeg/libavfilter/vf_ocr.c
        ./FFmpeg/libavfilter/vf_overlay.c
        # ./FFmpeg/libavfilter/vf_overlay_opencl.c
        # ./FFmpeg/libavfilter/vf_overlay_qsv.c
        ./FFmpeg/libavfilter/vf_owdenoise.c
        ./FFmpeg/libavfilter/vf_pad.c
        ./FFmpeg/libavfilter/vf_palettegen.c
        ./FFmpeg/libavfilter/vf_paletteuse.c
        ./FFmpeg/libavfilter/vf_perspective.c
        ./FFmpeg/libavfilter/vf_phase.c
        ./FFmpeg/libavfilter/vf_pixdesctest.c
        ./FFmpeg/libavfilter/vf_pp.c
        ./FFmpeg/libavfilter/vf_pp7.c
        ./FFmpeg/libavfilter/vf_premultiply.c
        # ./FFmpeg/libavfilter/vf_procamp_vaapi.c
        # ./FFmpeg/libavfilter/vf_program_opencl.c
        ./FFmpeg/libavfilter/vf_pseudocolor.c
        ./FFmpeg/libavfilter/vf_psnr.c
        ./FFmpeg/libavfilter/vf_pullup.c
        ./FFmpeg/libavfilter/vf_qp.c
        ./FFmpeg/libavfilter/vf_random.c
        ./FFmpeg/libavfilter/vf_readeia608.c
        ./FFmpeg/libavfilter/vf_readvitc.c
        ./FFmpeg/libavfilter/vf_remap.c
        ./FFmpeg/libavfilter/vf_removegrain.c
        ./FFmpeg/libavfilter/vf_removelogo.c
        ./FFmpeg/libavfilter/vf_repeatfields.c
        ./FFmpeg/libavfilter/vf_rotate.c
        ./FFmpeg/libavfilter/vf_sab.c
        ./FFmpeg/libavfilter/vf_scale.c
        # ./FFmpeg/libavfilter/vf_scale_cuda.c
        # ./FFmpeg/libavfilter/vf_scale_npp.c
        # ./FFmpeg/libavfilter/vf_scale_qsv.c
        # ./FFmpeg/libavfilter/vf_scale_vaapi.c
        ./FFmpeg/libavfilter/vf_selectivecolor.c
        ./FFmpeg/libavfilter/vf_separatefields.c
        ./FFmpeg/libavfilter/vf_setparams.c
        ./FFmpeg/libavfilter/vf_showinfo.c
        ./FFmpeg/libavfilter/vf_showpalette.c
        ./FFmpeg/libavfilter/vf_shuffleframes.c
        ./FFmpeg/libavfilter/vf_shuffleplanes.c
        ./FFmpeg/libavfilter/vf_signalstats.c
        ./FFmpeg/libavfilter/vf_signature.c
        ./FFmpeg/libavfilter/vf_smartblur.c
        ./FFmpeg/libavfilter/vf_spp.c
        ./FFmpeg/libavfilter/vf_sr.c
        ./FFmpeg/libavfilter/vf_ssim.c
        ./FFmpeg/libavfilter/vf_stack.c
        ./FFmpeg/libavfilter/vf_stereo3d.c
        # ./FFmpeg/libavfilter/vf_subtitles.c
        ./FFmpeg/libavfilter/vf_super2xsai.c
        ./FFmpeg/libavfilter/vf_swaprect.c
        ./FFmpeg/libavfilter/vf_swapuv.c
        ./FFmpeg/libavfilter/vf_telecine.c
        ./FFmpeg/libavfilter/vf_threshold.c
        ./FFmpeg/libavfilter/vf_thumbnail.c
        # ./FFmpeg/libavfilter/vf_thumbnail_cuda.c
        ./FFmpeg/libavfilter/vf_tile.c
        ./FFmpeg/libavfilter/vf_tinterlace.c
        ./FFmpeg/libavfilter/vf_tonemap.c
        # ./FFmpeg/libavfilter/vf_tonemap_opencl.c
        ./FFmpeg/libavfilter/vf_transpose.c
        # ./FFmpeg/libavfilter/vf_transpose_npp.c
        ./FFmpeg/libavfilter/vf_unsharp.c
        # ./FFmpeg/libavfilter/vf_unsharp_opencl.c
        ./FFmpeg/libavfilter/vf_uspp.c
        ./FFmpeg/libavfilter/vf_vaguedenoiser.c
        ./FFmpeg/libavfilter/vf_vectorscope.c
        ./FFmpeg/libavfilter/vf_vflip.c
        ./FFmpeg/libavfilter/vf_vfrdet.c
        ./FFmpeg/libavfilter/vf_vibrance.c
        # ./FFmpeg/libavfilter/vf_vidstabdetect.c
        # ./FFmpeg/libavfilter/vf_vidstabtransform.c
        ./FFmpeg/libavfilter/vf_vignette.c
        ./FFmpeg/libavfilter/vf_vmafmotion.c
        # ./FFmpeg/libavfilter/vf_vpp_qsv.c
        ./FFmpeg/libavfilter/vf_w3fdif.c
        ./FFmpeg/libavfilter/vf_waveform.c
        ./FFmpeg/libavfilter/vf_weave.c
        ./FFmpeg/libavfilter/vf_xbr.c
        ./FFmpeg/libavfilter/vf_yadif.c
        # ./FFmpeg/libavfilter/vf_yadif_cuda.c
        ./FFmpeg/libavfilter/vf_zoompan.c
        # ./FFmpeg/libavfilter/vf_zscale.c
        ./FFmpeg/libavfilter/video.c
        # ./FFmpeg/libavfilter/vidstabutils.c
        ./FFmpeg/libavfilter/vsink_nullsink.c
        ./FFmpeg/libavfilter/vsrc_cellauto.c
        ./FFmpeg/libavfilter/vsrc_life.c
        ./FFmpeg/libavfilter/vsrc_mandelbrot.c
        ./FFmpeg/libavfilter/vsrc_mptestsrc.c
        ./FFmpeg/libavfilter/vsrc_testsrc.c
        # ./FFmpeg/libavfilter/x86/af_afir_init.c
        # ./FFmpeg/libavfilter/x86/af_volume_init.c
        # ./FFmpeg/libavfilter/x86/avf_showcqt_init.c
        # ./FFmpeg/libavfilter/x86/colorspacedsp_init.c
        # ./FFmpeg/libavfilter/x86/vf_blend_init.c
        # ./FFmpeg/libavfilter/x86/vf_bwdif_init.c
        # ./FFmpeg/libavfilter/x86/vf_eq.c
        # ./FFmpeg/libavfilter/x86/vf_framerate_init.c
        # ./FFmpeg/libavfilter/x86/vf_fspp_init.c
        # ./FFmpeg/libavfilter/x86/vf_gradfun_init.c
        # ./FFmpeg/libavfilter/x86/vf_hflip_init.c
        # ./FFmpeg/libavfilter/x86/vf_hqdn3d_init.c
        # ./FFmpeg/libavfilter/x86/vf_idet_init.c
        # ./FFmpeg/libavfilter/x86/vf_limiter_init.c
        # ./FFmpeg/libavfilter/x86/vf_maskedmerge_init.c
        # ./FFmpeg/libavfilter/x86/vf_noise.c
        # ./FFmpeg/libavfilter/x86/vf_overlay_init.c
        # ./FFmpeg/libavfilter/x86/vf_pp7_init.c
        # ./FFmpeg/libavfilter/x86/vf_psnr_init.c
        # ./FFmpeg/libavfilter/x86/vf_pullup_init.c
        # ./FFmpeg/libavfilter/x86/vf_removegrain_init.c
        # ./FFmpeg/libavfilter/x86/vf_spp.c
        # ./FFmpeg/libavfilter/x86/vf_ssim_init.c
        # ./FFmpeg/libavfilter/x86/vf_stereo3d_init.c
        # ./FFmpeg/libavfilter/x86/vf_threshold_init.c
        # ./FFmpeg/libavfilter/x86/vf_tinterlace_init.c
        # ./FFmpeg/libavfilter/x86/vf_w3fdif_init.c
        # ./FFmpeg/libavfilter/x86/vf_yadif_init.c
        ./FFmpeg/libavfilter/yadif_common.c
        ./FFmpeg/libavformat/3dostr.c
        ./FFmpeg/libavformat/4xm.c
        ./FFmpeg/libavformat/a64.c
        ./FFmpeg/libavformat/aacdec.c
        ./FFmpeg/libavformat/aadec.c
        ./FFmpeg/libavformat/ac3dec.c
        ./FFmpeg/libavformat/acm.c
        ./FFmpeg/libavformat/act.c
        ./FFmpeg/libavformat/adp.c
        ./FFmpeg/libavformat/ads.c
        ./FFmpeg/libavformat/adtsenc.c
        ./FFmpeg/libavformat/adxdec.c
        ./FFmpeg/libavformat/aea.c
        ./FFmpeg/libavformat/afc.c
        ./FFmpeg/libavformat/aiffdec.c
        ./FFmpeg/libavformat/aiffenc.c
        ./FFmpeg/libavformat/aixdec.c
        ./FFmpeg/libavformat/allformats.c
        ./FFmpeg/libavformat/amr.c
        ./FFmpeg/libavformat/anm.c
        ./FFmpeg/libavformat/apc.c
        ./FFmpeg/libavformat/ape.c
        ./FFmpeg/libavformat/apetag.c
        ./FFmpeg/libavformat/apngdec.c
        ./FFmpeg/libavformat/apngenc.c
        ./FFmpeg/libavformat/aptxdec.c
        ./FFmpeg/libavformat/aqtitledec.c
        ./FFmpeg/libavformat/asf.c
        ./FFmpeg/libavformat/asfcrypt.c
        ./FFmpeg/libavformat/asfdec_f.c
        ./FFmpeg/libavformat/asfdec_o.c
        ./FFmpeg/libavformat/asfenc.c
        # ./FFmpeg/libavformat/assdec.c
        # ./FFmpeg/libavformat/assenc.c
        ./FFmpeg/libavformat/ast.c
        ./FFmpeg/libavformat/astdec.c
        ./FFmpeg/libavformat/astenc.c
        ./FFmpeg/libavformat/async.c
        ./FFmpeg/libavformat/au.c
        ./FFmpeg/libavformat/audiointerleave.c
        ./FFmpeg/libavformat/av1.c
        ./FFmpeg/libavformat/avc.c
        ./FFmpeg/libavformat/avidec.c
        ./FFmpeg/libavformat/avienc.c
        ./FFmpeg/libavformat/avio.c
        ./FFmpeg/libavformat/aviobuf.c
        ./FFmpeg/libavformat/avisynth.c
        ./FFmpeg/libavformat/avlanguage.c
        ./FFmpeg/libavformat/avr.c
        ./FFmpeg/libavformat/avs.c
        ./FFmpeg/libavformat/bethsoftvid.c
        ./FFmpeg/libavformat/bfi.c
        ./FFmpeg/libavformat/bink.c
        ./FFmpeg/libavformat/bintext.c
        ./FFmpeg/libavformat/bit.c
        # ./FFmpeg/libavformat/bluray.c
        ./FFmpeg/libavformat/bmv.c
        ./FFmpeg/libavformat/boadec.c
        ./FFmpeg/libavformat/brstm.c
        ./FFmpeg/libavformat/c93.c
        ./FFmpeg/libavformat/cache.c
        ./FFmpeg/libavformat/caf.c
        ./FFmpeg/libavformat/cafdec.c
        ./FFmpeg/libavformat/cafenc.c
        ./FFmpeg/libavformat/cavsvideodec.c
        ./FFmpeg/libavformat/cdg.c
        ./FFmpeg/libavformat/cdxl.c
        # ./FFmpeg/libavformat/chromaprint.c
        ./FFmpeg/libavformat/cinedec.c
        # ./FFmpeg/libavformat/codec2.c
        ./FFmpeg/libavformat/concat.c
        ./FFmpeg/libavformat/concatdec.c
        ./FFmpeg/libavformat/crcenc.c
        ./FFmpeg/libavformat/crypto.c
        ./FFmpeg/libavformat/cutils.c
        # ./FFmpeg/libavformat/dash.c
        # ./FFmpeg/libavformat/dashdec.c
        # ./FFmpeg/libavformat/dashenc.c
        ./FFmpeg/libavformat/data_uri.c
        ./FFmpeg/libavformat/dauddec.c
        ./FFmpeg/libavformat/daudenc.c
        # ./FFmpeg/libavformat/davs2.c
        ./FFmpeg/libavformat/dcstr.c
        # ./FFmpeg/libavformat/demuxer_list.c
        ./FFmpeg/libavformat/dfa.c
        ./FFmpeg/libavformat/diracdec.c
        ./FFmpeg/libavformat/dnxhddec.c
        ./FFmpeg/libavformat/dsfdec.c
        ./FFmpeg/libavformat/dsicin.c
        ./FFmpeg/libavformat/dss.c
        ./FFmpeg/libavformat/dtsdec.c
        ./FFmpeg/libavformat/dtshddec.c
        ./FFmpeg/libavformat/dump.c
        ./FFmpeg/libavformat/dv.c
        ./FFmpeg/libavformat/dvbsub.c
        ./FFmpeg/libavformat/dvbtxt.c
        ./FFmpeg/libavformat/dvenc.c
        ./FFmpeg/libavformat/dxa.c
        ./FFmpeg/libavformat/eacdata.c
        ./FFmpeg/libavformat/electronicarts.c
        ./FFmpeg/libavformat/epafdec.c
        ./FFmpeg/libavformat/ffmetadec.c
        ./FFmpeg/libavformat/ffmetaenc.c
        ./FFmpeg/libavformat/fifo.c
        ./FFmpeg/libavformat/fifo_test.c
        ./FFmpeg/libavformat/file.c
        # ./FFmpeg/libavformat/file_open.c
        ./FFmpeg/libavformat/filmstripdec.c
        ./FFmpeg/libavformat/filmstripenc.c
        ./FFmpeg/libavformat/fitsdec.c
        ./FFmpeg/libavformat/fitsenc.c
        ./FFmpeg/libavformat/flac_picture.c
        ./FFmpeg/libavformat/flacdec.c
        ./FFmpeg/libavformat/flacenc.c
        ./FFmpeg/libavformat/flacenc_header.c
        ./FFmpeg/libavformat/flic.c
        ./FFmpeg/libavformat/flvdec.c
        ./FFmpeg/libavformat/flvenc.c
        ./FFmpeg/libavformat/format.c
        ./FFmpeg/libavformat/framecrcenc.c
        ./FFmpeg/libavformat/framehash.c
        ./FFmpeg/libavformat/frmdec.c
        ./FFmpeg/libavformat/fsb.c
        ./FFmpeg/libavformat/ftp.c
        ./FFmpeg/libavformat/g722.c
        ./FFmpeg/libavformat/g723_1.c
        ./FFmpeg/libavformat/g726.c
        ./FFmpeg/libavformat/g729dec.c
        ./FFmpeg/libavformat/gdv.c
        ./FFmpeg/libavformat/genh.c
        ./FFmpeg/libavformat/gif.c
        ./FFmpeg/libavformat/gifdec.c
        # ./FFmpeg/libavformat/golomb_tab.c
        # ./FFmpeg/libavformat/gopher.c
        ./FFmpeg/libavformat/gsmdec.c
        ./FFmpeg/libavformat/gxf.c
        ./FFmpeg/libavformat/gxfenc.c
        ./FFmpeg/libavformat/h261dec.c
        ./FFmpeg/libavformat/h263dec.c
        ./FFmpeg/libavformat/h264dec.c
        ./FFmpeg/libavformat/hashenc.c
        ./FFmpeg/libavformat/hdsenc.c
        ./FFmpeg/libavformat/hevc.c
        ./FFmpeg/libavformat/hevcdec.c
        ./FFmpeg/libavformat/hls.c
        ./FFmpeg/libavformat/hlsenc.c
        ./FFmpeg/libavformat/hlsplaylist.c
        ./FFmpeg/libavformat/hlsproto.c
        ./FFmpeg/libavformat/hnm.c
        # ./FFmpeg/libavformat/http.c
        # ./FFmpeg/libavformat/httpauth.c
        # ./FFmpeg/libavformat/icecast.c
        ./FFmpeg/libavformat/icodec.c
        ./FFmpeg/libavformat/icoenc.c
        ./FFmpeg/libavformat/id3v1.c
        ./FFmpeg/libavformat/id3v2.c
        ./FFmpeg/libavformat/id3v2enc.c
        ./FFmpeg/libavformat/idcin.c
        ./FFmpeg/libavformat/idroqdec.c
        ./FFmpeg/libavformat/idroqenc.c
        ./FFmpeg/libavformat/iff.c
        # ./FFmpeg/libavformat/ilbc.c
        ./FFmpeg/libavformat/img2.c
        ./FFmpeg/libavformat/img2_alias_pix.c
        ./FFmpeg/libavformat/img2_brender_pix.c
        ./FFmpeg/libavformat/img2dec.c
        ./FFmpeg/libavformat/img2enc.c
        ./FFmpeg/libavformat/ingenientdec.c
        # ./FFmpeg/libavformat/ip.c
        ./FFmpeg/libavformat/ipmovie.c
        ./FFmpeg/libavformat/ircam.c
        ./FFmpeg/libavformat/ircamdec.c
        ./FFmpeg/libavformat/ircamenc.c
        ./FFmpeg/libavformat/isom.c
        ./FFmpeg/libavformat/iss.c
        ./FFmpeg/libavformat/iv8.c
        ./FFmpeg/libavformat/ivfdec.c
        ./FFmpeg/libavformat/ivfenc.c
        ./FFmpeg/libavformat/jacosubdec.c
        ./FFmpeg/libavformat/jacosubenc.c
        ./FFmpeg/libavformat/jvdec.c
        ./FFmpeg/libavformat/latmenc.c
        # ./FFmpeg/libavformat/libgme.c
        # ./FFmpeg/libavformat/libmodplug.c
        # ./FFmpeg/libavformat/libopenmpt.c
        # ./FFmpeg/libavformat/librtmp.c
        # ./FFmpeg/libavformat/libsmbclient.c
        # ./FFmpeg/libavformat/libsrt.c
        # ./FFmpeg/libavformat/libssh.c
        ./FFmpeg/libavformat/lmlm4.c
        ./FFmpeg/libavformat/loasdec.c
        # ./FFmpeg/libavformat/log2_tab.c
        ./FFmpeg/libavformat/lrc.c
        ./FFmpeg/libavformat/lrcdec.c
        ./FFmpeg/libavformat/lrcenc.c
        ./FFmpeg/libavformat/lvfdec.c
        ./FFmpeg/libavformat/lxfdec.c
        ./FFmpeg/libavformat/m4vdec.c
        ## BZ2 is missing
        # ./FFmpeg/libavformat/matroska.c
        # ./FFmpeg/libavformat/matroskadec.c
        # ./FFmpeg/libavformat/matroskaenc.c
        ./FFmpeg/libavformat/md5proto.c
        ./FFmpeg/libavformat/metadata.c
        ./FFmpeg/libavformat/mgsts.c
        ./FFmpeg/libavformat/microdvddec.c
        ./FFmpeg/libavformat/microdvdenc.c
        ./FFmpeg/libavformat/mj2kdec.c
        ./FFmpeg/libavformat/mkvtimestamp_v2.c
        ./FFmpeg/libavformat/mlpdec.c
        ./FFmpeg/libavformat/mlvdec.c
        ./FFmpeg/libavformat/mm.c
        ./FFmpeg/libavformat/mmf.c
        ./FFmpeg/libavformat/mms.c
        ./FFmpeg/libavformat/mmsh.c
        # ./FFmpeg/libavformat/mmst.c
        ./FFmpeg/libavformat/mov.c
        ./FFmpeg/libavformat/mov_chan.c
        ./FFmpeg/libavformat/mov_esds.c
        ./FFmpeg/libavformat/movenc.c
        ./FFmpeg/libavformat/movenccenc.c
        ./FFmpeg/libavformat/movenchint.c
        ./FFmpeg/libavformat/mp3dec.c
        ./FFmpeg/libavformat/mp3enc.c
        ./FFmpeg/libavformat/mpc.c
        ./FFmpeg/libavformat/mpc8.c
        ./FFmpeg/libavformat/mpeg.c
        ./FFmpeg/libavformat/mpegenc.c
        ./FFmpeg/libavformat/mpegts.c
        ./FFmpeg/libavformat/mpegtsenc.c
        ./FFmpeg/libavformat/mpegvideodec.c
        ./FFmpeg/libavformat/mpjpeg.c
        ./FFmpeg/libavformat/mpjpegdec.c
        ./FFmpeg/libavformat/mpl2dec.c
        ./FFmpeg/libavformat/mpsubdec.c
        ./FFmpeg/libavformat/msf.c
        ./FFmpeg/libavformat/msnwc_tcp.c
        ./FFmpeg/libavformat/mtaf.c
        ./FFmpeg/libavformat/mtv.c
        ./FFmpeg/libavformat/musx.c
        ./FFmpeg/libavformat/mux.c
        # ./FFmpeg/libavformat/muxer_list.c
        ./FFmpeg/libavformat/mvdec.c
        ./FFmpeg/libavformat/mvi.c
        ./FFmpeg/libavformat/mxf.c
        ./FFmpeg/libavformat/mxfdec.c
        ./FFmpeg/libavformat/mxfenc.c
        ./FFmpeg/libavformat/mxg.c
        ./FFmpeg/libavformat/ncdec.c
        # ./FFmpeg/libavformat/network.c
        ./FFmpeg/libavformat/nistspheredec.c
        ./FFmpeg/libavformat/nspdec.c
        ./FFmpeg/libavformat/nsvdec.c
        ./FFmpeg/libavformat/nullenc.c
        ./FFmpeg/libavformat/nut.c
        ./FFmpeg/libavformat/nutdec.c
        ./FFmpeg/libavformat/nutenc.c
        ./FFmpeg/libavformat/nuv.c
        ./FFmpeg/libavformat/oggdec.c
        ./FFmpeg/libavformat/oggenc.c
        ./FFmpeg/libavformat/oggparsecelt.c
        ./FFmpeg/libavformat/oggparsedaala.c
        ./FFmpeg/libavformat/oggparsedirac.c
        ./FFmpeg/libavformat/oggparseflac.c
        ./FFmpeg/libavformat/oggparseogm.c
        ./FFmpeg/libavformat/oggparseopus.c
        ./FFmpeg/libavformat/oggparseskeleton.c
        ./FFmpeg/libavformat/oggparsespeex.c
        ./FFmpeg/libavformat/oggparsetheora.c
        ./FFmpeg/libavformat/oggparsevorbis.c
        ./FFmpeg/libavformat/oggparsevp8.c
        ./FFmpeg/libavformat/oma.c
        ./FFmpeg/libavformat/omadec.c
        ./FFmpeg/libavformat/omaenc.c
        ./FFmpeg/libavformat/options.c
        ./FFmpeg/libavformat/os_support.c
        ./FFmpeg/libavformat/paf.c
        ./FFmpeg/libavformat/pcm.c
        ./FFmpeg/libavformat/pcmdec.c
        ./FFmpeg/libavformat/pcmenc.c
        ./FFmpeg/libavformat/pjsdec.c
        ./FFmpeg/libavformat/pmpdec.c
        ./FFmpeg/libavformat/prompeg.c
        # ./FFmpeg/libavformat/protocol_list.c
        ./FFmpeg/libavformat/protocols.c
        ./FFmpeg/libavformat/psxstr.c
        ./FFmpeg/libavformat/pva.c
        ./FFmpeg/libavformat/pvfdec.c
        ./FFmpeg/libavformat/qcp.c
        ./FFmpeg/libavformat/qtpalette.c
        ./FFmpeg/libavformat/r3d.c
        ./FFmpeg/libavformat/rawdec.c
        ./FFmpeg/libavformat/rawenc.c
        ./FFmpeg/libavformat/rawutils.c
        ./FFmpeg/libavformat/rawvideodec.c
        ./FFmpeg/libavformat/rdt.c
        ./FFmpeg/libavformat/realtextdec.c
        ./FFmpeg/libavformat/redspark.c
        ./FFmpeg/libavformat/replaygain.c
        ./FFmpeg/libavformat/riff.c
        ./FFmpeg/libavformat/riffdec.c
        ./FFmpeg/libavformat/riffenc.c
        ./FFmpeg/libavformat/rl2.c
        ./FFmpeg/libavformat/rm.c
        ./FFmpeg/libavformat/rmdec.c
        ./FFmpeg/libavformat/rmenc.c
        ./FFmpeg/libavformat/rmsipr.c
        ./FFmpeg/libavformat/rpl.c
        ./FFmpeg/libavformat/rsd.c
        ./FFmpeg/libavformat/rso.c
        ./FFmpeg/libavformat/rsodec.c
        ./FFmpeg/libavformat/rsoenc.c
        # ./FFmpeg/libavformat/rtmpcrypt.c
        # ./FFmpeg/libavformat/rtmpdh.c
        # ./FFmpeg/libavformat/rtmpdigest.c
        # ./FFmpeg/libavformat/rtmphttp.c
        # ./FFmpeg/libavformat/rtmppkt.c
        # ./FFmpeg/libavformat/rtmpproto.c
        ./FFmpeg/libavformat/rtp.c
        # ./FFmpeg/libavformat/rtpdec.c
        # ./FFmpeg/libavformat/rtpdec_ac3.c
        # ./FFmpeg/libavformat/rtpdec_amr.c
        # ./FFmpeg/libavformat/rtpdec_asf.c
        # ./FFmpeg/libavformat/rtpdec_dv.c
        # ./FFmpeg/libavformat/rtpdec_g726.c
        # ./FFmpeg/libavformat/rtpdec_h261.c
        # ./FFmpeg/libavformat/rtpdec_h263.c
        # ./FFmpeg/libavformat/rtpdec_h263_rfc2190.c
        # ./FFmpeg/libavformat/rtpdec_h264.c
        # ./FFmpeg/libavformat/rtpdec_hevc.c
        # ./FFmpeg/libavformat/rtpdec_ilbc.c
        # ./FFmpeg/libavformat/rtpdec_jpeg.c
        # ./FFmpeg/libavformat/rtpdec_latm.c
        # ./FFmpeg/libavformat/rtpdec_mpa_robust.c
        # ./FFmpeg/libavformat/rtpdec_mpeg12.c
        # ./FFmpeg/libavformat/rtpdec_mpeg4.c
        # ./FFmpeg/libavformat/rtpdec_mpegts.c
        # ./FFmpeg/libavformat/rtpdec_qcelp.c
        # ./FFmpeg/libavformat/rtpdec_qdm2.c
        # ./FFmpeg/libavformat/rtpdec_qt.c
        # ./FFmpeg/libavformat/rtpdec_rfc4175.c
        # ./FFmpeg/libavformat/rtpdec_svq3.c
        # ./FFmpeg/libavformat/rtpdec_vc2hq.c
        # ./FFmpeg/libavformat/rtpdec_vp8.c
        # ./FFmpeg/libavformat/rtpdec_vp9.c
        # ./FFmpeg/libavformat/rtpdec_xiph.c
        # ./FFmpeg/libavformat/rtpenc.c
        # ./FFmpeg/libavformat/rtpenc_aac.c
        # ./FFmpeg/libavformat/rtpenc_amr.c
        ./FFmpeg/libavformat/rtpenc_chain.c
        # ./FFmpeg/libavformat/rtpenc_h261.c
        # ./FFmpeg/libavformat/rtpenc_h263.c
        # ./FFmpeg/libavformat/rtpenc_h263_rfc2190.c
        # ./FFmpeg/libavformat/rtpenc_h264_hevc.c
        # ./FFmpeg/libavformat/rtpenc_jpeg.c
        # ./FFmpeg/libavformat/rtpenc_latm.c
        # ./FFmpeg/libavformat/rtpenc_mpegts.c
        # ./FFmpeg/libavformat/rtpenc_mpv.c
        # ./FFmpeg/libavformat/rtpenc_vc2hq.c
        # ./FFmpeg/libavformat/rtpenc_vp8.c
        # ./FFmpeg/libavformat/rtpenc_vp9.c
        # ./FFmpeg/libavformat/rtpenc_xiph.c
        # ./FFmpeg/libavformat/rtpproto.c
        # ./FFmpeg/libavformat/rtsp.c
        # ./FFmpeg/libavformat/rtspdec.c
        # ./FFmpeg/libavformat/rtspenc.c
        ./FFmpeg/libavformat/s337m.c
        ./FFmpeg/libavformat/samidec.c
        # ./FFmpeg/libavformat/sapdec.c
        # ./FFmpeg/libavformat/sapenc.c
        ./FFmpeg/libavformat/sauce.c
        ./FFmpeg/libavformat/sbcdec.c
        ./FFmpeg/libavformat/sbgdec.c
        ./FFmpeg/libavformat/sccdec.c
        ./FFmpeg/libavformat/sccenc.c
        # ./FFmpeg/libavformat/sctp.c
        ./FFmpeg/libavformat/sdp.c
        ./FFmpeg/libavformat/sdr2.c
        ./FFmpeg/libavformat/sdsdec.c
        ./FFmpeg/libavformat/sdxdec.c
        ./FFmpeg/libavformat/segafilm.c
        ./FFmpeg/libavformat/segafilmenc.c
        ./FFmpeg/libavformat/segment.c
        ./FFmpeg/libavformat/serdec.c
        ./FFmpeg/libavformat/shortendec.c
        ./FFmpeg/libavformat/sierravmd.c
        ./FFmpeg/libavformat/siff.c
        ./FFmpeg/libavformat/smacker.c
        ./FFmpeg/libavformat/smjpeg.c
        ./FFmpeg/libavformat/smjpegdec.c
        ./FFmpeg/libavformat/smjpegenc.c
        ./FFmpeg/libavformat/smoothstreamingenc.c
        ./FFmpeg/libavformat/smush.c
        ./FFmpeg/libavformat/sol.c
        ./FFmpeg/libavformat/soxdec.c
        ./FFmpeg/libavformat/soxenc.c
        ./FFmpeg/libavformat/spdif.c
        ./FFmpeg/libavformat/spdifdec.c
        ./FFmpeg/libavformat/spdifenc.c
        ./FFmpeg/libavformat/srtdec.c
        ./FFmpeg/libavformat/srtenc.c
        ./FFmpeg/libavformat/srtp.c
        ./FFmpeg/libavformat/srtpproto.c
        ./FFmpeg/libavformat/stldec.c
        ./FFmpeg/libavformat/subfile.c
        ./FFmpeg/libavformat/subtitles.c
        ./FFmpeg/libavformat/subviewer1dec.c
        ./FFmpeg/libavformat/subviewerdec.c
        ./FFmpeg/libavformat/supdec.c
        ./FFmpeg/libavformat/supenc.c
        ./FFmpeg/libavformat/svag.c
        ./FFmpeg/libavformat/swf.c
        ./FFmpeg/libavformat/swfdec.c
        ./FFmpeg/libavformat/swfenc.c
        ./FFmpeg/libavformat/takdec.c
        # ./FFmpeg/libavformat/tcp.c
        ./FFmpeg/libavformat/tedcaptionsdec.c
        ./FFmpeg/libavformat/tee.c
        ./FFmpeg/libavformat/tee_common.c
        ./FFmpeg/libavformat/teeproto.c
        # ./FFmpeg/libavformat/tests/fifo_muxer.c
        # ./FFmpeg/libavformat/tests/movenc.c
        # ./FFmpeg/libavformat/tests/noproxy.c
        # ./FFmpeg/libavformat/tests/rtmpdh.c
        # ./FFmpeg/libavformat/tests/seek.c
        # ./FFmpeg/libavformat/tests/srtp.c
        # ./FFmpeg/libavformat/tests/url.c
        ./FFmpeg/libavformat/thp.c
        ./FFmpeg/libavformat/tiertexseq.c
        # ./FFmpeg/libavformat/tls.c
        # ./FFmpeg/libavformat/tls_gnutls.c
        # ./FFmpeg/libavformat/tls_libtls.c
        # ./FFmpeg/libavformat/tls_mbedtls.c
        # ./FFmpeg/libavformat/tls_openssl.c
        # ./FFmpeg/libavformat/tls_schannel.c
        # ./FFmpeg/libavformat/tls_securetransport.c
        ./FFmpeg/libavformat/tmv.c
        ./FFmpeg/libavformat/tta.c
        ./FFmpeg/libavformat/ttaenc.c
        ./FFmpeg/libavformat/tty.c
        ./FFmpeg/libavformat/txd.c
        ./FFmpeg/libavformat/ty.c
        # ./FFmpeg/libavformat/udp.c
        ./FFmpeg/libavformat/uncodedframecrcenc.c
        # ./FFmpeg/libavformat/unix.c
        ./FFmpeg/libavformat/url.c
        ./FFmpeg/libavformat/urldecode.c
        ./FFmpeg/libavformat/utils.c
        ./FFmpeg/libavformat/v210.c
        ./FFmpeg/libavformat/vag.c
        # ./FFmpeg/libavformat/vapoursynth.c
        ./FFmpeg/libavformat/vc1dec.c
        ./FFmpeg/libavformat/vc1test.c
        ./FFmpeg/libavformat/vc1testenc.c
        ./FFmpeg/libavformat/vivo.c
        ./FFmpeg/libavformat/voc.c
        ./FFmpeg/libavformat/voc_packet.c
        ./FFmpeg/libavformat/vocdec.c
        ./FFmpeg/libavformat/vocenc.c
        ./FFmpeg/libavformat/vorbiscomment.c
        ./FFmpeg/libavformat/vpcc.c
        ./FFmpeg/libavformat/vpk.c
        ./FFmpeg/libavformat/vplayerdec.c
        ./FFmpeg/libavformat/vqf.c
        ./FFmpeg/libavformat/w64.c
        ./FFmpeg/libavformat/wavdec.c
        ./FFmpeg/libavformat/wavenc.c
        ./FFmpeg/libavformat/wc3movie.c
        ./FFmpeg/libavformat/webm_chunk.c
        ./FFmpeg/libavformat/webmdashenc.c
        ./FFmpeg/libavformat/webpenc.c
        ./FFmpeg/libavformat/webvttdec.c
        ./FFmpeg/libavformat/webvttenc.c
        ./FFmpeg/libavformat/westwood_aud.c
        ./FFmpeg/libavformat/westwood_vqa.c
        ./FFmpeg/libavformat/wsddec.c
        ./FFmpeg/libavformat/wtv_common.c
        ./FFmpeg/libavformat/wtvdec.c
        ./FFmpeg/libavformat/wtvenc.c
        ./FFmpeg/libavformat/wv.c
        ./FFmpeg/libavformat/wvdec.c
        ./FFmpeg/libavformat/wvedec.c
        ./FFmpeg/libavformat/wvenc.c
        ./FFmpeg/libavformat/xa.c
        ./FFmpeg/libavformat/xmv.c
        ./FFmpeg/libavformat/xvag.c
        ./FFmpeg/libavformat/xwma.c
        ./FFmpeg/libavformat/yop.c
        ./FFmpeg/libavformat/yuv4mpegdec.c
        ./FFmpeg/libavformat/yuv4mpegenc.c
        # ./FFmpeg/libavresample/aarch64/audio_convert_init.c
        # ./FFmpeg/libavresample/aarch64/neontest.c
        # ./FFmpeg/libavresample/aarch64/resample_init.c
        # ./FFmpeg/libavresample/arm/audio_convert_init.c
        # ./FFmpeg/libavresample/arm/neontest.c
        # ./FFmpeg/libavresample/arm/resample_init.c
        ./FFmpeg/libavresample/audio_convert.c
        ./FFmpeg/libavresample/audio_data.c
        ./FFmpeg/libavresample/audio_mix.c
        ./FFmpeg/libavresample/audio_mix_matrix.c
        ./FFmpeg/libavresample/dither.c
        ./FFmpeg/libavresample/options.c
        ./FFmpeg/libavresample/resample.c
        # ./FFmpeg/libavresample/resample_template.c
        # ./FFmpeg/libavresample/tests/avresample.c
        ./FFmpeg/libavresample/utils.c
        # ./FFmpeg/libavresample/x86/audio_convert_init.c
        # ./FFmpeg/libavresample/x86/audio_mix_init.c
        # ./FFmpeg/libavresample/x86/dither_init.c
        # ./FFmpeg/libavresample/x86/w64xmmtest.c
        # ./FFmpeg/libavutil/aarch64/cpu.c
        # ./FFmpeg/libavutil/aarch64/float_dsp_init.c
        ./FFmpeg/libavutil/adler32.c
        ./FFmpeg/libavutil/aes.c
        ./FFmpeg/libavutil/aes_ctr.c
        # ./FFmpeg/libavutil/arm/cpu.c
        # ./FFmpeg/libavutil/arm/float_dsp_init_arm.c
        # ./FFmpeg/libavutil/arm/float_dsp_init_neon.c
        # ./FFmpeg/libavutil/arm/float_dsp_init_vfp.c
        ./FFmpeg/libavutil/audio_fifo.c
        ./FFmpeg/libavutil/avstring.c
        ./FFmpeg/libavutil/base64.c
        ./FFmpeg/libavutil/blowfish.c
        ./FFmpeg/libavutil/bprint.c
        ./FFmpeg/libavutil/buffer.c
        ./FFmpeg/libavutil/camellia.c
        ./FFmpeg/libavutil/cast5.c
        ./FFmpeg/libavutil/channel_layout.c
        ./FFmpeg/libavutil/color_utils.c
        ./FFmpeg/libavutil/cpu.c
        ./FFmpeg/libavutil/crc.c
        ./FFmpeg/libavutil/des.c
        ./FFmpeg/libavutil/dict.c
        ./FFmpeg/libavutil/display.c
        ./FFmpeg/libavutil/downmix_info.c
        ./FFmpeg/libavutil/encryption_info.c
        ./FFmpeg/libavutil/error.c
        ./FFmpeg/libavutil/eval.c
        ./FFmpeg/libavutil/fifo.c
        ./FFmpeg/libavutil/file.c
        ./FFmpeg/libavutil/file_open.c
        ./FFmpeg/libavutil/fixed_dsp.c
        ./FFmpeg/libavutil/float_dsp.c
        ./FFmpeg/libavutil/frame.c
        ./FFmpeg/libavutil/hash.c
        ./FFmpeg/libavutil/hmac.c
        ./FFmpeg/libavutil/hwcontext.c
        # ./FFmpeg/libavutil/hwcontext_cuda.c
        # ./FFmpeg/libavutil/hwcontext_d3d11va.c
        # ./FFmpeg/libavutil/hwcontext_drm.c
        # ./FFmpeg/libavutil/hwcontext_dxva2.c
        ./FFmpeg/libavutil/hwcontext_mediacodec.c
        # ./FFmpeg/libavutil/hwcontext_opencl.c
        # ./FFmpeg/libavutil/hwcontext_qsv.c
        # ./FFmpeg/libavutil/hwcontext_vaapi.c
        # ./FFmpeg/libavutil/hwcontext_vdpau.c
        # ./FFmpeg/libavutil/hwcontext_videotoolbox.c
        ./FFmpeg/libavutil/imgutils.c
        ./FFmpeg/libavutil/integer.c
        ./FFmpeg/libavutil/intmath.c
        ./FFmpeg/libavutil/lfg.c
        ./FFmpeg/libavutil/lls.c
        ./FFmpeg/libavutil/log.c
        ./FFmpeg/libavutil/log2_tab.c
        ./FFmpeg/libavutil/lzo.c
        ./FFmpeg/libavutil/mastering_display_metadata.c
        ./FFmpeg/libavutil/mathematics.c
        ./FFmpeg/libavutil/md5.c
        ./FFmpeg/libavutil/mem.c
        ./FFmpeg/libavutil/mips/float_dsp_mips.c
        ./FFmpeg/libavutil/murmur3.c
        ./FFmpeg/libavutil/opt.c
        ./FFmpeg/libavutil/parseutils.c
        ./FFmpeg/libavutil/pca.c
        ./FFmpeg/libavutil/pixdesc.c
        ./FFmpeg/libavutil/pixelutils.c
        # ./FFmpeg/libavutil/ppc/cpu.c
        # ./FFmpeg/libavutil/ppc/float_dsp_altivec.c
        # ./FFmpeg/libavutil/ppc/float_dsp_init.c
        # ./FFmpeg/libavutil/ppc/float_dsp_vsx.c
        ./FFmpeg/libavutil/random_seed.c
        ./FFmpeg/libavutil/rational.c
        ./FFmpeg/libavutil/rc4.c
        ./FFmpeg/libavutil/reverse.c
        ./FFmpeg/libavutil/ripemd.c
        ./FFmpeg/libavutil/samplefmt.c
        ./FFmpeg/libavutil/sha.c
        ./FFmpeg/libavutil/sha512.c
        ./FFmpeg/libavutil/slicethread.c
        ./FFmpeg/libavutil/spherical.c
        ./FFmpeg/libavutil/stereo3d.c
        ./FFmpeg/libavutil/tea.c
        # ./FFmpeg/libavutil/tests/adler32.c
        # ./FFmpeg/libavutil/tests/aes.c
        # ./FFmpeg/libavutil/tests/aes_ctr.c
        # ./FFmpeg/libavutil/tests/audio_fifo.c
        # ./FFmpeg/libavutil/tests/avstring.c
        # ./FFmpeg/libavutil/tests/base64.c
        # ./FFmpeg/libavutil/tests/blowfish.c
        # ./FFmpeg/libavutil/tests/bprint.c
        # ./FFmpeg/libavutil/tests/camellia.c
        # ./FFmpeg/libavutil/tests/cast5.c
        # ./FFmpeg/libavutil/tests/color_utils.c
        # ./FFmpeg/libavutil/tests/cpu.c
        # ./FFmpeg/libavutil/tests/cpu_init.c
        # ./FFmpeg/libavutil/tests/crc.c
        # ./FFmpeg/libavutil/tests/des.c
        # ./FFmpeg/libavutil/tests/dict.c
        # ./FFmpeg/libavutil/tests/display.c
        # ./FFmpeg/libavutil/tests/encryption_info.c
        # ./FFmpeg/libavutil/tests/error.c
        # ./FFmpeg/libavutil/tests/eval.c
        # ./FFmpeg/libavutil/tests/fifo.c
        # ./FFmpeg/libavutil/tests/file.c
        # ./FFmpeg/libavutil/tests/hash.c
        # ./FFmpeg/libavutil/tests/hmac.c
        # ./FFmpeg/libavutil/tests/hwdevice.c
        # ./FFmpeg/libavutil/tests/imgutils.c
        # ./FFmpeg/libavutil/tests/integer.c
        # ./FFmpeg/libavutil/tests/lfg.c
        # ./FFmpeg/libavutil/tests/lls.c
        # ./FFmpeg/libavutil/tests/log.c
        # ./FFmpeg/libavutil/tests/lzo.c
        # ./FFmpeg/libavutil/tests/md5.c
        # ./FFmpeg/libavutil/tests/murmur3.c
        # ./FFmpeg/libavutil/tests/opt.c
        # ./FFmpeg/libavutil/tests/parseutils.c
        # ./FFmpeg/libavutil/tests/pca.c
        # ./FFmpeg/libavutil/tests/pixdesc.c
        # ./FFmpeg/libavutil/tests/pixelutils.c
        # ./FFmpeg/libavutil/tests/pixfmt_best.c
        # ./FFmpeg/libavutil/tests/random_seed.c
        # ./FFmpeg/libavutil/tests/rational.c
        # ./FFmpeg/libavutil/tests/ripemd.c
        # ./FFmpeg/libavutil/tests/sha.c
        # ./FFmpeg/libavutil/tests/sha512.c
        # ./FFmpeg/libavutil/tests/softfloat.c
        # ./FFmpeg/libavutil/tests/tea.c
        # ./FFmpeg/libavutil/tests/tree.c
        # ./FFmpeg/libavutil/tests/twofish.c
        # ./FFmpeg/libavutil/tests/utf8.c
        # ./FFmpeg/libavutil/tests/xtea.c
        ./FFmpeg/libavutil/threadmessage.c
        ./FFmpeg/libavutil/time.c
        ./FFmpeg/libavutil/timecode.c
        ./FFmpeg/libavutil/tree.c
        ./FFmpeg/libavutil/twofish.c
        ./FFmpeg/libavutil/utils.c
        # ./FFmpeg/libavutil/x86/cpu.c
        # ./FFmpeg/libavutil/x86/fixed_dsp_init.c
        # ./FFmpeg/libavutil/x86/float_dsp_init.c
        # ./FFmpeg/libavutil/x86/imgutils_init.c
        # ./FFmpeg/libavutil/x86/lls_init.c
        # ./FFmpeg/libavutil/x86/pixelutils_init.c
        ./FFmpeg/libavutil/xga_font_data.c
        ./FFmpeg/libavutil/xtea.c
        ./FFmpeg/libpostproc/postprocess.c
        # ./FFmpeg/libpostproc/postprocess_altivec_template.c
        # ./FFmpeg/libpostproc/postprocess_template.c
        # ./FFmpeg/libswresample/aarch64/audio_convert_init.c
        # ./FFmpeg/libswresample/aarch64/neontest.c
        # ./FFmpeg/libswresample/aarch64/resample_init.c
        # ./FFmpeg/libswresample/arm/audio_convert_init.c
        # ./FFmpeg/libswresample/arm/neontest.c
        # ./FFmpeg/libswresample/arm/resample_init.c
        ./FFmpeg/libswresample/audioconvert.c
        ./FFmpeg/libswresample/dither.c
        # ./FFmpeg/libswresample/dither_template.c
        # ./FFmpeg/libswresample/log2_tab.c
        # ./FFmpeg/libswresample/noise_shaping_data.c
        ./FFmpeg/libswresample/options.c
        ./FFmpeg/libswresample/rematrix.c
        # ./FFmpeg/libswresample/rematrix_template.c
        ./FFmpeg/libswresample/resample.c
        ./FFmpeg/libswresample/resample_dsp.c
        # ./FFmpeg/libswresample/resample_template.c
        # ./FFmpeg/libswresample/soxr_resample.c
        ./FFmpeg/libswresample/swresample.c
        ./FFmpeg/libswresample/swresample_frame.c
        # ./FFmpeg/libswresample/tests/swresample.c
        # ./FFmpeg/libswresample/x86/audio_convert_init.c
        # ./FFmpeg/libswresample/x86/rematrix_init.c
        # ./FFmpeg/libswresample/x86/resample_init.c
        # ./FFmpeg/libswresample/x86/w64xmmtest.c
        # ./FFmpeg/libswscale/aarch64/swscale.c
        # ./FFmpeg/libswscale/aarch64/swscale_unscaled.c
        ./FFmpeg/libswscale/alphablend.c
        # ./FFmpeg/libswscale/arm/swscale.c
        # ./FFmpeg/libswscale/arm/swscale_unscaled.c
        # ./FFmpeg/libswscale/bayer_template.c
        ./FFmpeg/libswscale/gamma.c
        ./FFmpeg/libswscale/hscale.c
        ./FFmpeg/libswscale/hscale_fast_bilinear.c
        ./FFmpeg/libswscale/input.c
        # ./FFmpeg/libswscale/log2_tab.c
        ./FFmpeg/libswscale/options.c
        ./FFmpeg/libswscale/output.c
        # ./FFmpeg/libswscale/ppc/swscale_altivec.c
        # ./FFmpeg/libswscale/ppc/yuv2rgb_altivec.c
        # ./FFmpeg/libswscale/ppc/yuv2yuv_altivec.c
        ./FFmpeg/libswscale/rgb2rgb.c
        # ./FFmpeg/libswscale/rgb2rgb_template.c
        ./FFmpeg/libswscale/slice.c
        ./FFmpeg/libswscale/swscale.c
        ./FFmpeg/libswscale/swscale_unscaled.c
        # ./FFmpeg/libswscale/tests/colorspace.c
        # ./FFmpeg/libswscale/tests/pixdesc_query.c
        # ./FFmpeg/libswscale/tests/swscale.c
        ./FFmpeg/libswscale/utils.c
        ./FFmpeg/libswscale/vscale.c
        # ./FFmpeg/libswscale/x86/hscale_fast_bilinear_simd.c
        # ./FFmpeg/libswscale/x86/rgb2rgb.c
        # ./FFmpeg/libswscale/x86/rgb2rgb_template.c
        # ./FFmpeg/libswscale/x86/swscale.c
        # ./FFmpeg/libswscale/x86/swscale_template.c
        # ./FFmpeg/libswscale/x86/w64xmmtest.c
        # ./FFmpeg/libswscale/x86/yuv2rgb.c
        # ./FFmpeg/libswscale/x86/yuv2rgb_template.c
        ./FFmpeg/libswscale/yuv2rgb.c
        # ./FFmpeg/tests/api/api-band-test.c
        # ./FFmpeg/tests/api/api-codec-param-test.c
        # ./FFmpeg/tests/api/api-flac-test.c
        # ./FFmpeg/tests/api/api-h264-slice-test.c
        # ./FFmpeg/tests/api/api-h264-test.c
        # ./FFmpeg/tests/api/api-seek-test.c
        # ./FFmpeg/tests/api/api-threadmessage-test.c
        # ./FFmpeg/tests/audiogen.c
        # ./FFmpeg/tests/audiomatch.c
        # ./FFmpeg/tests/base64.c
        # ./FFmpeg/tests/checkasm/aacpsdsp.c
        # ./FFmpeg/tests/checkasm/alacdsp.c
        # ./FFmpeg/tests/checkasm/audiodsp.c
        # ./FFmpeg/tests/checkasm/blockdsp.c
        # ./FFmpeg/tests/checkasm/bswapdsp.c
        # ./FFmpeg/tests/checkasm/checkasm.c
        # ./FFmpeg/tests/checkasm/exrdsp.c
        # ./FFmpeg/tests/checkasm/fixed_dsp.c
        # ./FFmpeg/tests/checkasm/flacdsp.c
        # ./FFmpeg/tests/checkasm/float_dsp.c
        # ./FFmpeg/tests/checkasm/fmtconvert.c
        # ./FFmpeg/tests/checkasm/g722dsp.c
        # ./FFmpeg/tests/checkasm/h264dsp.c
        # ./FFmpeg/tests/checkasm/h264pred.c
        # ./FFmpeg/tests/checkasm/h264qpel.c
        # ./FFmpeg/tests/checkasm/hevc_add_res.c
        # ./FFmpeg/tests/checkasm/hevc_idct.c
        # ./FFmpeg/tests/checkasm/hevc_sao.c
        # ./FFmpeg/tests/checkasm/huffyuvdsp.c
        # ./FFmpeg/tests/checkasm/jpeg2000dsp.c
        # ./FFmpeg/tests/checkasm/llviddsp.c
        # ./FFmpeg/tests/checkasm/llviddspenc.c
        # ./FFmpeg/tests/checkasm/pixblockdsp.c
        # ./FFmpeg/tests/checkasm/sbrdsp.c
        # ./FFmpeg/tests/checkasm/sw_rgb.c
        # ./FFmpeg/tests/checkasm/synth_filter.c
        # ./FFmpeg/tests/checkasm/utvideodsp.c
        # ./FFmpeg/tests/checkasm/v210enc.c
        # ./FFmpeg/tests/checkasm/vf_blend.c
        # ./FFmpeg/tests/checkasm/vf_colorspace.c
        # ./FFmpeg/tests/checkasm/vf_hflip.c
        # ./FFmpeg/tests/checkasm/vf_nlmeans.c
        # ./FFmpeg/tests/checkasm/vf_threshold.c
        # ./FFmpeg/tests/checkasm/videodsp.c
        # ./FFmpeg/tests/checkasm/vp8dsp.c
        # ./FFmpeg/tests/checkasm/vp9dsp.c
        # ./FFmpeg/tests/rotozoom.c
        # ./FFmpeg/tests/tiny_psnr.c
        # ./FFmpeg/tests/tiny_ssim.c
        # ./FFmpeg/tests/utils.c
        # ./FFmpeg/tests/videogen.c
        # ./FFmpeg/tools/aviocat.c
        # ./FFmpeg/tools/coverity.c
        # ./FFmpeg/tools/crypto_bench.c
        # ./FFmpeg/tools/cws2fws.c
        # ./FFmpeg/tools/enum_options.c
        # ./FFmpeg/tools/ffescape.c
        # ./FFmpeg/tools/ffeval.c
        # ./FFmpeg/tools/ffhash.c
        # ./FFmpeg/tools/fourcc2pixfmt.c
        # ./FFmpeg/tools/graph2dot.c
        # ./FFmpeg/tools/ismindex.c
        # ./FFmpeg/tools/pktdumper.c
        # ./FFmpeg/tools/probetest.c
        # ./FFmpeg/tools/qt-faststart.c
        # ./FFmpeg/tools/seek_print.c
        # ./FFmpeg/tools/sidxindex.c
        # ./FFmpeg/tools/sofa2wavs.c
        # ./FFmpeg/tools/target_dec_fuzzer.c
        # ./FFmpeg/tools/trasher.c
        # ./FFmpeg/tools/uncoded_frame.c
        # ./FFmpeg/tools/yuvcmp.c
        # ./FFmpeg/tools/zmqsend.c
)
endif()

file(GLOB
    GIFTOOLS_LIB_SOURCES
    ${PROJECT_SOURCE_DIR}/lib/*.h
    ${PROJECT_SOURCE_DIR}/lib/*.c
    ${PROJECT_SOURCE_DIR}/lib/*.cc
    ${PROJECT_SOURCE_DIR}/lib/*.cpp
)

set(GIFTOOLS_LIB_TEST_SOURCES)
if(NOT ${GIFTOOLS_USE_EMSDK})
    file(GLOB
        GIFTOOLS_LIB_TEST_SOURCES
        ${PROJECT_SOURCE_DIR}/tests/*.cpp
    )
endif()

# message(STATUS "GIFTOOLS_FFMPEG_SOURCES=${GIFTOOLS_FFMPEG_SOURCES}")
# message(STATUS "GIFTOOLS_LIB_SOURCES=${GIFTOOLS_LIB_SOURCES}")

add_executable(
    GifTools
    ${GIFTOOLS_LIB_SOURCES}
    ${GIFTOOLS_FFMPEG_SOURCES}
    ${GIFTOOLS_LIB_TEST_SOURCES}
)


set(GifToolsLibraries)
if(NOT ${GIFTOOLS_USE_EMSDK})
    message(STATUS "Linking with Google Test.")
    target_include_directories(
        GifTools
        PUBLIC
        ${CMAKE_SOURCE_DIR}/dependencies/googletest/googlemock/include
        ${CMAKE_SOURCE_DIR}/dependencies/googletest/googletest/include
        )
        
    add_dependencies(
        GifTools
        googletest
    )

    set(GifToolsLibraries
        debug ${googletest_binary_dir}/lib/Debug/libgmockd.a
        debug ${googletest_binary_dir}/lib/Debug/libgtestd.a
        debug ${googletest_binary_dir}/lib/Debug/libgtest_maind.a
        optimized ${googletest_binary_dir}/lib/Release/libgmock.a
        optimized ${googletest_binary_dir}/lib/Release/libgtest.a
        optimized ${googletest_binary_dir}/lib/Release/libgtest_main.a
        )

    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GIFTOOLS_LIB_TEST_SOURCES})
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GIFTOOLS_LIB_SOURCES})

if (${GIFTOOLS_USE_FFMPEG})
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${GIFTOOLS_FFMPEG_SOURCES})
endif()

target_link_libraries(
    GifTools
    ${GifToolsLibraries}
    pthread
    )
