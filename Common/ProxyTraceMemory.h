#pragma once

#include <Windows.h>

enum class ProxyTraceTag : __int32
{
    None = 0,
    RegisterContents,
    ReleaseBeforeLeave,
    Enter,
    EnterSkipNull,
    Leave,
    LeaveSkipNull,
    UpdateNull,
    CtrlFree,
    Dtor,

    Alloc,
    Init,

    EnterAfterCreateMe,
    EnterAfterCreateOther,
    EnterEnd,

    MapUpdateCheck,
    MapDeregister,

    LeaveEnd,
    ReleaseAfterLeave
};

struct ProxyTraceEntry
{
    volatile unsigned long _beginSequence = 0;
    volatile unsigned long _endSequence = 0;

    unsigned __int64 _tick = 0;
    unsigned long _threadId = 0;
    ProxyTraceTag _tag = ProxyTraceTag::None;

    void* _contents = nullptr;
    void* _proxy = nullptr;
    void* _proxyServer = nullptr;

    __int32 _contentsNum = 0;
    __int32 _matched = 0;
    __int32 _playerCount = 0;
    __int32 _redCount = 0;
    __int32 _blueCount = 0;
    __int32 _end = 0;

    __int64 _sessionId = 0;
};

struct ProxyTraceBuffer
{
    static constexpr long kCapacity = 1 << 15;

    static ProxyTraceEntry _buffer[kCapacity];
    static volatile long _writeIndex;

    static void Write(
        ProxyTraceTag tag,
        void* contents,
        void* proxy,
        void* proxyServer,
        __int32 contentsNum,
        __int64 sessionId,
        __int32 matched,
        __int32 playerCount,
        __int32 redCount,
        __int32 blueCount,
        __int32 end)
    {
        const long index = InterlockedIncrement(&_writeIndex);
        const unsigned long sequence = static_cast<unsigned long>(index);
        ProxyTraceEntry& slot = _buffer[index & (kCapacity - 1)];

        slot._beginSequence = sequence;
        MemoryBarrier();

        slot._tick = GetTickCount64();
        slot._threadId = GetCurrentThreadId();
        slot._tag = tag;

        slot._contents = contents;
        slot._proxy = proxy;
        slot._proxyServer = proxyServer;

        slot._contentsNum = contentsNum;
        slot._matched = matched;
        slot._playerCount = playerCount;
        slot._redCount = redCount;
        slot._blueCount = blueCount;
        slot._end = end;

        slot._sessionId = sessionId;

        MemoryBarrier();
        slot._endSequence = sequence;
    }

    static bool IsCommitted(const ProxyTraceEntry& entry)
    {
        return entry._beginSequence == entry._endSequence;
    }
};