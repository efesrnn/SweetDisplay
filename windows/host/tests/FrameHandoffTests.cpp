// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#define NOMINMAX
#include "../../shared/FrameHandoffProtocol.h"
#include "../../shared/PatternOracle.h"
#include "../../shared/ContentClassification.h"
#include "../DesktopSample.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace SweetDisplay::Handoff;
static unsigned checks=0;
static void Check(bool ok,const char* what){++checks;if(!ok){fprintf(stderr,"FAIL %s\n",what);exit(1);}}
static Frame Make(uint64_t id){Frame f{};f.id=id;f.qpc=id*100;f.epoch=99;f.width=2400;f.height=1080;f.format=87;return f;}
int main(){
    auto c=Packet<Connect>();c.epoch=99;c.width=2400;c.height=1080;c.format=87;
    for(unsigned i=0;i<Capacity;++i)swprintf_s(c.names[i],L"Global\\SweetDisplay.12345678-abcd-1234-abcd123456781234.%u",i);
    Check(ValidConnect(c,sizeof(c)),"valid connect");
    std::vector<BYTE> bytes(sizeof(c));memcpy(bytes.data(),&c,sizeof(c));Connect decoded{};memcpy(&decoded,bytes.data(),sizeof(decoded));
    Check(ValidConnect(decoded,bytes.size())&&!memcmp(&decoded,&c,sizeof(c)),"wire round trip");
    Check(!ValidConnect(c,sizeof(c)-1),"truncated packet");Check(!ValidConnect(c,sizeof(c)+1),"oversized packet");
    auto bad=c;bad.header.version++;Check(!ValidConnect(bad,sizeof(bad)),"unknown version");bad=c;bad.header.magic=0;Check(!ValidConnect(bad,sizeof(bad)),"wrong magic");
    bad=c;bad.width=UINT32_MAX;Check(!ValidConnect(bad,sizeof(bad)),"unbounded width");bad=c;bad.count=4;Check(!ValidConnect(bad,sizeof(bad)),"slot count");
    bad=c;for(auto& x:bad.names[0])x=L'a';Check(!ValidConnect(bad,sizeof(bad)),"unterminated name");
    bad=c;wcscpy_s(bad.names[1],bad.names[0]);Check(!ValidConnect(bad,sizeof(bad)),"aliased slots");
    bad=c;bad.names[0][25]=L'\\';Check(!ValidConnect(bad,sizeof(bad)),"foreign namespace");
    auto f=Make(1);Check(ValidFrame(f,99,0,0,100),"valid frame");Check(!ValidFrame(f,98,0,0,100),"epoch changed");
    Check(!ValidFrame(f,99,1,0,100),"duplicate ID");Check(!ValidFrame(f,99,2,0,100),"reversed ID");
    Check(!ValidFrame(f,99,0,100,200),"nonmonotonic timestamp");Check(!ValidFrame(f,99,0,0,99),"future timestamp");
    f.flags=1;Check(!ValidFrame(f,99,0,0,100),"protected/unknown flags");
    Check(Milliseconds(259689,10000000)==25.9689,"QPC conversion");Check(Milliseconds(10,0)==0,"zero frequency guarded");
    Queue q;for(uint64_t id=1;id<=3;++id){int slot=q.ProducerSlot();Check(slot>=0,"free capacity");q.Publish(slot,Make(id));}
    Check(q.stats.depth==3&&q.stats.highWater==3,"bounded capacity");q.Publish(q.ProducerSlot(),Make(4));Check(q.stats.producerDrops==1,"oldest producer replacement");
    int newest=q.Fetch();Check(q.slots[newest].frame.id==4&&q.stats.hostQueueDrops==2&&q.stats.held==1,"latest Host policy");
    Check(q.Fetch()==-1,"one outstanding lease");
    for(uint64_t id=5;id<1005;++id){int slot=q.ProducerSlot();Check(slot!=newest&&slot>=0,"held never overwritten");q.Publish(slot,Make(id));Check(q.stats.depth+q.stats.held<=Capacity,"capacity under slow consumer");}
    auto ack=Packet<Ack>();ack.epoch=99;ack.id=3;ack.slot=newest;Check(!q.Acknowledge(ack),"stale ack");ack.id=4;Check(q.Acknowledge(ack),"release lease");Check(!q.Acknowledge(ack),"duplicate ack");
    q={};Check(q.stats.depth==0&&q.stats.held==0,"disconnect clears metadata");
    q.Publish(q.ProducerSlot(),Make(2000));Check(q.Fetch()>=0,"fresh reconnect");
    q={};for(auto& slot:q.slots)slot.state=SlotState::Held;Check(q.ProducerSlot()==-1,"fully held pool drops input");
    printf("PASS %u production metadata/queue checks\n",checks);
    const auto baselineChecks=checks;
    using namespace SweetDisplay::PatternOracle;
    Check(Evaluate(7,7,10,11,12,true,90,100)==0,"new published content");
    Check(Evaluate(7,7,11,11,12,true,90,100)==0,"unchanged desktop content is allowed");
    Check(Evaluate(7,0,11,0,12,false,0,100)==(Zero|Nonce),"zero overlay fails closed");
    Check(Evaluate(7,7,11,0,12,false,0,100)==Zero,"zero counter is not accepted");
    Check(Evaluate(7,8,11,10,12,true,90,100)==Nonce,"foreign pattern fails");
    Check(Evaluate(7,7,16148,16122,16200,true,90,100)==Regression,"historical regression rejected");
    Check(Evaluate(7,7,10,13,12,false,0,100)==(Unpublished|Expired),"unrendered future counter rejected");
    Check(Evaluate(7,7,10,11,5000,false,0,100)==Expired,"expired ledger cannot silently pass");
    Check(Evaluate(7,7,10,11,12,true,101,100)==Future,"render after acquisition rejected");
    Check(Evaluate(7,7,10,11,12,true,100,100)==0,"equal timestamp allowed");
    Check(Evaluate(7,7,10,100,100,true,90,100)==0,"dropped intermediate counters allowed");
    const uint32_t nonce=GetCurrentProcessId()^GetTickCount()^0x98765432u;
    Mapping writer,reader;writer.Open(nonce,true);reader.Open(nonce,false);
    uint64_t stamp=0;writer.Publish(1,100);
    Check(reader.Lookup(1,stamp)&&stamp==100,"shared producer ledger read");
    writer.Publish(1+SweetDisplay::PatternOracle::Capacity,200);
    Check(!reader.Lookup(1,stamp),"overwritten ring entry rejected");
    Check(reader.Lookup(1+SweetDisplay::PatternOracle::Capacity,stamp)&&stamp==200,"ring wrap preserves new identity");
    InterlockedExchange(&reader.data->hostCounter,42);
    Check(Load(&writer.data->hostCounter)==42,"controlled test feedback");
    bool collision=false;try{Mapping duplicate;duplicate.Open(nonce,true);}catch(const std::exception&){collision=true;}
    Check(collision,"producer recreation cannot alias existing generation");
    printf("PASS %u diagnostic oracle/ledger checks\n",checks-baselineChecks);
    const auto classificationStart=checks;
    namespace Content=SweetDisplay::Content;
    Check(Content::Classify(true,true,false,false)==Content::A,"valid pattern needs no independent transition proof");
    Check(Content::Classify(true,false,false,true)==Content::B,"independently matched desktop absence allowed");
    Check(Content::Classify(true,true,true,true)==Content::C,"independently matched old desktop content allowed");
    Check(Content::Classify(true,false,false,false)==Content::E,"zero/foreign pixels without corroboration stop UNKNOWN");
    Check(Content::Classify(true,true,true,false)==Content::E,"regression without corroboration stops UNKNOWN");
    for(bool pattern:{false,true})for(bool regression:{false,true})for(bool reference:{false,true})Check(Content::Classify(false,pattern,regression,reference)==Content::D,"integrity failure cannot be masked by matching pixels");
    SweetDisplay::DesktopSample::Pixels pixels{};
    for(unsigned row=0;row<2;++row)for(unsigned x=0;x<16;++x)for(unsigned channel=0;channel<3;++channel)pixels[(row*640+row*20+x)*4+channel]=255;
    Check(SweetDisplay::DesktopSample::BinaryCells(pixels,1,2),"exact producer cell structure");
    auto different=pixels;different[3]=19;
    Check(SweetDisplay::DesktopSample::Equal(pixels,different),"GDI unused alpha ignored, RGB retained");
    different[0]=254;
    Check(!SweetDisplay::DesktopSample::Equal(pixels,different),"one channel difference rejects corroboration");
    Check(!SweetDisplay::DesktopSample::BinaryCells(different,1,2),"threshold decode alone insufficient for valid cells");
    printf("PASS %u classified acceptance checks\n",checks-classificationStart);return 0;
}

