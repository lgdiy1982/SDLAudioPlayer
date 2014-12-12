//
//  AudioPlayer.h
//  SDLAaudioPlayer
//
//  Created by lugang on 12/1/14.
//  Copyright (c) 2014 live365. All rights reserved.
//

#ifndef __SDLAaudioPlayer__AudioPlayer__
#define __SDLAaudioPlayer__AudioPlayer__

#include <stdio.h>

enum EEStat
{
    
};

struct playbackCallback
{
    void* _pusr;
    void (*progressCallback) (size_t expired, size_t duration);
    void (*stateChg) (EEStat);
};

class Player
{
public:
    Player(const char* url);
    void start();
    void stop();
private:
    class PlayerImpl *_priv;
};
#endif /* defined(__SDLAaudioPlayer__AudioPlayer__) */
