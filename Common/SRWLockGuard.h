#pragma once

#pragma once

#include <windows.h>

class SRWSharedLockGuard {
public:
    explicit SRWSharedLockGuard(SRWLOCK& lock) : lock_(lock) {
        AcquireSRWLockShared(&lock_);
    }

    ~SRWSharedLockGuard() {
        ReleaseSRWLockShared(&lock_);
    }

    SRWSharedLockGuard(const SRWSharedLockGuard&) = delete;
    SRWSharedLockGuard& operator=(const SRWSharedLockGuard&) = delete;

private:
    SRWLOCK& lock_;
};

class SRWExclusiveLockGuard {
public:
    explicit SRWExclusiveLockGuard(SRWLOCK& lock) : lock_(lock) {
        AcquireSRWLockExclusive(&lock_);
    }

    ~SRWExclusiveLockGuard() {
        ReleaseSRWLockExclusive(&lock_);
    }

    SRWExclusiveLockGuard(const SRWExclusiveLockGuard&) = delete;
    SRWExclusiveLockGuard& operator=(const SRWExclusiveLockGuard&) = delete;

private:
    SRWLOCK& lock_;
};
