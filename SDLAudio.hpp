#include <SDL2/SDL.h>


static void AudioInputCallback(void *userdata, Uint8 * stream, int len);

struct SdlAudioCallback
{
    void *_pusr;
    void (*drain)(void* _pusr);
};


struct AudioOutput:IceUtil::Shared
{
    BytesBufferPtr _pBuffer;
    BufferChunk _eatChunk;
    
    SDL_AudioSpec _spec;
    int _dev;
    Uint8*  _audioFrameChunk;
    
    SdlAudioCallback _sdlAudioCB;
    AudioOutput(BytesBufferPtr buffer);
    size_t eatCallBackFun(const ChunkInfoRef ref,  bool terminated)
    {
        if (terminated && ref->_data == 0) {
            _sdlAudioCB.drain(_sdlAudioCB._pusr);
            return 0;
        }
        memcpy(_audioFrameChunk, ref->_data, ref->_size);
        return ref->_size;
    }
    

    
    void audioInputCallback(Uint8 * stream, int len)
    {
        _audioFrameChunk = stream;
        _pBuffer->eat(len, &_eatChunk);
    }
    
    void openDevice(SDL_AudioFormat format, size_t channel, size_t freq) {
        
        /* Load the SDL library */
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
            return ;
        }
        
        SDL_zero(_spec);
        _spec.channels = channel;
        _spec.format = AUDIO_S16SYS;
        _spec.freq = (int)freq;
        _spec.samples = 2048;
        _spec.callback = AudioInputCallback;  // you wrote this function elsewhere.
        _spec.userdata = this;
        SDL_AudioSpec obtained;
        _dev = SDL_OpenAudio(&_spec, &obtained);
        if (_dev != 0) {
            SP::printf("cannot open device");
            SDL_CloseAudio();
        }
        
    }
    
    void start()
    {
        if (_dev == 0) {
            SDL_PauseAudio(0);
        }
    }
    
    void close() {
        if (_dev == 0) {
            SDL_PauseAudio(1);
            SDL_CloseAudio();
        }
    }
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

static void AudioInputCallback(void *userdata, Uint8 * stream, int len)
{
    AudioOutput* thiz = (AudioOutput*)userdata;
    thiz->audioInputCallback(stream, len);
}