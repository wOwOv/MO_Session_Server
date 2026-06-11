#include "MatchIDGenerator.h"

#include <chrono>
#include <cassert>

MatchIDGenerator::MatchIDGenerator(std::uint16_t serverID)
    : _serverID(serverID)
{
    assert(serverID <= MAX_SERVER_ID);
}

__int64 MatchIDGenerator::Create()
{
    std::uint64_t nowMs = GetRelativeUnixTimestampMs();

    if (nowMs < _lastTimestamp)
    {
        nowMs = _lastTimestamp;
    }

    if (nowMs == _lastTimestamp)
    {
        ++_sequence;

        if (_sequence > MAX_SEQUENCE)
        {
            nowMs = WaitNextMillisecond(_lastTimestamp);
            _sequence = 0;
        }
    }
    else
    {
        _sequence = 0;
    }

    _lastTimestamp = nowMs;

    return MakeID(nowMs, _serverID, _sequence);
}

std::uint64_t MatchIDGenerator::GetRelativeUnixTimestampMs()
{
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto unixMs = duration_cast<milliseconds>(
        now.time_since_epoch()).count();

    return static_cast<std::uint64_t>(unixMs) - CUSTOM_EPOCH_MS;
}

std::uint64_t MatchIDGenerator::WaitNextMillisecond(
    std::uint64_t lastTimestamp)
{
    std::uint64_t nowMs = GetRelativeUnixTimestampMs();

    while (nowMs <= lastTimestamp)
    {
        nowMs = GetRelativeUnixTimestampMs();
    }

    return nowMs;
}

__int64 MatchIDGenerator::MakeID(
    std::uint64_t timestamp,
    std::uint16_t serverID,
    std::uint16_t sequence)
{
    const std::uint64_t id =
        (timestamp << TIMESTAMP_SHIFT) |
        (static_cast<std::uint64_t>(serverID) << SERVER_ID_SHIFT) |
        sequence;

    return static_cast<__int64>(id);
}