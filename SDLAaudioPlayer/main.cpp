//
//  main.cpp
//  SDLAaudioPlayer
//
//  Created by lugang on 11/27/14.


#include <stdio.h>
#include <SDL2/SDL_test_common.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <SP.h>
#include "AudioPlayer.h"


void  stateChg (EEStat state, void* _pusr)
{
    printf("-----------------%d \n", state);
}
int main(int argc, char **argv)
{
    PlaybackCallback cb = {0, stateChg};
    /* 30 seconds */ Player::instance().start("http://easycast-sf.live365.com/cgi-bin/sample.cgi?length=30&file=Side_Of_The_Moon_That_s_Dark.mp3&sessionid=protest01:VEEY69kFsfqAKU3&device_id=0c:4d:e9:99:cf:7f&app_id=live365:S365BasicOSX&device_type=S365BasicOSX", cb);

    char c;
    while ( (c = getchar()) ) {
            Player::instance().stop();
            Player::instance().start("http://www.live365.com/play/fxradio2?device_type=R365-iPhone2&sessionid=0:0&device_id=7904CCDD-1A49-48CB-8AA6-FE061F283CE7&tag=live365&device_model=Simulator&app_id=live365:R365-iPhone2&ff=30", cb);

    }
    return 0;
}




