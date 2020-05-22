
cmake -Bbuild_xcode -H. -G Xcode -DGIFTOOLS_BUILD_FFMPEG=TRUE
xcodebuild -project build_xcode/GifTools.xcodeproj -configuration Release
