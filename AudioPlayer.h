//
//  AudioPlayer.h
//  SDLAaudioPlayer
//
//  Created by lugang on 12/1/14.

//

#ifndef __SDLAaudioPlayer__AudioPlayer__
#define __SDLAaudioPlayer__AudioPlayer__

#include <stdio.h>

enum EEStat
{
    eIdel,
    ePlayErr,
    ePlayWaitting,
    ePlayStarted,
    ePlayAborted,
    ePlayFinished
};

struct PlaybackCallback
{
    void* _pusr;
    void (*stateChg) (EEStat state, void* _pusr);
};

class Player
{
public:
    //
    static Player& instance();
    void start(const char* url, PlaybackCallback cb);
    void stop();

private:
    Player();
    ~Player();
    class PlayerImpl *_priv;
};
#endif /* defined(__SDLAaudioPlayer__AudioPlayer__) */
