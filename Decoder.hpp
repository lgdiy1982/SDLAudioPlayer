extern "C"
{
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}


typedef enum {
    kxMovieErrorNone,
    kxMovieErrorOpenFile,
    kxMovieErrorStreamInfoNotFound,
    kxMovieErrorStreamNotFound,
    kxMovieErrorCodecNotFound,
    kxMovieErrorOpenCodec,
    kxMovieErrorAllocateFrame,
    kxMovieErroSetupScaler,
    kxMovieErroReSampler,
    kxMovieErroUnsupported,
    
} kxMovieError;


static size_t FeedCallBackFun(void* userData, const ChunkInfoRef chunkinfo,  bool terminated);

//static int interruptCallback(void *)
//{
//    SP::printf("interruptCallback!!!!!\n");
//    return 0;
//}

struct DecoderSignals
{
    void* _pusr;
    void (*fileEOF)(void *pusr);
    void (*fileOpen)(bool , int channel, int freq, void *pusr);
};

static const char * errorMessage (kxMovieError errorCode)
{
    switch (errorCode) {
        case kxMovieErrorNone:
            return "";
            
        case kxMovieErrorOpenFile:
            return "Unable to open file";
            
        case kxMovieErrorStreamInfoNotFound:
            return "Unable to find stream information";
            
        case kxMovieErrorStreamNotFound:
            return "Unable to find stream";
            
        case kxMovieErrorCodecNotFound:
            return "Unable to find codec";
            
        case kxMovieErrorOpenCodec:
            return "Unable to open codec";
            
        case kxMovieErrorAllocateFrame:
            return "Unable to allocate frame";
            
        case kxMovieErroSetupScaler:
            return "Unable to setup scaler";
            
        case kxMovieErroReSampler:
            return "Unable to setup resampler";
            
        case kxMovieErroUnsupported:
            return "The ability is not supported";
    }
}


#include <SDL2/SDL_audio.h>
struct Decoder : public IceUtil::Thread
{
    BytesBufferPtr _pBuffer;
    BufferChunk _feedChunk;

    
    AVFormatContext     *_formatCtx;
    AVCodecContext      *_audioCodecCtx;
    AVFrame             *_audioFrame;
    SwrContext          *_swrContext;
    AVPacket             _packet;
    size_t              _audioStream;
    std::string              _file;
    float               _audioTimeBase;
    
    bool                _isEOF;
    //SDL_AudioFormat     _format;

    int (*_interruptCallback) (void*);
    
    DecoderSignals _signals;
    IceUtil::Mutex _mutex;
    bool _stopFlag;
    
    Decoder(BytesBufferPtr buffer)
    :_pBuffer(buffer), _stopFlag(false)
    {
        _feedChunk._userData = this;
        _feedChunk._callback = FeedCallBackFun;
        
        _formatCtx = NULL;
        _audioCodecCtx = NULL;
        _audioFrame = NULL;
        _audioStream = -1;
        _swrContext = NULL;
        _isEOF = false;
        _interruptCallback = NULL;
        //_interruptCallback = interruptCallback;
    }
    
    
    
    size_t feedCallBackFun(const ChunkInfoRef chunkinfo,  bool terminated)
    {
        if (terminated) {
            return 0;
        }
        if (_swrContext ) {
            uint8_t *outbuf[2] = { (uint8_t*)chunkinfo->_data, 0 };
            
            int numFrames = swr_convert(_swrContext,
                                        outbuf,
                                        (int) (_audioFrame->nb_samples),
                                        (const uint8_t **)_audioFrame->data,
                                        _audioFrame->nb_samples);
            
            
            const int bufSize = av_samples_get_buffer_size(NULL,
                                                           _audioCodecCtx->channels,
                                                           numFrames,
                                                           AV_SAMPLE_FMT_S16,
                                                           1);
            
            return bufSize;
        } else {
            memcpy(chunkinfo->_data, _audioFrame->data[0], chunkinfo->_size);
            return chunkinfo->_size;
        }
    }

    void onSDLAudioStop()
    {
        
    }
    
    void openFile(const char* filepath)
    {
        _file = filepath;
        start();
    }

    void closeFile()
    {
        IceUtil::Mutex::Lock lock(_mutex);
        _stopFlag = true;
    }
    
    kxMovieError openAudioStream()
    {
        kxMovieError errCode = kxMovieErrorStreamNotFound;
        _audioStream = -1;
        for (size_t i = 0; i < _formatCtx->nb_streams; ++i ) {
            if (AVMEDIA_TYPE_AUDIO == _formatCtx->streams[i]->codec->codec_type) {
                errCode = openAudioStream(i);
                if (errCode == kxMovieErrorNone)
                    break;
                else {
                    if (kxMovieErrorNone != errCode) {
                        SP::printf("%s", errorMessage(errCode));
                    }
                }
            }
        }
        return errCode;
    }
    
    bool audioCodecIsSupported(AVCodecContext *audio)
    {
        return audio->sample_fmt == AV_SAMPLE_FMT_S16;
    }
    
    kxMovieError openAudioStream(size_t audioStream)
    {
        AVCodecContext *codecCtx = _formatCtx->streams[audioStream]->codec;
        SwrContext *swrContext = NULL;
        
        AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
        if(!codec)
            return kxMovieErrorCodecNotFound;
        
        if (avcodec_open2(codecCtx, codec, NULL) < 0)
            return kxMovieErrorOpenCodec;
        

        
        if (!audioCodecIsSupported(codecCtx))
        {
            swrContext = swr_alloc_set_opts(NULL,
                                            av_get_default_channel_layout(codecCtx->channels),
                                            AV_SAMPLE_FMT_S16,
                                            codecCtx->sample_rate,
                                            av_get_default_channel_layout(codecCtx->channels),
                                            codecCtx->sample_fmt,
                                            codecCtx->sample_rate,
                                            0,
                                            NULL);
            
            if (!swrContext ||
                swr_init(swrContext)) {
                
                if (swrContext)
                    swr_free(&swrContext);
                avcodec_close(codecCtx);
                return kxMovieErroReSampler;
            }
        }
        
        _audioFrame = av_frame_alloc();
        
        if (!_audioFrame) {
            if (swrContext)
                swr_free(&swrContext);
            avcodec_close(codecCtx);
            return kxMovieErrorAllocateFrame;
        }
        
        _audioStream = audioStream;
        _audioCodecCtx = codecCtx;
        _swrContext = swrContext;
        return kxMovieErrorNone; 
    }
    
    void handleAudioFrame()
    {
        //SP::printf("handleAudioFrame  \n");
        if (!_audioFrame->data[0])
            return ;
        
        const int bufSize = av_samples_get_buffer_size(NULL,
                                                       _audioCodecCtx->channels,
                                                       (int)(_audioFrame->nb_samples),
                                                       AV_SAMPLE_FMT_S16,
                                                       1);
        _pBuffer->feed(bufSize, &_feedChunk);
    }
    
    void decodeFrames(float)
    {
        if ( _audioStream == -1) {
            return ;
        }
        
        bool finished = false;
        
        while (!finished) {
            
            if (av_read_frame(_formatCtx, &_packet) < 0) {
                _isEOF = true;
                break;
            }
            
            if (_packet.stream_index == _audioStream) {
                int pktSize = _packet.size;
                while (pktSize > 0) {
                    int gotframe = 0;
                    int len = avcodec_decode_audio4(_audioCodecCtx,
                                                    _audioFrame,
                                                    &gotframe,
                                                    &_packet);
                    if (len < 0) {
                        break;
                    }
                    if (gotframe) {
                        handleAudioFrame();
                        finished = true;
                    }
                    
                    if (0 == len)
                        break;
                    
                    pktSize -= len;
                }
            }
        }
    }
    
    void closeAudioStream()
    {
        _audioStream = -1;
        
        if (_swrContext) {
            
            swr_free(&_swrContext);
            _swrContext = NULL;
        }
        
        if (_audioFrame) {
            
            av_free(_audioFrame);
            _audioFrame = NULL;
        }
        
        if (_audioCodecCtx) {
            
            avcodec_close(_audioCodecCtx);
            _audioCodecCtx = NULL;
        }
        
        avformat_close_input(&_formatCtx);
        
       
        //av_register_all();
    }
    
    kxMovieError openInput(const char* file)
    {
        AVFormatContext *formatCtx = NULL;
        if (_interruptCallback) {
            formatCtx = avformat_alloc_context();
            if (!formatCtx)
                return kxMovieErrorOpenFile;
            
            AVIOInterruptCB cb = {_interruptCallback, this};
            formatCtx->interrupt_callback = cb;
        }
        
        if (avformat_open_input(&formatCtx, file, NULL, NULL) < 0) {
            if (formatCtx)
                avformat_free_context(formatCtx);
            return kxMovieErrorOpenFile;
        }
        
        if (avformat_find_stream_info(formatCtx, NULL) < 0) {
            avformat_close_input(&formatCtx);
            return kxMovieErrorStreamInfoNotFound;
        }
        
        av_dump_format(formatCtx, 0, file, false);
        _formatCtx = formatCtx;
        return kxMovieErrorNone;
    }
    
    static void init()
    {
        avformat_network_init();
        av_register_all();
    }
    
    static void uninit()
    {
         avformat_network_deinit();
    }
    
    void run()
    {
        do
        {
            kxMovieError errCode = openInput(_file.c_str());
            if (errCode == kxMovieErrorNone) {
                errCode = openAudioStream();
            }
            if (errCode == kxMovieErrorNone) {
                _signals.fileOpen(errCode == kxMovieErrorNone, _audioCodecCtx->channels, _audioCodecCtx->sample_rate, _signals._pusr);
            } else {
                _signals.fileOpen(errCode == kxMovieErrorNone, 0, 0, _signals._pusr);
                break;
            }
            
            
            do {
                decodeFrames(0.1);
                IceUtil::Mutex::Lock lock(_mutex);
                if (_stopFlag || _isEOF) {
                    break;
                }
            } while (true);
            
            if (_isEOF) {
                _signals.fileEOF(_signals._pusr);
                //SP::printf("download finished!!!!!!! \n");
            } else {
                //SP::printf("download aborted !!!!!!! \n");
            }
            
        } while (false) ;
        
        closeAudioStream();
    }
};
typedef IceUtil::Handle< Decoder> DecoderPtr;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



size_t FeedCallBackFun(void* userData, const ChunkInfoRef chunkinfo,  bool terminated)
{
    Decoder* thiz = (Decoder*) userData;
    return thiz->feedCallBackFun(chunkinfo, terminated);
}