// Synthetic NAL-category fixture for transport tests only; NOT live video evidence.
#include "../TransportSender.h"
namespace P=SweetDisplay::Protocol;namespace T=SweetDisplay::Transport;
int wmain(int argc,wchar_t** argv){try{
 P::Require(argc==3||argc==4,"fixture output/port/optional slow-tail arguments");T::Sender sender(uint16_t(wcstoul(argv[2],nullptr,10)),argv[1]);
 LARGE_INTEGER frequency{},start{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
 for(uint64_t i=0;i<400;++i){bool idr=i%30==0;std::vector<uint8_t> b=idr?std::vector<uint8_t>{0,0,0,1,0x67,1,0,0,1,0x68,2,0,0,1,0x65,3}:std::vector<uint8_t>{0,0,1,0x41,3};LARGE_INTEGER now{};QueryPerformanceCounter(&now);sender.Consume({i+1,uint64_t(now.QuadPart),uint64_t(frequency.QuadPart),P::Nanoseconds(uint64_t(now.QuadPart-start.QuadPart),uint64_t(frequency.QuadPart))/100,2400,1080,idr?15u:0u,uint32_t(b.size()),b.data()});Sleep(argc==4&&i>=330?120:10);}
 sender.Finish();return 0;
 }catch(const std::exception& e){fprintf(stderr,"fixture ERROR: %s\n",e.what());return 1;}}
