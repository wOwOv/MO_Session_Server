#pragma once

#include <cstdint>

class MatchIDGenerator
{
public:
    explicit MatchIDGenerator(std::uint16_t serverID = 0);

    // This generator is not thread-safe.
    // It should be used only by the owning control thread.
    __int64 Create();

private:
    static constexpr std::uint64_t CUSTOM_EPOCH_MS = 1767225600000ULL; // 2026-01-01 UTC
    static constexpr std::uint16_t MAX_SERVER_ID = 0x03FF;             // 10bit
    static constexpr std::uint16_t MAX_SEQUENCE = 0x0FFF;              // 12bit

    static constexpr std::uint32_t SERVER_ID_SHIFT = 12;
    static constexpr std::uint32_t TIMESTAMP_SHIFT = 22;

private:
    static std::uint64_t GetRelativeUnixTimestampMs();

    static std::uint64_t WaitNextMillisecond(
        std::uint64_t lastTimestamp);

    static __int64 MakeID(
        std::uint64_t timestamp,
        std::uint16_t serverID,
        std::uint16_t sequence);

private:
    std::uint64_t _lastTimestamp = 0;
    std::uint16_t _serverID = 0;
    std::uint16_t _sequence = 0;
};