// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
// Windows-only deterministic exercise of the production TouchInput::Session.
#include "../host/TouchInput.h"
#include <cstdio>
#include <filesystem>
#include <string>

namespace T=SweetDisplay::Touch;
namespace I=SweetDisplay::TouchInput;
static uint64_t stamp=1000;
static T::Event Event(uint16_t contact,T::Action action,uint32_t x,uint32_t y,uint32_t active){
 return {contact,action,x,y,512,uint16_t(T::EventPressureValid|(contact==0?T::EventPrimary:0)),active,++stamp};
}
static void Need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static void Begin(I::Session& touch,uint64_t id,T::Configuration configuration){touch.Begin(id,configuration);}
static void Finish(I::Session& touch){touch.End();Need(touch.ReleaseClean()&&touch.ActiveMask()==0,"unclean touch release");}

int wmain(int argc,wchar_t** argv){
 if(argc!=3){fwprintf(stderr,L"usage: TouchReleaseHarness <new-directory> <iterations>\n");return 2;}
 try{
  const std::filesystem::path directory(argv[1]);const unsigned iterations=std::stoul(argv[2]);
  Need(iterations>0&&iterations<=100,"iterations out of bounds");
  if(std::filesystem::exists(directory))throw std::runtime_error("refusing existing evidence directory");
  std::filesystem::create_directories(directory);FILE* result=nullptr;if(_wfopen_s(&result,(directory/L"touch-harness.csv").c_str(),L"w")||!result)throw std::runtime_error("harness output");
  fprintf(result,"scenario,iteration,active_after,release_clean,injections\n");fflush(result);
  I::Session touch(I::Mode::Inject,directory.wstring());const auto configuration=touch.CurrentConfiguration();uint64_t id=100;
  auto run=[&](const char* name,auto body){for(unsigned iteration=0;iteration<iterations;++iteration){body(++id,configuration);fprintf(result,"%s,%u,%u,%u,%llu\n",name,iteration,touch.ActiveMask(),touch.ReleaseClean()?1:0,touch.Injected());fflush(result);}};
  run("A_down_up",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));touch.Accept(current,Event(0,T::Action::Up,32768,32768,0));Finish(touch);});
  run("B_down_move_up",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));touch.Accept(current,Event(0,T::Action::Move,40000,30000,1));touch.Tick();touch.Accept(current,Event(0,T::Action::Up,40000,30000,0));Finish(touch);});
  run("C_two_sequential_up",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,6000,56000,1));touch.Accept(current,Event(1,T::Action::Down,59000,56000,3));touch.Accept(current,Event(0,T::Action::Up,6000,56000,2));touch.Accept(current,Event(1,T::Action::Up,59000,56000,0));Finish(touch);});
  run("D_two_teardown",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,6000,56000,1));touch.Accept(current,Event(1,T::Action::Down,59000,56000,3));Finish(touch);});
  run("E_transport_disconnect",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));Finish(touch);});
  run("F_session_replacement",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));Begin(touch,current+100000,c);Finish(touch);});
  run("G_host_shutdown",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));Finish(touch);});
  run("H_move_disconnect",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,18000,30000,1));for(unsigned move=0;move<8;++move){touch.Accept(current,Event(0,T::Action::Move,18000+move*3000,30000+move*1000,1));touch.Tick();}Finish(touch);});
  run("I_rapid_down_disconnect",[&](uint64_t current,const T::Configuration& c){Begin(touch,current,c);touch.Accept(current,Event(0,T::Action::Down,32768,32768,1));Finish(touch);});
  fclose(result);printf("PASS %u iterations; deterministic production touch-release scenarios completed\n",iterations);return 0;
 }catch(const std::exception& error){fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
