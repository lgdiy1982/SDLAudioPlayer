//
//  SP.cpp
//
//  Created by lu gang on 8/27/13.
//  Copyright (c) 2013 lu gang. All rights reserved.
//

#include "SP.h"
#include <IceUtil/Handle.h>
#include <IceUtil/Thread.h>
#include <IceUtil/Monitor.h>
#include <list>

#if defined(_WIN32)
typedef DWORD ID;
#else
typedef pthread_t ID;
#endif

inline ID PthreadSelf()
{
#ifdef WIN32
    return ::GetCurrentThreadId();
#else
    return pthread_self();
#endif
}

inline void threadName(char* name, size_t len)
{
#ifdef WIN32
    
#else
    if ( pthread_getname_np( pthread_self(), name, len) != 0) {
        
    }
#endif
}
using namespace IceUtil;
using namespace std;


typedef Handle<class SP_context>  SP_context_Ptr;

class SP_context: public IceUtil::Thread
{
public:
    //static SP_context& instance();
    SP_context();
    ~SP_context();
    void printf(const char* format, ...);
    void printf(const char* format, va_list vl);
    void destroy();
    void waitforFinished();
    void setOutput(OutType type);
    void run();
private:
    list<string> _l;
    static Monitor<Mutex> _monitor;
    OutType _type;
    string filePath;
    char _buf[1<<23];
    bool _destroy;
    bool _waitforFinished;
    
};

Monitor<Mutex> SP_context::_monitor;

SP_context::SP_context()
:_destroy(false)
,_waitforFinished(false)
,_type(eStd)
{
    
}


SP_context::~SP_context()
{
    
}

void SP_context::setOutput(OutType type)
{
    _type = type;
}

//void SP_context::printf(const char* format, ...)
//{
//	char buf[2<<8];
//	memset(buf, 0, sizeof(buf));
//    sprintf(buf, "ThreadID:[ %lld ], timestamp:[%s] ", (Int64)PthreadSelf(), IceUtil::Time::now().toDateTime().c_str()  );
//    
//    va_list vl;
//    va_start(vl, format);
//    vsnprintf(buf + strlen(buf), sizeof(buf), format, vl);
//    va_end(vl);
//    Monitor<Mutex>::Lock lock(_monitor);
//    if (_l.empty()) {
//        _monitor.notify();
//    }
//    _l.push_front(string(buf));
//}

void SP_context::printf(const char* format, va_list vl)
{
    Monitor<Mutex>::Lock lock(_monitor);
    //sprintf(_buf, "%s[%llx] ",  IceUtil::Time::now().toDateTime().c_str(), (int64_t)PthreadSelf() );
    
    static char name[32] = {0};
    threadName(name, 32);
    sprintf(_buf, "%s[%s] ",  IceUtil::Time::now().toDateTime().c_str(), name);
    vsnprintf(_buf + strlen(_buf), sizeof(_buf) - strlen(_buf), format, vl);
    if (_l.empty()) _monitor.notify();
    _l.push_front(string(_buf));
}

void SP_context::run()
{
//    string log_path = string_tool::wstring_to_utf8(filesystem_tool::get_live365_datapath(""));
//    log_path += "/studio365-basic.log";
//    
//    ofstream loogerFile(log_path.c_str());
    while (true) {
        {
            Monitor<Mutex>::Lock lock(_monitor);
            while (_l.empty() && !_destroy && !_waitforFinished) {
                _monitor.wait();
            }
        }
        
        if(_destroy)
            break;
        {
            Monitor<Mutex>::Lock lock(_monitor);
            if (_l.empty() && _waitforFinished)
                break;
        }
        
        string msg;//
        {
            Monitor<Mutex>::Lock lock(_monitor);
            msg = _l.back();
            _l.pop_back();
        }
        switch (_type) {
            case eStd:
                cout << msg ;
                break;
            case eErr:
                cerr << msg;
                break;
            case eFile:
                break;
            default:
                break;
        }
        //loogerFile << msg;
        
    }
    //loogerFile.close();
}

void SP_context::destroy()
{
    Monitor<Mutex>::Lock lock(_monitor);
    _destroy = true;
    if (_l.empty()) _monitor.notify();
}

void SP_context::waitforFinished()
{
    {
        Monitor<Mutex>::Lock lock(_monitor);
        _waitforFinished = true;
        _monitor.notify();
    }
    getThreadControl().join();
}

static SP_context_Ptr gContext = NULL ;
SP globalSP;
//------------------------------------------------------------------------------------------------------------------------------------------------------------

SP::SP() {
    if (!gContext) {
        gContext = new SP_context();
        gContext->start();
    }
}

SP::~SP()
{
    SP::printf("\n\n\n!!!!! This is the last message !!!! \n\n\n");
    gContext->waitforFinished();
}

void SP::printf(const char* format, ...)
{
//#ifdef DEBUG
    va_list vl;
    va_start(vl, format);
    gContext->printf(format, vl);
    va_end(vl);
//#endif
}

void SP::setOutput(OutType type)
{
//#ifdef DEBUG
    gContext->setOutput(type);
//#endif
}


void SP::quit()
{
    gContext->waitforFinished();
}

