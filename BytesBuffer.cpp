//
//  BinaryCircleBuffer.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.

//

#include "BytesBuffer.h"
#include <IceUtil/IceUtil.h>
#include <SP.h>
#include <HexDump.h>
using namespace IceUtil;

#define WAITINGBUGTIME 1000
class BytesBuffer_context
{
public:
    BytesBuffer_context(size_t bufferSize);
    ~BytesBuffer_context();
    friend class BytesBuffer;
    
    
    void feed(size_t size, BufferChunkRef cbChunk);
    void eat(size_t size, BufferChunkRef cbChunk);
    void clean();
    void terminateFeed();
    void terminateEat();
    bool empty();
private:
    const  size_t _totalBufferSize;
    size_t _feedBeginIndex;
    size_t _feedCapacity;
    
    size_t _eatBeginIndex;
    size_t _eatCapacity;
    Monitor<RecMutex> _monitor;
    unsigned char* _buffer;
    unsigned char* _eatSwapBuffer;
    unsigned char* _feedSwapBuffer;
    bool _feedTerminated;
    bool _eatTerminated;
    size_t _currentFeedRequestSize;
    size_t _currentEatRequestSize;
    
};

BytesBuffer_context::BytesBuffer_context(size_t bufferSize) :
_totalBufferSize(bufferSize),
_feedBeginIndex(0),
_feedCapacity(bufferSize),
_eatBeginIndex(0),
_eatCapacity(0),
_currentFeedRequestSize(0),
_currentEatRequestSize(0),
_feedTerminated(false),
_eatTerminated(false)
{
    _buffer = (unsigned char*)malloc(bufferSize);
    _eatSwapBuffer = (unsigned char*)malloc(bufferSize);
    _feedSwapBuffer = (unsigned char*)malloc(bufferSize);
    bzero(_buffer, bufferSize);
}

void BytesBuffer_context::feed(size_t size, BufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        if (_feedTerminated) return;
        
        while( size > _feedCapacity && !_eatTerminated) {
            _currentFeedRequestSize = size;
            
            if ( !_monitor.timedWait(IceUtil::Time::milliSeconds(WAITINGBUGTIME)) ) {
                SP::printf("\nwait feeding %d \n", _currentFeedRequestSize);
            }
        }
        _currentFeedRequestSize = 0;
    }
    
    //if _eatTerminated is true; size > _feedCapacity
    size = size > _feedCapacity ? _feedCapacity : size;
    
    bool truncated = false;
    size_t truncatedSize = 0;
    
    //at this moment , _feedCapacity maybe increased, but do not disturb the result;
    if (_totalBufferSize < _feedBeginIndex + size) {
        truncated = true;
        truncatedSize = _totalBufferSize - _feedBeginIndex;
    }
    
    cbChunk->_size = size;
    if (truncated)
        cbChunk->_data = _feedSwapBuffer;
    else
        cbChunk->_data = _buffer + _feedBeginIndex;
    
    //if (truncated)  SP::printf("\n>>>>>>>>>>>>>>>>>>>>feed truncated (start: %u, size: %u, t: %u \n", _feedBeginIndex, size, truncatedSize);
    
    
    //shit out the continious buffer
    size_t realSize = cbChunk->_callback(cbChunk->_userData, cbChunk, _eatTerminated);
    
    if (truncated) {
        if (realSize <= truncated) {
            memcpy(_buffer+_feedBeginIndex, cbChunk->_data, realSize);
        }
        else {
            memcpy(_buffer+_feedBeginIndex, cbChunk->_data, truncatedSize);
            memcpy(_buffer, cbChunk->_data + truncatedSize, realSize - truncatedSize);
        }
    }

    
    size = realSize;
    
    
    
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        //check and notify, if _eatTerminated then _currentEatRequestSize=0,  verify already included
        if (_eatCapacity < _currentEatRequestSize && _eatCapacity + size >= _currentEatRequestSize)
            _monitor.notify();
        
        _eatCapacity += size;
        _feedBeginIndex = (_feedBeginIndex + size)%_totalBufferSize;
        _feedCapacity -= size;
    }
    
    //call back once more, last chunk
    if (_eatTerminated && _feedCapacity == 0) //nomore
    {
        cbChunk->_data = 0;
        cbChunk->_size = 0;
        cbChunk->_callback(cbChunk->_userData, cbChunk, _eatTerminated);
    }
}

void BytesBuffer_context::eat(size_t size, BufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        if (_eatTerminated)  return;
        while(size > _eatCapacity && !_feedTerminated) {
            _currentEatRequestSize = size;
            if ( ! _monitor.timedWait(IceUtil::Time::milliSeconds(WAITINGBUGTIME)) ) {
                SP::printf("\nwait eating %d \n", _currentFeedRequestSize);
            }
        }
        _currentEatRequestSize = 0;
    }
    //terminated
    size = size > _eatCapacity ? _eatCapacity : size;
    
    
    bool truncated = false;
    size_t truncatedSize = 0;
    //at this moment , _eatCapacity maybe increased, but do not disturb the result;
    if (_totalBufferSize < _eatBeginIndex + size ) {
        truncated = true;
        truncatedSize = _totalBufferSize - _eatBeginIndex ;
    }
    
    cbChunk->_size = size;
    if (truncated) {
        cbChunk->_data = _eatSwapBuffer;
        memcpy(cbChunk->_data, _buffer+_eatBeginIndex, truncatedSize);
        memcpy(cbChunk->_data + truncatedSize, _buffer, size-truncatedSize);
    }
    else
    {
        cbChunk->_data = _buffer+_eatBeginIndex;
    }
    //if (truncated)  SP::printf("\n<<<<<<<<<<<<<<<<<eat truncated (start: %u, size: %u, t: %u \n", _eatBeginIndex, size, truncatedSize);
    //shit out the continious buffer, the size may be changed
    size = cbChunk->_callback(cbChunk->_userData, cbChunk, _feedTerminated);
    
    
    
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        //check and notify, if _feedTerminated then _currentFeedRequestSize=0,  verify already included
        if (_feedCapacity < _currentFeedRequestSize && _feedCapacity + size >= _currentFeedRequestSize)
            _monitor.notify();
        
        _eatCapacity -= size;
        _eatBeginIndex = (_eatBeginIndex + size)%_totalBufferSize;
        _feedCapacity += size;
    }
    
    //call back once more, last chunk
    if (_feedTerminated && _eatCapacity == 0) //nomore
    {
        cbChunk->_data = 0;
        cbChunk->_size = 0;
        cbChunk->_callback(cbChunk->_userData, cbChunk, _feedTerminated);
    }
}

void BytesBuffer_context::clean()
{
    _feedBeginIndex = 0;
    _feedCapacity = _totalBufferSize;
    
    _eatBeginIndex = 0;
    _eatCapacity = 0;
    
    _feedTerminated = false;
    _eatTerminated = false;
    //SP::printf("-------------clean\n");
}

void BytesBuffer_context::terminateFeed()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    if(!_feedTerminated )
    {
        _feedTerminated = true;
        
        if (_eatTerminated)
        {
            //SP::printf("feeding over\n");
            clean();
        }
        else
        {
            _monitor.notify();
            //SP::printf("notify the eatting part, feeding terminated\n");
        }
    }
}

void BytesBuffer_context::terminateEat()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    if(_eatTerminated != true )
    {
        _eatTerminated = true;
        
        if (_feedTerminated)
        {
            //SP::printf("eatting over\n");
            clean();
        }
        else
        {
            _monitor.notify();
            //SP::printf("notify the feeding part, eatting terminated\n");
        }
    }
}

bool BytesBuffer_context::empty()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    return _feedCapacity == _totalBufferSize;
}

BytesBuffer_context::~BytesBuffer_context()
{
    free(_buffer);
    free(_eatSwapBuffer);
    free(_feedSwapBuffer);
}



//--------------------------------------------------------------------------------------------
BytesBuffer::BytesBuffer(size_t bufferSize)
{
    _ctx = std::auto_ptr<BytesBuffer_context>(new BytesBuffer_context(bufferSize));
}


void BytesBuffer::feed(size_t size, BufferChunkRef cbChunk)
{
    _ctx->feed(size, cbChunk);
}


void BytesBuffer::eat(size_t size, BufferChunkRef cbChunk)
{
    _ctx->eat(size, cbChunk);
}


void BytesBuffer::terminatedFeed()
{
    _ctx->terminateFeed();
}


void BytesBuffer::terminatedEat()
{
    _ctx->terminateEat();
}

bool BytesBuffer::empty()
{
    return  _ctx->empty();
}

void BytesBuffer::clean()
{
    _ctx->clean();
}
