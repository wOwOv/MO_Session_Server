# MOSessionServer

## 개요
MOSessionServer는 RPC 기반으로 동작하는 게임 서버로,  
매칭을 통해 플레이어를 그룹화하고 전투 컨텐츠를 생성하여 처리하는 구조를 구현했습니다.

클라이언트는 MatchContents에서 매칭을 수행한 뒤,  
FightContents로 이동하여 전투를 진행하는 흐름으로 구성되어 있습니다.

---
## 서버 구조
![MOSession](../../Images/MOSession.png)

---
## 역할
- 매칭 컨텐츠 처리 (MatchContents)
- 전투 컨텐츠 생성 및 관리 (FightContents)
- RPC 기반 요청/응답 처리
- 컨텐츠 간 전환 및 세션 이동 관리

---

## 사용 라이브러리
- ContentLibB(RPC Compiler)
- Common

---

## 구조 특징

### 1. 컨텐츠 단위 직렬화 (락 기반)
- 동일 컨텐츠에 대한 로직은 동시에 수행되지 않도록 구성
- 컨텐츠에 대한 락을 획득한 상태에서 로직 수행
- 멀티스레드 환경에서도 데이터 일관성 유지

---

### 2. 워커 스레드 직접 처리 구조
- 별도의 컨텐츠 전용 스레드를 생성하지 않음
- IOCP 워커 스레드가 컨텐츠 락을 획득하고 직접 로직 수행

---

### 3. 컨텐츠 기반 상태 분리
- MatchContents: 매칭 처리
- FightContents: 전투 처리
- 컨텐츠 단위로 상태와 로직 분리

---

### 4. ControlThread 기반 컨텐츠 관리
- FightContents 생성 및 삭제 담당
- 컨텐츠 등록 및 반환 처리

---

## 동작 흐름

1. 클라이언트가 서버에 접속합니다.
2. 세션은 기본적으로 MatchContents에 할당됩니다.
3. MatchContents에서 플레이어 매칭을 수행합니다.
4. 6명의 플레이어가 모이면 ControlThread에 FightContents 생성을 요청합니다.
5. ControlThread가 새로운 FightContents를 생성하고 등록합니다.
6. 매칭된 6명의 플레이어를 해당 FightContents로 이동시킵니다.
7. 모든 플레이어 이동이 완료되면 FightContents에서 플레이어 생성 메시지를 전송합니다.
8. 이후 클라이언트 입력에 따라 전투 로직이 수행됩니다.
9. 게임 종료 조건이 만족되면 FightContents는 ControlThread에 반환 요청을 합니다.
10. ControlThread가 해당 FightContents를 해제하고 반환합니다.

---

## 컨텐츠 프레임 처리 중 발생한 문제

컨텐츠는 일정 주기로 로직을 수행하기 위해 OnUpdate 기반 프레임 처리 구조를 사용했습니다.  
OnUpdate 실행은 PostQueuedCompletionStatus(PQCS)를 통해 IOCP 워커 스레드에 전달되며,  
실행 이후 일정 시간 대기 후 다시 OnUpdate 작업을 등록하는 방식으로 동작합니다.

---

### 문제 상황

FightContents의 수가 증가하면서  
컨텐츠 생성 및 삭제를 담당하는 ControlThread가 정상적으로 동작하지 않는 문제가 발생했습니다.

MatchContents에서 새로운 FightContents를 생성하기 위해 RegisterContents를 호출했지만,  
해당 함수가 장시간 반환되지 않는 현상이 발생했습니다.

이로 인해 컨텐츠 생성 및 삭제가 즉시 처리되지 않고  
서버 동작이 지연되는 문제가 발생했습니다.

---

### 문제 원인

문제의 원인은 컨텐츠 프레임 처리 과정에서  
contentsMap에 대한 shared lock을 장시간 유지하고 있었기 때문입니다.

기존 구조에서는 다음과 같은 흐름으로 동작했습니다.

- contentsMap shared lock 획득
- OnUpdate 실행
- Sleep 수행
- 다음 OnUpdate 작업을 PQCS로 등록
- lock 해제

이 구조에서는 OnUpdate 실행뿐 아니라 Sleep 구간까지 shared lock이 유지됩니다.

컨텐츠 수가 증가하면 여러 스레드가 shared lock을 지속적으로 점유하게 되고,  
ControlThread는 contentsMap에 대한 exclusive lock을 획득하지 못하게 됩니다.

결과적으로 exclusive lock이 장시간 대기 상태에 빠지는  
락 기아(starvation) 현상이 발생했습니다.

---

### 해결 방법

락 유지 범위를 최소화하도록 OnUpdate 처리 구조를 수정했습니다.

수정된 흐름은 다음과 같습니다.

- contentsMap shared lock 획득
- OnUpdate 실행
- lock 즉시 해제
- Sleep 수행
- 다시 lock 획득 후 컨텐츠 존재 여부 확인
- 다음 OnUpdate 작업을 PQCS로 등록

이와 같은 변경을 통해 contentsMap에 대한 점유 시간을 크게 줄일 수 있었고,  
ControlThread가 exclusive lock을 빠르게 획득할 수 있게 되었습니다.

그 결과 컨텐츠 생성 및 삭제 지연 문제가 해결되었습니다.

---

### 핵심 정리

- 락의 종류뿐 아니라 **락 유지 범위와 시점**이 매우 중요함
- Sleep과 같은 대기 구간에서 락을 유지하는 것은 성능 문제를 유발함
- 멀티스레드 환경에서는 **락 점유 시간 최소화**가 중요함

---

## 핵심 구현 포인트

- RPC Compiler 기반 코드 생성 구조 구현
- 컨텐츠 단위 락 기반 직렬화 구조
- 워커 스레드 직접 로직 처리 모델
- Match → Fight 컨텐츠 전환 구조 설계
- ControlThread 기반 컨텐츠 관리
- 컨텐츠 프레임 처리 구조 설계 및 문제 해결

---

## 특징

- 매칭과 전투를 분리한 컨텐츠 구조
- 컨텐츠 단위 직렬화를 통한 안정적인 로직 처리
- 세션 기반 컨텐츠 관리 구조
- 실제 게임 서버 흐름을 반영한 설계

---

## 한계

- 컨텐츠 락을 획득한 상태로 로직이 수행되므로  
  로직 수행 시간이 길어질 경우 대기 시간이 증가할 수 있습니다.
- 동일 컨텐츠에 대한 요청은 직렬화되므로  
  특정 컨텐츠에 요청이 집중될 경우 병목이 발생할 수 있습니다.

