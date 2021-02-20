
enum GifToolsMsgType {
    SET_VM,
};

class GifToolsMessagePayload {
    msgType: GifToolsMsgType;
    msgId: number;
};