#include <GifTools.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

#ifndef GIFTOOLS_EMSCRIPTEN
namespace {

struct GifToolsGifParams {
    size_t width = 0;
    size_t height = 0;
    std::string targetResolutionId = "";
};

struct GifToolsFFmpegParams : GifToolsGifParams {
    std::string videoFilePath = "";
    std::string videoResolutionId = "";
    double stepSeconds = 1.0;
};

struct GifToolsTestBase : public testing::Test {
    static constexpr std::string_view testsBinRelativePath = "./../../tests/bin";
    static constexpr std::string_view resultsPath = "results_actual_cpp";
    static constexpr std::string_view expectedResultsPath = "results_expected";
    static constexpr std::string_view imagesPath = "image";
    static constexpr std::string_view videosPath = "video";
    
    const std::string expectedResultsRelativePath = std::string(testsBinRelativePath) + std::string("/") + std::string(expectedResultsPath);
    const std::string resultsRelativePath = std::string(testsBinRelativePath) + std::string("/") + std::string(resultsPath);
    const std::string imagesRelativePath = std::string(testsBinRelativePath) + std::string("/") + std::string(imagesPath);
    const std::string videosRelativePath = std::string(testsBinRelativePath) + std::string("/") + std::string(videosPath);
    
    std::string expectedResultFilePath(std::string_view fileName) { return expectedResultsRelativePath + std::string("/") + std::string(fileName); }
    std::string resultFilePath(std::string_view fileName) { return resultsRelativePath + std::string("/") + std::string(fileName); }
    std::string imageFilePath(std::string_view fileName) { return imagesRelativePath + std::string("/") + std::string(fileName); }
    std::string videoFilePath(std::string_view fileName) { return videosRelativePath + std::string("/") + std::string(fileName); }
    
    void SetUp() override {
        std::filesystem::create_directory(resultsRelativePath);
        std::filesystem::create_directory(expectedResultsRelativePath);
    }
    
    void TearDown() override {}
    
    void MatchOrWriteExpected(const giftools::Buffer* buffer, const std::string& actualFileName) {
        auto expectedFilePath = expectedResultFilePath(actualFileName);
        if (!std::filesystem::exists(expectedFilePath)) {
            fileBinaryWrite(expectedFilePath.c_str(), buffer);
        } else {
            auto expectedBuffer = giftools::fileBinaryRead(expectedFilePath.c_str());
            ASSERT_EQ(expectedBuffer->size(), buffer->size());
            ASSERT_EQ(0, memcmp(expectedBuffer->data(),  buffer->data(),  buffer->size()));
        }
    }
};

struct GifToolsGifTest : public GifToolsTestBase, public testing::WithParamInterface<GifToolsGifParams> {};
struct GifToolsFFmpegTest : public GifToolsTestBase, public testing::WithParamInterface<GifToolsFFmpegParams> {};

TEST_P(GifToolsGifTest, GifToolsGifEncodingTest) {
    using namespace giftools;
    constexpr size_t delay = 30;
    
    GifToolsGifParams params = GetParam();
    auto width = params.width;
    auto height = params.height;
    ASSERT_FALSE(params.targetResolutionId.empty());
    ASSERT_TRUE(width);
    ASSERT_TRUE(height);

    UniqueManagedObj<Buffer> imgBufferObjs[] = {
        fileBinaryRead(imageFilePath("IMG_20191217_083053.jpg").c_str()),
        fileBinaryRead(imageFilePath("IMG_20191217_083055.jpg").c_str()),
        fileBinaryRead(imageFilePath("IMG_20191217_083056.jpg").c_str()),
        fileBinaryRead(imageFilePath("IMG_20191217_083058.jpg").c_str()),
        fileBinaryRead(imageFilePath("IMG_20191217_083059.jpg").c_str()),
        fileBinaryRead(imageFilePath("IMG_20191217_083101.jpg").c_str())
    };
    
    ASSERT_TRUE(imgBufferObjs[0]);
    ASSERT_TRUE(imgBufferObjs[1]);
    ASSERT_TRUE(imgBufferObjs[2]);
    ASSERT_TRUE(imgBufferObjs[3]);
    ASSERT_TRUE(imgBufferObjs[4]);
    ASSERT_TRUE(imgBufferObjs[5]);

    int imgIndices[] = {0, 1, 2, 3, 4, 5, 4, 3, 2, 1};

    UniqueManagedObj<Image> imageObjs[] = {
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[0]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[1]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[2]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[3]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[4]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[5]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[6]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[7]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[8]].get()),
        imageLoadFromFileBuffer(imgBufferObjs[imgIndices[9]].get()),
    };
    
    ASSERT_TRUE(imageObjs[0]);
    ASSERT_TRUE(imageObjs[1]);
    ASSERT_TRUE(imageObjs[2]);
    ASSERT_TRUE(imageObjs[3]);
    ASSERT_TRUE(imageObjs[4]);
    ASSERT_TRUE(imageObjs[5]);
    ASSERT_TRUE(imageObjs[6]);
    ASSERT_TRUE(imageObjs[7]);
    ASSERT_TRUE(imageObjs[8]);
    ASSERT_TRUE(imageObjs[9]);

    imageObjs[0] = imageResizeOrClone(imageObjs[0].get(), width, height);
    imageObjs[1] = imageResizeOrClone(imageObjs[1].get(), width, height);
    imageObjs[2] = imageResizeOrClone(imageObjs[2].get(), width, height);
    imageObjs[3] = imageResizeOrClone(imageObjs[3].get(), width, height);
    imageObjs[4] = imageResizeOrClone(imageObjs[4].get(), width, height);
    imageObjs[5] = imageResizeOrClone(imageObjs[5].get(), width, height);
    imageObjs[6] = imageResizeOrClone(imageObjs[6].get(), width, height);
    imageObjs[7] = imageResizeOrClone(imageObjs[7].get(), width, height);
    imageObjs[8] = imageResizeOrClone(imageObjs[8].get(), width, height);
    imageObjs[9] = imageResizeOrClone(imageObjs[9].get(), width, height);
    
    ASSERT_TRUE(imageObjs[0]);
    ASSERT_TRUE(imageObjs[1]);
    ASSERT_TRUE(imageObjs[2]);
    ASSERT_TRUE(imageObjs[3]);
    ASSERT_TRUE(imageObjs[4]);
    ASSERT_TRUE(imageObjs[5]);
    ASSERT_TRUE(imageObjs[6]);
    ASSERT_TRUE(imageObjs[7]);
    ASSERT_TRUE(imageObjs[8]);
    ASSERT_TRUE(imageObjs[9]);

    UniqueManagedObj<GifBuilder> gifBuilderObj = gifBuilderInitialize(width, height, delay);
    ASSERT_TRUE(gifBuilderObj);
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[0].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[1].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[2].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[3].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[4].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[5].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[6].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[7].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[8].get(), delay));
    ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), imageObjs[9].get(), delay));

    UniqueManagedObj<Buffer> gifBufferObj = gifBuilderFinalize(gifBuilderObj.get());
    ASSERT_TRUE(gifBufferObj);
    
    auto actualFileName = std::string("dump_") + params.targetResolutionId + ".gif";
    
    fileBinaryWrite(resultFilePath(actualFileName).c_str(), gifBufferObj.get());
    MatchOrWriteExpected(gifBufferObj.get(), actualFileName);
}

TEST_P(GifToolsFFmpegTest, GifToolsFFmpegEncodingTest) {
    using namespace giftools;
    constexpr size_t delay = 30;
    
    GifToolsFFmpegParams params = GetParam();
    ASSERT_FALSE(params.targetResolutionId.empty());
    ASSERT_FALSE(params.videoResolutionId.empty());
    ASSERT_FALSE(params.videoFilePath.empty());
    
    UniqueManagedObj<Buffer> videoFileBufferObj = fileBinaryRead(videoFilePath(params.videoFilePath).c_str());
    ASSERT_TRUE(videoFileBufferObj);
    
    UniqueManagedObj<FFmpegInputStream> input = ffmpegInputStreamLoadFromBuffer(videoFileBufferObj.get());
    ASSERT_TRUE(input);
    
    UniqueManagedObj<FFmpegVideoStream> video = ffmpegVideoStreamOpen(input.get());
    ASSERT_TRUE(video);
    ASSERT_TRUE(video->estimatedTotalDurationSeconds() > 0);
    ASSERT_TRUE(video->estimatedFrameDurationSeconds() > 0);
    
    auto width = params.width ? params.width : video->frameWidth();
    auto height = params.height ? params.height : video->frameHeight();
    
    ASSERT_TRUE(width);
    ASSERT_TRUE(height);
    
    std::vector<UniqueManagedObj<FFmpegVideoFrame>> frames = {};
    std::vector<UniqueManagedObj<Image>> resizedImages = {};
    std::vector<const Image*> images = {};
    
    for (double t = 0.0; t < video->estimatedTotalDurationSeconds(); t += params.stepSeconds) {
        frames.emplace_back(ffmpegVideoStreamPickBestFrame(video.get(), t));
        ASSERT_TRUE(frames.back()->image());
        
        if (frames.back()->image()->width() != width || frames.back()->image()->height() != height) {
            resizedImages.emplace_back(imageResizeOrClone(frames.back()->image(), width, height));
            images.push_back(resizedImages.back().get());
            ASSERT_TRUE(resizedImages.back());
        } else {
            images.push_back(frames.back()->image());
        }
        
        printf("Progress: %.1f%%\n", 50.0 * (t / video->estimatedTotalDurationSeconds()));
    }
    
    UniqueManagedObj<GifBuilder> gifBuilderObj = gifBuilderInitialize(width, height, delay);
    ASSERT_TRUE(gifBuilderObj);
    
    int i = 0;
    for (auto& image : images) {
        ASSERT_TRUE(gifBuilderAddImage(gifBuilderObj.get(), image, delay));
        printf("Progress: %.1f%%\n", 50.0 + (50.0 * (double(i++) / images.size())));
    }

    UniqueManagedObj<Buffer> gifBufferObj = gifBuilderFinalize(gifBuilderObj.get());
    ASSERT_TRUE(gifBufferObj);
    
    auto prefix = std::string("dump_");
    auto fileName = std::filesystem::path(params.videoFilePath).stem().string();
    auto id = params.videoResolutionId + "_" + params.targetResolutionId;
    auto actualFileName = prefix + "_" + fileName + "_" + id + ".gif";
    
    fileBinaryWrite(resultFilePath(actualFileName).c_str(), gifBufferObj.get());
    
    MatchOrWriteExpected(gifBufferObj.get(), actualFileName);
}

#define GIFTOOLS_TEST_ALL 0

INSTANTIATE_TEST_SUITE_P(
    resolutions,
    GifToolsGifTest,
    testing::Values(
        GifToolsGifParams{640, 360, "360p"}
        
        #if GIFTOOLS_TEST_ALL
        , GifToolsGifParams{1280, 720, "720p"}
        , GifToolsGifParams{1920, 1080, "fhd"}
        , GifToolsGifParams{4608, 3456, "uhd"}
        #endif
    ));

INSTANTIATE_TEST_SUITE_P(
    resolutions,
    GifToolsFFmpegTest,
    testing::Values(
        GifToolsFFmpegParams{{0, 0, "default"}, "VID_20200503_154756_360P.mp4", "360p"}
        
        #if GIFTOOLS_TEST_ALL
        , GifToolsFFmpegParams{{0, 0, "default"}, "VID_20200521_193627_FHD.mp4", "fhd"}
        , GifToolsFFmpegParams{{0, 0, "default"}, "VID_20200521_193627_UHD.mp4", "uhd"}
        , GifToolsFFmpegParams{{640, 360, "360p"}, "VID_20200503_154756_360P.mp4", "360p"}
        , GifToolsFFmpegParams{{640, 360, "360p"}, "VID_20200521_193627_FHD.mp4", "fhd"}
        , GifToolsFFmpegParams{{640, 360, "360p"}, "VID_20200521_193627_UHD.mp4", "uhd"}
        #endif
        
        , GifToolsFFmpegParams{{1280, 720, "720p"}, "VID_20200503_154756_360P.mp4", "360p"}
        
        #if GIFTOOLS_TEST_ALL
        , GifToolsFFmpegParams{{1280, 720, "720p"}, "VID_20200521_193627_FHD.mp4", "fhd"}
        , GifToolsFFmpegParams{{1280, 720, "720p"}, "VID_20200521_193627_UHD.mp4", "uhd"}
        , GifToolsFFmpegParams{{4608, 3456, "4k"}, "VID_20200503_154756_360P.mp4", "360p"}
        , GifToolsFFmpegParams{{4608, 3456, "4k"}, "VID_20200521_193627_FHD.mp4", "fhd"}
        , GifToolsFFmpegParams{{4608, 3456, "4k"}, "VID_20200521_193627_UHD.mp4", "uhd"}
        #endif
    ));
}
#endif
