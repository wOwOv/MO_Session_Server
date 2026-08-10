# Common

`Common`은 서버와 라이브러리에서 공통으로 사용하는 패킷, 버퍼, 메모리 관리, 로깅 및 진단 기능을 제공합니다.

## 주요 구성

| 모듈 | 역할 |
|---|---|
| `CPacket` | 패킷 직렬화·역직렬화 및 `SBuffer` 참조 횟수 자동 관리 |
| `SBuffer` | 패킷 데이터와 읽기·쓰기 위치 및 참조 횟수 관리 |
| `RingBuffer` | 네트워크 수신 데이터를 처리하는 원형 버퍼 |
| `MemoryPool` | 객체 재사용을 위한 메모리 풀 |
| `TLS` | Thread Local Storage 기반 메모리 관리 및 DB 연결 |
| `LockFree` | Lock-Free Stack과 Queue |
| `Logger` | 로그 레벨 및 파일 기반 로깅 |
| `Parser` | 설정 파일의 블록과 값을 읽는 파서 |
| `Profiler` | 코드 구간별 실행 시간 측정 |
| `PDProducer` | 시스템 및 프로세스 성능 지표 수집 |
| `CrashDump` | 비정상 종료 시 Crash Dump 생성 |
| `SRWLockGuard` | SRW Lock의 획득과 해제를 관리하는 RAII Guard |
| Protocol Headers | 클라이언트·서버 통신 및 모니터링 프로토콜 정의 |

## 패킷과 버퍼

### CPacket

`CPacket`은 패킷 데이터를 직렬화·역직렬화하며, 콘텐츠 코드에서 `SBuffer`의 참조 횟수를 직접 다루지 않도록 생성·복사 생성·소멸 과정에서 참조 횟수를 자동으로 조정합니다.

기본 생성 시 TLS Memory Pool에서 `SBuffer`를 할당받고 참조 횟수를 증가시킵니다. 복사 생성된 `CPacket`은 기존 `SBuffer`를 공유하면서 참조 횟수를 증가시키며, 소멸 시 참조 횟수를 감소시킵니다.

참조 횟수는 `SBuffer`가 보유하며, 마지막 참조가 해제되면 해당 버퍼를 TLS Memory Pool로 반환합니다.

기본 자료형은 `<<`, `>>` 연산자를 이용해 패킷에 기록하거나 읽을 수 있습니다.

```cpp
CPacket packet;

packet << messageType << accountNo << positionX << positionY;
packet >> messageType >> accountNo >> positionX >> positionY;
```

`PutData()`와 `GetData()`를 사용하면 지정한 크기의 데이터를 직접 기록하거나 읽을 수 있습니다.

### SBuffer

`SBuffer`는 실제 패킷 데이터와 다음 상태를 관리합니다.

- 전체 버퍼 크기
- 읽기 위치
- 쓰기 위치
- 현재 데이터 크기
- 원자적으로 변경되는 참조 횟수

`SBuffer`는 참조 횟수를 직접 증감하는 기능을 제공합니다. 일반적인 콘텐츠 코드에서는 `CPacket`을 통해 이 과정이 생성·복사 생성·소멸 시 자동으로 처리됩니다.

마지막 참조가 해제되면 `SBuffer`는 `SBuffer::BufPool`로 반환됩니다. 이를 통해 패킷 처리마다 버퍼를 새로 생성하고 해제하는 비용을 줄였습니다.

### RingBuffer

`RingBuffer`는 네트워크에서 수신한 데이터를 순환 구조로 저장합니다.

연속되지 않은 여유 공간을 활용할 수 있도록 직접 Enqueue·Dequeue할 수 있는 위치와 크기를 제공합니다. TCP 수신 과정에서 패킷이 나뉘거나 여러 패킷이 함께 도착하는 경우를 처리하는 데 사용됩니다.

## 메모리 관리

### MemoryPool

`MemoryPool`은 객체를 매번 생성하고 해제하는 대신 미리 확보한 메모리를 재사용합니다.

빈번하게 생성되는 객체의 할당 비용을 줄이고, 현재 사용 중인 객체 수와 Pool의 전체 Capacity를 확인할 수 있습니다.

### TLS

`TLS` 디렉터리는 Thread Local Storage를 사용하는 메모리 관리 기능과 Thread별 DB 연결 기능을 포함합니다.

Thread별로 자원을 분리하여 여러 Worker Thread가 공용 자원을 두고 경쟁하는 구간을 줄입니다.

자세한 내용은 [TLS](TLS/README.md)를 참고하십시오.

### Lock-Free

`LockFree` 디렉터리는 멀티스레드 환경에서 사용하는 Lock-Free Stack과 Queue를 포함합니다.

원자적 연산을 이용해 명시적인 Mutex 없이 여러 Thread가 데이터를 추가하고 가져갈 수 있도록 구성했습니다.

자세한 내용은 [Lock-Free Containers](LockFree/README.md)를 참고하십시오.

## 설정과 로깅

### Logger

`Logger`는 Singleton으로 관리되며 로그 레벨과 저장 경로를 설정할 수 있습니다.

지원하는 로그 레벨은 다음과 같습니다.

| 레벨 | 용도 |
|---|---|
| `LVDEBUG` | 디버깅 정보 |
| `LVERROR` | 오류 정보 |
| `LVSYSTEM` | 서버 실행과 상태 정보 |

로그에는 분류, 기록 시각, 로그 번호 및 로그 레벨이 포함되며, 설정한 디렉터리에 월 단위 텍스트 파일로 저장됩니다.

### Parser

`Parser`는 설정 파일을 메모리에 불러온 뒤 블록 이름과 항목 이름을 기준으로 값을 읽습니다.

정수와 문자열 설정을 지원하며 서버 주소, 포트, Worker Thread 수와 같은 실행 설정을 불러오는 데 사용됩니다.

```cpp
Parser parser;

parser.LoadFile("ServerConfig.cnf");
parser.GetValue("SERVER", "PORT", &port);
parser.GetString("SERVER", "IP", ip, sizeof(ip));
```

## 진단 도구

### Profiler

`Profiler`는 `PRO_BEGIN`과 `PRO_END` 사이의 실행 시간을 측정합니다.

`Profile` 객체를 사용하면 객체가 생성될 때 측정을 시작하고 Scope를 벗어날 때 자동으로 측정을 종료할 수 있습니다.

측정 결과는 `ProfileDataOutText()`를 통해 텍스트 파일로 출력할 수 있습니다.

```cpp
{
    Profile profile("FightContents::OnUpdate");
    OnUpdate();
}
```

Profiler는 `ENABLE_PROFILER`가 정의된 경우에만 활성화됩니다.

### PDProducer

`PDProducer`는 Windows PDH와 Process 정보를 이용해 성능 지표를 수집합니다.

| 클래스 | 수집 항목 |
|---|---|
| `HwPDProducer` | 전체 CPU 사용률, 가용 메모리, Nonpaged Memory, 네트워크 송수신량 |
| `PcPDProducer` | 프로세스 CPU 사용률, User Memory, Nonpaged Memory 및 프로세스 메모리 사용량 |

수집된 지표는 서버 상태 확인과 모니터링 데이터 전송에 사용할 수 있습니다.

### CrashDump

`CrashDump`는 서버에서 처리되지 않은 예외가 발생했을 때 분석을 위한 Dump 파일을 생성합니다.

생성된 Dump 파일은 Visual Studio에서 실행 당시의 Call Stack과 Thread 상태를 확인하는 데 사용할 수 있습니다.

## 동기화 도구

`SRWLockGuard`는 Windows SRW Lock의 획득과 해제를 Scope 단위로 관리합니다.

| 클래스 | Lock 방식 |
|---|---|
| `SRWSharedLockGuard` | Shared Lock |
| `SRWExclusiveLockGuard` | Exclusive Lock |

Guard 객체가 생성될 때 Lock을 획득하고 Scope를 벗어날 때 자동으로 해제합니다. 복사 생성과 복사 대입은 금지되어 있습니다.

```cpp
{
    SRWSharedLockGuard guard(lock);
    // Shared Lock이 필요한 작업
}
```

## 프로토콜 정의

| 파일 | 역할 |
|---|---|
| `CommonProtocol.h` | 클라이언트·서버 통신에 사용하는 패킷 타입과 상태값 정의 |
| `MonitorProtocol.h` | 모니터링 데이터 전송에 사용하는 패킷 타입과 지표 정의 |

## Related Documentation

- [Project Overview](../README.md)
- [TLS](TLS/README.md)
- [Lock-Free Containers](LockFree/README.md)
