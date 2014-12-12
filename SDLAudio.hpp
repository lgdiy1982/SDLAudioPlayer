#include <SDL2/SDL.h>

struct AudioOutput : public IceUtil::Thread
{
    AudioOutput(BytesBufferPtr buffer);
    void run();
    size_t eatCallBackFun(const ChunkInfoRef,  bool terminated);
    
    
    
    BytesBufferPtr _pBuffer;
    BufferChunk _eatChunk;
};
typedef IceUtil::Handle< AudioOutput > AudioOutputPtr;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static size_t EatCallBackFun(void* userData, const ChunkInfoRef info,  bool terminated)
{
    AudioOutput* thiz = (AudioOutput*)userData;
    return thiz->eatCallBackFun(info, terminated);
}


AudioOutput::AudioOutput(BytesBufferPtr buffer)
:_pBuffer(buffer)
{
    _eatChunk._userData = this;
    _eatChunk._callback = EatCallBackFun;
}

size_t AudioOutput::eatCallBackFun(const ChunkInfoRef,  bool terminated)
{
    return 0;
}

void AudioOutput::run()
{
    //wait for specified amount of bytes
    
}