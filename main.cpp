#include <cstdio>
#include <cstdint>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


std::vector<uint8_t> fileReadFileToBuffer(const char* imgFilePath) {
    if (FILE* imgFileHandle = fopen(imgFilePath, "rb")) {
        fseek(imgFileHandle, 0, SEEK_END);
        size_t imgFileSize = ftell(imgFileHandle);
        fseek(imgFileHandle, 0, SEEK_SET);
        std::vector<uint8_t> imgFileBuffer;
        imgFileBuffer.resize(imgFileSize);
        fread(imgFileBuffer.data(), imgFileSize, 1, imgFileHandle);
        fclose(imgFileHandle);
        return imgFileBuffer;
    }
    
    return {};
}

static int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
    // This function must fill the buffer with data and return number of bytes copied.
    // opaque is the pointer to private_data in the call to av_alloc_put_byte (4th param)
    
    memcpy(buf, opaque, buf_size);
    return buf_size;
}

int main(int argc, char** argv) {
    printf("Yup.");
    
    const char* file = "/Users/vserhiienko/Downloads/2020-02-23 18.53.40.mp4";
    std::vector<uint8_t> buffer = fileReadFileToBuffer(file);
    auto pBuffer = buffer.data();
    auto bufLen = buffer.size();
    
    av_register_all();
    av_log_set_level(AV_LOG_DEBUG);
    
    AVFormatContext *ic;
    ic = avformat_alloc_context();
    ic->pb = avio_alloc_context(pBuffer, bufLen, 0, pBuffer, read_buffer, NULL, NULL);
    if(!ic->pb) {
        return -1;
    }

    // Need to probe buffer for input format unless you already know it
    AVProbeData probe_data;
    probe_data.buf_size = (bufLen < 4096) ? bufLen : 4096;
    probe_data.filename = "stream";
    probe_data.buf = (unsigned char *) malloc(probe_data.buf_size);
    memcpy(probe_data.buf, pBuffer, 4096);
    
    AVInputFormat *pAVInputFormat = av_probe_input_format(&probe_data, 1);
    
    if(!pAVInputFormat)
        pAVInputFormat = av_probe_input_format(&probe_data, 0);
    
    free(probe_data.buf);
    probe_data.buf = NULL;
    
    if(!pAVInputFormat) {
        return -1;
    }

    pAVInputFormat->flags |= AVFMT_NOFILE;
    
    if (avformat_open_input(&ic, "stream", nullptr, nullptr) < 0) {
        return -1;
    }
    
    return 0;
}
