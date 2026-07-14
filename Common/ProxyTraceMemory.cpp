#include "ProxyTraceMemory.h"

ProxyTraceEntry ProxyTraceBuffer::_buffer[ProxyTraceBuffer::kCapacity]{};
volatile long ProxyTraceBuffer::_writeIndex = -1;