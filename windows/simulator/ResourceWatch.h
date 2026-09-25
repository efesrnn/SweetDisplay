// Diagnostic observer only. PSS work and file I/O never run on receiver/UI threads.
// Snapshots are asynchronous: requested/observed times must not be conflated.
#pragma once
#include "../protocol/TransportSender.h"
#include <processsnapshot.h>
#include <psapi.h>
#include <map>
#include <string>
namespace SweetDisplay::Visual {
class ResourceWatch {
    Transport::EvidenceFile checkpoints,types;
    std::atomic<uint64_t> requested{0};
    std::atomic<bool> stop{false};
    std::thread worker;
    std::exception_ptr error;
    static constexpr const char* Names[]={"startup","protocol_connection","renderer_created","decoder_created",
        "first_submission","first_decoded","first_successful_present","decoder_reset","decoder_released","renderer_cleanup","shutdown"};
    void Sample(uint64_t reasons,bool snapshot){
        auto began=Transport::Now();DWORD before=0,after=0;
        Protocol::Require(GetProcessHandleCount(GetCurrentProcess(),&before)!=0,"observer handle count");
        PROCESS_MEMORY_COUNTERS_EX pm{};pm.cb=sizeof(pm);
        Protocol::Require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pm),sizeof(pm))!=0,"observer memory");
        if(snapshot){HPSS captured=nullptr;HPSSWALK marker=nullptr;
            auto flags=PSS_CAPTURE_FLAGS(PSS_CAPTURE_HANDLES|PSS_CAPTURE_HANDLE_NAME_INFORMATION|PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION);
            Protocol::Require(PssCaptureSnapshot(GetCurrentProcess(),flags,0,&captured)==ERROR_SUCCESS,"observer PSS capture");
            try{Protocol::Require(PssWalkMarkerCreate(nullptr,&marker)==ERROR_SUCCESS,"observer PSS marker");
                std::map<std::wstring,unsigned> histogram;
                for(;;){PSS_HANDLE_ENTRY entry{};auto status=PssWalkSnapshot(captured,PSS_WALK_HANDLES,marker,&entry,sizeof(entry));if(status==ERROR_NO_MORE_ITEMS)break;
                    Protocol::Require(status==ERROR_SUCCESS,"observer PSS walk");
                    ++histogram[entry.TypeName?std::wstring(entry.TypeName,entry.TypeNameLength/sizeof(wchar_t)):L"unknown"];}
                for(const auto& item:histogram)fprintf(types.value,"%llu,%llu,%ls,%u\n",began,reasons,item.first.c_str(),item.second);
            }catch(...){if(marker)PssWalkMarkerFree(marker);PssFreeSnapshot(GetCurrentProcess(),captured);throw;}
            PssWalkMarkerFree(marker);PssFreeSnapshot(GetCurrentProcess(),captured);types.Flush();
        }
        Protocol::Require(GetProcessHandleCount(GetCurrentProcess(),&after)!=0,"observer handle count after snapshot");
        fprintf(checkpoints.value,"%llu,%llu,%llu,%lu,%lu,%zu,%zu,%u\n",began,Transport::Now(),reasons,before,after,pm.PrivateUsage,pm.WorkingSetSize,unsigned(snapshot));checkpoints.Flush();
    }
    void Run(){try{uint64_t last=0,lastSnapshot=0;unsigned samples=0;
        for(;;){auto reason=requested.exchange(0);auto now=Transport::Now();bool ending=stop.load();
            if(reason||ending||now-last>=1000000000ULL){Protocol::Require(++samples<=1024,"bounded resource observations");bool snapshot=reason||ending||now-lastSnapshot>=5000000000ULL;
                Sample(reason,snapshot);last=now;if(snapshot)lastSnapshot=now;}
            if(ending)return;Sleep(10);
        }
    }catch(...){error=std::current_exception();}}
public:
    explicit ResourceWatch(const std::wstring& dir):checkpoints(dir+L"resource-checkpoints.csv"),types(dir+L"resource-types.csv"){
        fprintf(checkpoints.value,"observed_begin_ns,observed_end_ns,reason_mask,handles_before,handles_after,private_bytes,working_set,pss\n");
        fprintf(types.value,"observed_ns,reason_mask,type,count\n");
        Transport::EvidenceFile legend(dir+L"resource-reasons.csv");fprintf(legend.value,"bit,reason\n");for(unsigned i=0;i<sizeof(Names)/sizeof(Names[0]);++i)fprintf(legend.value,"%u,%s\n",i,Names[i]);legend.Flush();
        Request(0);worker=std::thread([this]{Run();});
    }
    ~ResourceWatch(){stop=true;if(worker.joinable())worker.join();}
    void Request(unsigned bit){requested.fetch_or(uint64_t(1)<<bit);}
    void Finish(){Request(10);stop=true;if(worker.joinable())worker.join();if(error)std::rethrow_exception(error);}
};
}
