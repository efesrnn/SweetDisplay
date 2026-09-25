#include "../../host/EncoderWorkerQueue.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace SweetDisplay::Encoding;
static unsigned checks=0;
static void Check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}

struct Item {unsigned id=0;};
int main(){
 try{
  EncoderWorkerQueue<Item,4> queue;
  Check(queue.Capacity==4&&queue.Empty()&&queue.Size()==0&&queue.Peak()==0,"initial queue state");
  for(unsigned i=1;i<=4;++i)Check(queue.Push(Item{i}),"bounded queue rejected capacity item");
  Check(queue.Size()==4&&queue.Peak()==4&&!queue.Push(Item{5}),"reject-newest bound");
  for(unsigned i=1;i<=2;++i)Check(queue.Pop().id==i,"FIFO prefix ordering");
  Check(queue.Push(Item{5})&&queue.Push(Item{6})&&queue.Size()==4&&queue.Peak()==4,"ring wrap");
  for(unsigned i=3;i<=6;++i)Check(queue.Pop().id==i,"FIFO wrapped ordering");
  Check(queue.Empty()&&queue.Size()==0,"queue did not drain");
  Check(queue.Push(Item{7})&&queue.Push(Item{8}),"refill");queue.Clear();Check(queue.Empty()&&queue.Peak()==4,"bounded clear/peak");

  Check(ValidWorkerTransition(EncoderWorkerPhase::Starting,EncoderWorkerPhase::Running),"start transition");
  Check(ValidWorkerTransition(EncoderWorkerPhase::Running,EncoderWorkerPhase::Finishing),"finish transition");
  Check(ValidWorkerTransition(EncoderWorkerPhase::Finishing,EncoderWorkerPhase::Draining),"drain transition");
  Check(ValidWorkerTransition(EncoderWorkerPhase::Draining,EncoderWorkerPhase::Stopped),"stop transition");
  Check(!ValidWorkerTransition(EncoderWorkerPhase::Running,EncoderWorkerPhase::Stopped),"unbounded stop shortcut accepted");
  Check(ValidWorkerTransition(EncoderWorkerPhase::Running,EncoderWorkerPhase::Failed),"running failure transition");
  Check(!ValidWorkerTransition(EncoderWorkerPhase::Stopped,EncoderWorkerPhase::Failed),"stopped worker failed again");
  std::printf("PASS %u encoder-worker bounded queue/lifecycle checks\n",checks);return 0;
 }catch(const std::exception& error){std::fprintf(stderr,"FAIL after %u checks: %s\n",checks,error.what());return 1;}
}
