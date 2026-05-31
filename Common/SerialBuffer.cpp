#include "SerialBuffer.h"

TlsMemoryPool<SBuffer> SBuffer::BufPool(500, 500);