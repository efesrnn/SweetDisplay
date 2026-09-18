// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <memory>
#include <atomic>
#include "Protocol.h"
namespace SweetDisplay::Transport {
namespace P=SweetDisplay::Protocol;
inline uint64_t Now(){LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);return P::Nanoseconds(uint64_t(q.QuadPart),uint64_t(f.QuadPart));}
inline uint64_t SessionId(){uint64_t id=0;while(!id){if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&id),sizeof(id),BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)throw std::runtime_error("session random failure");}return id;}
struct IoError:std::runtime_error{int code;IoError(const char* text,int c):std::runtime_error(text),code(c){}};
struct Winsock {Winsock(){WSADATA w{};int e=WSAStartup(MAKEWORD(2,2),&w);if(e)throw IoError("WSAStartup",e);}~Winsock(){WSACleanup();}};
// Read/write SOME bytes. Deadlines and EOF are transport results, never framing.
struct ByteStream {virtual ~ByteStream()=default;virtual size_t Read(uint8_t*,size_t,uint64_t)=0;virtual size_t Write(const uint8_t*,size_t,uint64_t)=0;};
class TcpStream final:public ByteStream {
 SOCKET socket=INVALID_SOCKET;const std::atomic<bool>* cancel=nullptr;
 void Wait(bool writing,uint64_t deadline){
  for(;;){if(cancel&&cancel->load())throw IoError("cancelled",WSAEINTR);auto now=Now();if(now>=deadline)throw IoError("I/O deadline",WSAETIMEDOUT);
   fd_set ready,errors;FD_ZERO(&ready);FD_ZERO(&errors);FD_SET(socket,&ready);FD_SET(socket,&errors);timeval t{};t.tv_usec=long(std::min<uint64_t>(20000,(deadline-now)/1000));
   int r=select(0,writing?nullptr:&ready,writing?&ready:nullptr,&errors,&t);if(r==SOCKET_ERROR)throw IoError("select",WSAGetLastError());
   if(FD_ISSET(socket,&errors)){int e=0;int len=sizeof(e);getsockopt(socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&e),&len);throw IoError("socket exception",e);}
   if(r&&FD_ISSET(socket,&ready))return;
  }
 }
public:
 explicit TcpStream(SOCKET s,const std::atomic<bool>* c=nullptr):socket(s),cancel(c){
  u_long nonblocking=1;int bytes=65536,one=1;
  if(ioctlsocket(s,FIONBIO,&nonblocking)||setsockopt(s,SOL_SOCKET,SO_SNDBUF,reinterpret_cast<char*>(&bytes),sizeof(bytes))||setsockopt(s,SOL_SOCKET,SO_RCVBUF,reinterpret_cast<char*>(&bytes),sizeof(bytes))||setsockopt(s,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<char*>(&one),sizeof(one))){auto e=WSAGetLastError();closesocket(socket);socket=INVALID_SOCKET;throw IoError("socket configuration",e);}
 }
 ~TcpStream(){if(socket!=INVALID_SOCKET)closesocket(socket);}
 TcpStream(const TcpStream&)=delete;TcpStream& operator=(const TcpStream&)=delete;
 static std::unique_ptr<TcpStream> Connect(uint16_t port,const std::atomic<bool>* cancel=nullptr){
  SOCKET s=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(s==INVALID_SOCKET)throw IoError("socket",WSAGetLastError());auto t=std::make_unique<TcpStream>(s,cancel);
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(port);address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
  int r=connect(s,reinterpret_cast<sockaddr*>(&address),sizeof(address));if(r==SOCKET_ERROR){auto e=WSAGetLastError();if(e!=WSAEWOULDBLOCK)throw IoError("connect",e);t->Wait(true,Now()+1000000000ULL);int error=0,len=sizeof(error);if(getsockopt(s,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&len)||error)throw IoError("connect completion",error?error:WSAGetLastError());}return t;
 }
 size_t Read(uint8_t* p,size_t n,uint64_t deadline)override{for(;;){Wait(false,deadline);int r=recv(socket,reinterpret_cast<char*>(p),int(std::min(n,size_t(INT_MAX))),0);if(r>0)return size_t(r);if(!r)throw IoError("peer EOF",0);int e=WSAGetLastError();if(e!=WSAEWOULDBLOCK)throw IoError("recv",e);}}
 size_t Write(const uint8_t* p,size_t n,uint64_t deadline)override{for(;;){Wait(true,deadline);int r=send(socket,reinterpret_cast<const char*>(p),int(std::min(n,size_t(INT_MAX))),0);if(r>0)return size_t(r);int e=WSAGetLastError();if(e!=WSAEWOULDBLOCK)throw IoError("send",e);}}
};
class Listener {
 SOCKET socket=INVALID_SOCKET;
public:
 explicit Listener(uint16_t port){
  socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(socket==INVALID_SOCKET)throw IoError("listen socket",WSAGetLastError());
  BOOL exclusive=TRUE;u_long nb=1;sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(port);
  if(setsockopt(socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof(exclusive))||bind(socket,reinterpret_cast<sockaddr*>(&a),sizeof(a))||listen(socket,1)||ioctlsocket(socket,FIONBIO,&nb)){auto e=WSAGetLastError();closesocket(socket);socket=INVALID_SOCKET;throw IoError("exclusive loopback listener",e);}
 }
 ~Listener(){if(socket!=INVALID_SOCKET)closesocket(socket);}
 std::unique_ptr<TcpStream> Accept(){auto s=accept(socket,nullptr,nullptr);if(s!=INVALID_SOCKET)return std::make_unique<TcpStream>(s);if(WSAGetLastError()!=WSAEWOULDBLOCK)throw IoError("accept",WSAGetLastError());return {};}
};
class Channel {
 ByteStream& stream;P::Parser parser;
public:
 uint64_t bytesWritten=0,bytesRead=0,messagesWritten=0,messagesRead=0;
 explicit Channel(ByteStream& s):stream(s){}
 bool Partial()const{return parser.Partial();}
 void Send(const P::Message& m,uint64_t deadline){auto data=P::Wire(m);size_t offset=0;while(offset<data.size()){auto n=stream.Write(data.data()+offset,data.size()-offset,deadline);P::Require(n&&n<=data.size()-offset,"stream write contract");offset+=n;bytesWritten+=n;}++messagesWritten;}
 P::Message Receive(uint64_t deadline){P::Message message;bool done=false;std::array<uint8_t,16384> bytes{};while(!done){size_t n=stream.Read(bytes.data(),std::min(bytes.size(),parser.Needed()),deadline);P::Require(n&&n<=std::min(bytes.size(),parser.Needed()),"stream read contract");bytesRead+=n;parser.Feed(bytes.data(),n,[&](P::Message& m){message=std::move(m);done=true;});}++messagesRead;return message;}
};
inline void HostHandshake(Channel& channel,P::Connection& c){auto deadline=Now()+2000000000ULL;channel.Send(c.Make(P::Type::Hello,P::Hello(P::Role::Host),Now()),deadline);c.Receive(channel.Receive(deadline));channel.Send(c.Make(P::Type::Capabilities,P::Capabilities(),Now()),deadline);c.Receive(channel.Receive(deadline));P::Require(c.GetState()==P::State::Ready,"handshake not ready");}
inline void DeviceHandshake(Channel& channel,P::Connection& c){auto deadline=Now()+2000000000ULL;c.Receive(channel.Receive(deadline));channel.Send(c.Make(P::Type::Hello,P::Hello(P::Role::Device),Now()),deadline);channel.Send(c.Make(P::Type::Capabilities,P::Capabilities(),Now()),deadline);c.Receive(channel.Receive(deadline));P::Require(c.GetState()==P::State::Ready,"handshake not ready");}
}
