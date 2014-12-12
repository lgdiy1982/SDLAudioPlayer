//
//  AudioPlayer.cpp
//  SDLAaudioPlayer
//
//  Created by lugang on 12/1/14.

//

#include "AudioPlayer.h"
#include <IceUtil/Thread.h>
#include <BytesBuffer.h>
#include <SP.h>
#include "Decoder.hpp"
#include "SDLAudio.hpp"
#include <vector>
#include <IceUtil/IceUtil.h>
using namespace std;


class PlayerImpl
{
public:
    PlayerImpl();
    ~PlayerImpl();
    void start(const char* url, PlaybackCallback cb);
    void stop();
    
    //decode signals
    void decoderEof();
    void sdlDataDrain();
    void fileOpen(bool , int channel, int freq);
private:
    DecoderPtr _decoder;
    AudioOutputPtr _player;
    BytesBufferPtr _buffer;
    PlaybackCallback _cb;
    IceUtil::Monitor<IceUtil::Mutex> _monitor;
    bool _dry;
};


static void DecoderEofSlot(void *pusr)
{
    PlayerImpl *thiz = (PlayerImpl *)pusr;
    thiz->decoderEof();
}

static void SdlDataDrain(void *pusr)
{
    PlayerImpl *thiz = (PlayerImpl *)pusr;
    thiz->sdlDataDrain();
}

static void FileOpen(bool suc, int channel, int freq, void *pusr)
{
    PlayerImpl *thiz = (PlayerImpl *)pusr;
    thiz->fileOpen(suc, channel, freq);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PlayerImpl::PlayerImpl()
:_dry(false)
{
    Decoder::init();
    _buffer = new BytesBuffer(1 << 20);
    _player = new AudioOutput(_buffer);
}

PlayerImpl::~PlayerImpl()
{
    Decoder::uninit();
}

void PlayerImpl::start(const char* url, PlaybackCallback cb)
{
    _cb = cb;
    _decoder = new Decoder(_buffer);
    _decoder->_signals._pusr = this;
    _decoder->_signals.fileEOF = DecoderEofSlot;
    _decoder->_signals.fileOpen = FileOpen;
    _decoder->openFile(url);
    
    _cb.stateChg(ePlayWaitting , _cb._pusr);
}

void PlayerImpl::decoderEof()
{
    _buffer->terminatedFeed();
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(_monitor);
    while (!_dry) {
        _monitor.wait();
    }
    _player->close();
    _buffer->terminatedEat();
    _cb.stateChg(ePlayFinished , _cb._pusr);
}

void PlayerImpl::fileOpen(bool suc, int channel, int freq)
{
    _cb.stateChg(suc ? ePlayStarted : ePlayErr, _cb._pusr);
    if (!suc) {
        return;
    }
    _player->_sdlAudioCB._pusr = this;
    _player->_sdlAudioCB.drain = SdlDataDrain;
    _player->openDevice(AUDIO_S16SYS, channel, freq);
    _player->start();
}

void PlayerImpl::sdlDataDrain()
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock lock(_monitor);
    _dry = true;
    _monitor.notify();
}

void PlayerImpl::stop()
{
    _decoder->closeFile();
    _buffer->terminatedEat();
    _decoder->getThreadControl().join();
    _player->close();
    _buffer->terminatedFeed();
    
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Player::Player()
{
    
    
    _priv = new PlayerImpl();
}


void Player::start(const char* url, PlaybackCallback cb)
{
    _priv->start(url, cb);
}

void Player::stop()
{
    _priv->stop();
}

Player::~Player(){
    delete _priv;
}

Player& Player::instance()
{
    static Player _player;
    return _player;
}
