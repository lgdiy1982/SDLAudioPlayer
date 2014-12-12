//
//  AudioPlayer.cpp
//  SDLAaudioPlayer
//
//  Created by lugang on 12/1/14.
//  Copyright (c) 2014 live365. All rights reserved.
//

#include "AudioPlayer.h"
#include <IceUtil/Thread.h>
#include <BytesBuffer.h>
#include <SP.h>
#include "Decoder.hpp"
#include "Audio.hpp"





class PlayerImpl
{
public:
    PlayerImpl();
private:
    DecoderPtr _decoder;
    AudioOutputPtr _player;
    BytesBufferPtr _buffer;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PlayerImpl::PlayerImpl()
{
    _buffer = new BytesBuffer(1 << 11);
}