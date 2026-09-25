#pragma once
#include <cstdint>
namespace SweetDisplay::Transport {
enum class Admission {Accept,Disconnected,Resync};
class ResyncGate {
 bool connected=false,needIdr=true;
public:
 void Connected(){connected=true;needIdr=true;}
 void Lost(){connected=false;needIdr=true;}
 Admission Inspect(uint32_t flags)const {return !connected?Admission::Disconnected:needIdr&&(flags&7)!=7?Admission::Resync:Admission::Accept;}
 void Commit(){needIdr=false;}
};
}
