# ContentLibB

## 개요
ContentLibB는 IOCP 기반 네트워크 계층 위에서 동작하는 컨텐츠 처리 라이브러리로,  
RPC 기반 메시지 처리와 멀티스레드 환경에서의 컨텐츠 실행을 지원하기 위해 설계되었습니다.

네트워크에서 수신된 요청을 RPC 형태로 해석하고, 해당 컨텐츠의 로직을 실행하는 구조를 제공하며,  
컨텐츠 데이터와 처리 흐름을 분리하여 서버 확장성과 구조적 명확성을 확보하는 것을 목표로 했습니다.

---
## 라이브러리 구조
![ContentLibB](../../Images/contentlibB_structure.png)

---
## 사용법

RPCDefinition
![defitinion](../../Images/definition.png)
##사이의 문자열: RPCCompiler가 생성할 파일의 이름 #Example#->ExampleDefine.h

<>사이의 문자열: Stub과 Proxy 클래스의 이름 <Town>->class TownProxy

CS: Client->Server Stub클래스의 함수로 선언됨

SC: Server->Client Proxy클래스의 함수로 선언, 정의됨

SCCreateMe: 함수의 이름이 됨

(unsigned int id,unsigned char dir,unsigned short x,unsigned short y,unsigned char hp): 메시지 데이터 구조

'#'뒤의 숫자: 패킷 식별 번호

1. RPCDefinition 규칙을 따라서 Stub, Proxy에 넣을 함수들 작성(ContentsServer와 Content 모두 세션으로부터 메시지가 오면 Stub의 ProcMessage를 호출하므로 위 예시의 Match 처럼 빈칸으로 두고 작성 후 연결)
2. 빌드 전 이벤트를 사용하여 ServerRPCCompiler.exe와 Definition File을 인자로 넘겨 실행
3. Define.h, Proxy.h, Proxy.cpp, Stub.h, Stub.cpp가 생성되었다면 프로젝트로 가져오기
4. ContentsServer를 상속받은 클래스 구현 - 이벤트 핸들러 함수 오버라이딩
- OnConnectionRequest 클라이언트 연결 요청 시 세션을 생성하기 전에 호출되며, 연결을 허용할지 결정할 기회를 제공한다.
- OnAccept 세션이 생성된 이후 호출되며, 연결이 정상적으로 수락되었음을 알린다.
- OnRelease 세션이 정리된 이후 호출되며, 이후에는 해당 sessionID를 가진 세션에 대해 어떤 이벤트도 발생하지 않음을 보장한다.
- OnUnusual 메시지 디코딩에 실패하거나 패킷 헤더의 코드가 일치하지 않는 등 비정상적인 상황이 발생했을 때 호출된다.
- OnSecond 모니터링 스레드에 의해 1초마다 호출되며, 주기적인 상태 확인이나 통계 처리와 같은 작업에 사용된다.
  
(메시지가 도착했을때 세션이 속한 컨텐츠가 없을시 ContentsServer에 연결된 Stub의 ProcMessage 호출)

5. Contents를 상속받은 클래스 구현 - 이벤트 핸들러 함수 오버라이딩
- OnEnter 컨텐츠에 세션 입장시 호출
- OnLeave 세션 퇴장시 호출
- OnUpdate 설정한 프레임마다 호출
  
  (메시지 도착시 Contents에 연결된 Stub의 ProcMessage 호출)
  
6. Proxy의 경우 해당 클래스 객체 생성 후 필요한 Contents나 ContentsServer에 AttachProxy
7. Stub의 경우 선언된 Stub클래스를 상속받은 클래스에서 가상 함수 오버라이딩
8. 구현한 Stub클래스 객체 생성 후 Contents나 ContentsServer에 AttachStub
9. SetDefaultContents로 기본 컨텐츠 설정 가능
10. InsertToContents: 다른 컨텐츠로 이동시키기, DeleteFromContents: 컨텐츠에서 퇴장시키기
11.  ContentsServer의 Start함수 호출하여 서버 시작
 ---

Contents를 상속받아 클래스 생성
Contents의 이벤트 핸들러 함수 오버라이딩


## 역할
- RPC 기반 메시지 처리 구조 제공
- 컨텐츠 단위 로직 실행
- 네트워크 계층과 컨텐츠 계층 분리
- 멀티스레드 환경에서의 컨텐츠 처리 지원

---

## 주요 구성

### Content 객체
- 게임 로직 단위를 담당하는 객체
- 컨텐츠 데이터를 관리
- RPC 요청에 대응하는 로직 함수 보유

### RPC 처리 구조
- 수신된 Packet을 RPC 메시지로 변환
- RPC ID 기반으로 핸들러 함수 매핑
- Stub / Proxy 구조를 통해 호출 흐름 구성

### 네트워크 연동
- ContentsServer에서 전달된 이벤트를 기반으로 동작
- Session과 컨텐츠를 연결하여 요청 처리

---

## 동작 흐름

1. 클라이언트 또는 서버로부터 Packet이 수신됩니다.
2. Packet은 RPC 메시지 형태로 파싱됩니다.
3. RPC ID를 기준으로 해당 컨텐츠의 핸들러가 결정됩니다.
4. 컨텐츠 로직이 실행됩니다.
5. 필요 시 결과를 RPC 응답 형태로 송신합니다.

---

## 핵심 구현 포인트

- RPC 기반 요청 / 응답 처리 구조 직접 구현
- Packet → RPC → Content 로 이어지는 처리 흐름 구성
- 컨텐츠 로직과 네트워크 처리의 명확한 분리
- 멀티스레드 환경에서 컨텐츠 처리 구조 설계
- RPC Compiler를 통한 코드 생성 및 호출 구조 구성

---

## ContentLibA와의 차이

| 항목 | ContentLibA | ContentLibB |
|------|------------|------------|
| 실행 트리거 | 이벤트 큐 | RPC 호출 |
| 직렬화 | 컨텐츠 단위 직렬화 | 컨텐츠 단위 직렬화 |
| 메시지 모델 | 이벤트 | 요청/응답(RPC) |
| 구조 초점 | 실행 모델 | 통신 모델 |

---

## 특징

- RPC 기반으로 명확한 요청/응답 흐름 구성
- 네트워크 계층과 컨텐츠 계층의 역할 분리
- 다양한 서버 구조에 적용 가능한 확장성 확보
- 실제 게임 서버 구조에 가까운 처리 모델 구성

