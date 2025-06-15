--------------------------------------------------------

TCP Select Model 기반 서버입니다.

Client는 제공받았고, 서버만 직접 만들었습니다.

--------------------------------------------------------

- Client 조작

이동 : 방향키
공격 : z, x, c
(죽으면 접속 종료)

- Client 접속 ip는 config파일에서 수정할 수 있습니다.
- 서버는 5000번 포트를 사용합니다.
- ------------------------------------------------------
**1.1**

**Json파일을 기반으로한 RPC를 추가하였습니다.**

**RPCComplier는 
proxy.h, proxy.cpp,
stub.h, stub.cpp를 생성합니다.**

- proxy에서 호출한 함수는 Stub::RPC_PacketProcedure(int SessionId, Stub* handle)에서 실행합니다.
SessionId는 참여자를 구분합니다.
handle은 Stub을 상속받은, 실제 동작할 함수를 정의하는 클래스입니다.

**RPC는 상대방 함수를 호출한다는 개념으로써, 네트워크 상의 일련의 과정들을
추상화하는 방법론입니다.**


- 송신자
 proxy.func1(id, ...) -> Engine(Ringbuffer의 내용물 Send) -> 네트워크

 - 수신자
 네트워크 -> Engine(Ringbuffer의 내용물 Recv) -> Stub::RPC_PacketProcedure -> handle->func1(...)

가변인자를 위해 const char\*, char\*와 List를 호환합니다.
가변인자 패킷을 만드는 경우, 반드시 뒤에 크기가 와야합니다.
(char*형은 int size(바이트 크기에 해당), list는 int num(요소 개수에 해당))

-------------------------------------------------------

 
