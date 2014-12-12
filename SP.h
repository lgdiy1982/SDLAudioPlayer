//
//  SP.h
//
//  Created by lu gang on 8/27/13.
//

#ifndef __AmrRecoderAndPlayer__SP__
#define __AmrRecoderAndPlayer__SP__



enum OutType
{
    eStd,
    eErr,
    eFile
};



class SP
{
public:
    SP();
    ~SP();
public:
    void init();
    void setOutput(OutType);
    static void printf(const char* format, ...);
    static void quit();
};

extern SP globalSP;
#endif /* defined(__AmrRecoderAndPlayer__SP__) */
