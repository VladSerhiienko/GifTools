
cmake -Bbuild_xcode -H. -G Xcode
xcodebuild -project ./build_xcode/GifTools.xcodeproj -configuration Release
