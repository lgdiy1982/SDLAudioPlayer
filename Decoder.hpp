#include <libavutil/opt.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libswresample/swresample.h>



struct Decoder : public IceUtil::Thread
{
    Decoder(BytesBufferPtr buffer);
    void run();

    size_t feedCallBackFun(const ChunkInfoRef chunkinfo,  bool terminated);
    BytesBufferPtr _pBuffer;
    BufferChunk _feedChunk;
};
typedef IceUtil::Handle< Decoder> DecoderPtr;

static size_t FeedCallBackFun(void* userData, const ChunkInfoRef chunkinfo,  bool terminated)
{
    Decoder* thiz = (Decoder*) userData;
    return thiz->feedCallBackFun(chunkinfo, terminated);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Decoder::Decoder(BytesBufferPtr buffer)
:_pBuffer(buffer)
{
    _feedChunk._userData = this;
    _feedChunk._callback = FeedCallBackFun;
}

size_t Decoder::feedCallBackFun(const ChunkInfoRef chunkinfo,  bool terminated)
{
    return 0;
}

void Decoder::run()
{
    
}

