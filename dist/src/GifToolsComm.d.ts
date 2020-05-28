declare enum GifToolsMsgType {
    SET_VM = 0
}
declare class GifToolsMessagePayload {
    msgType: GifToolsMsgType;
    msgId: number;
}
