// Diagnostic recorded-data harness, never live Display3/capacity acceptance.
#include "VisualPipeline.h"
#include <processsnapshot.h>
#include <psapi.h>
#include <fstream>
#include <string>
namespace V=SweetDisplay::Visual;namespace T=SweetDisplay::Transport;namespace P=SweetDisplay::Protocol;
static FILE* counts=nullptr;static FILE* types=nullptr;static FILE* stages=nullptr;static FILE* modules=nullptr;
static unsigned cycle=0,session=0;static uint64_t frame=0;static bool firstDecoded=false,firstPresent=false;
static void Sample(const char* stage,bool snapshot) {
    DWORD before=0,after=0;GetProcessHandleCount(GetCurrentProcess(),&before);PROCESS_MEMORY_COUNTERS_EX pm{};pm.cb=sizeof(pm);
    P::Require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pm),sizeof(pm))!=0,"probe memory");
    auto stamp=T::Now();
    if(snapshot){HPSS snap=nullptr;HPSSWALK marker=nullptr;
        DWORD error=PssCaptureSnapshot(GetCurrentProcess(),PSS_CAPTURE_FLAGS(PSS_CAPTURE_HANDLES|PSS_CAPTURE_HANDLE_NAME_INFORMATION|PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION),0,&snap);
        P::Require(error==ERROR_SUCCESS,"PSS capture");
        try{P::Require(PssWalkMarkerCreate(nullptr,&marker)==ERROR_SUCCESS,"PSS marker");std::map<std::wstring,unsigned> histogram;
            for(;;){PSS_HANDLE_ENTRY e{};error=PssWalkSnapshot(snap,PSS_WALK_HANDLES,marker,&e,sizeof(e));if(error==ERROR_NO_MORE_ITEMS)break;P::Require(error==ERROR_SUCCESS,"PSS walk");
                // No object names, tokens, addresses or file paths are retained.
                ++histogram[e.TypeName?std::wstring(e.TypeName,e.TypeNameLength/sizeof(wchar_t)):L"unknown"];
                if(e.ObjectType==PSS_OBJECT_TYPE_THREAD&&e.TypeSpecificInformation.Thread.Win32StartAddress){HMODULE module=nullptr;
                    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(e.TypeSpecificInformation.Thread.Win32StartAddress),&module)){
                        wchar_t path[32768]{};GetModuleFileNameW(module,path,32768);auto name=std::filesystem::path(path).filename().wstring();fprintf(modules,"%llu,%u,%u,%llu,%s,thread_start,%ls,%lu\n",stamp,cycle,session,frame,stage,name.c_str(),e.TypeSpecificInformation.Thread.ThreadId);}}}
            for(const auto& item:histogram)fprintf(types,"%llu,%u,%u,%llu,%s,%ls,%u\n",stamp,cycle,session,frame,stage,item.first.c_str(),item.second);
        }catch(...){if(marker)PssWalkMarkerFree(marker);PssFreeSnapshot(GetCurrentProcess(),snap);throw;}
        PssWalkMarkerFree(marker);PssFreeSnapshot(GetCurrentProcess(),snap);fflush(types);
        HMODULE list[512]{};DWORD bytes=0;P::Require(EnumProcessModules(GetCurrentProcess(),list,sizeof(list),&bytes)!=0&&bytes<=sizeof(list),"bounded module inventory");
        for(DWORD i=0;i<bytes/sizeof(HMODULE);++i){wchar_t path[32768]{};GetModuleFileNameW(list[i],path,32768);auto name=std::filesystem::path(path).filename().wstring();fprintf(modules,"%llu,%u,%u,%llu,%s,module,%ls,0\n",stamp,cycle,session,frame,stage,name.c_str());}fflush(modules);
    }
    GetProcessHandleCount(GetCurrentProcess(),&after);
    fprintf(counts,"%llu,%u,%u,%llu,%s,%lu,%lu,%zu,%zu,%lu\n",stamp,cycle,session,frame,stage,before,after,pm.PrivateUsage,pm.WorkingSetSize,GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS));fflush(counts);
}
static void Trace(const char* stage){fprintf(stages,"%llu,%u,%u,%llu,%s\n",T::Now(),cycle,session,frame,stage);
    if(!firstDecoded&&!strcmp(stage,"decoded_surface")){firstDecoded=true;Sample("first_decoded",true);}
    if(!firstPresent&&!strcmp(stage,"present_success")){firstPresent=true;Sample("first_successful_present",true);}}
int wmain(int argc,wchar_t** argv){if(argc!=4&&argc!=5)return 2;int code=0;bool idle=argc==5&&!wcscmp(argv[4],L"idle");
 try{bool async=argc==5&&!wcscmp(argv[4],L"async");P::Require(argc==4||idle||async,"probe mode");
    auto dir=std::filesystem::weakly_canonical(argv[1]);wchar_t exe[32768]{};P::Require(GetModuleFileNameW(nullptr,exe,32768)!=0,"probe path");
    auto root=std::filesystem::weakly_canonical(std::filesystem::path(exe).parent_path().parent_path().parent_path()/L"docs/evidence/private/phase3d").wstring()+L"\\";
    P::Require(!_wcsnicmp(dir.c_str(),root.c_str(),root.size())&&dir.wstring().size()>root.size(),"private probe evidence");
    auto requested=wcstoul(argv[3],nullptr,10);P::Require(requested>=120&&requested<=1700,"bounded fixture length");
    T::EvidenceFile c((dir/L"checkpoints.csv").wstring()),t((dir/L"handle-types.csv").wstring()),timing((dir/L"stages.csv").wstring()),m((dir/L"modules.csv").wstring());counts=c.value;types=t.value;stages=timing.value;modules=m.value;
    fprintf(modules,"ns,cycle,session,frame,stage,kind,module,thread_id\n");
    fprintf(counts,"ns,cycle,session,frame,stage,handles_before,handles_after,private_bytes,working_set,gdi_objects\n");fprintf(types,"ns,cycle,session,frame,stage,type,count\n");fprintf(stages,"ns,cycle,session,frame,stage\n");
    Sample("cold",true);Sample("pss_repeat",true);V::Check(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"COM");Sample("com_initialized",true);V::Check(MFStartup(MF_VERSION),"MF startup");Sample("mf_initialized",true);
    for(cycle=0;cycle<(idle?1u:3u);++cycle){
        {std::unique_ptr<T::EvidenceFile> sparse;if(async)sparse=std::make_unique<T::EvidenceFile>((dir/(L"sparse-"+std::to_wstring(cycle)+L".csv")).wstring());
            V::GpuWindow gpu(sparse.get());Sample("renderer_initialized",true);std::mutex gate;std::atomic<uint64_t> epoch{0};
            if(idle){DWORD prior=0;GetProcessHandleCount(GetCurrentProcess(),&prior);for(frame=0;frame<900;++frame){P::Require(gpu.Pump(),"idle probe window closed");if(frame%20==0){DWORD current=0;GetProcessHandleCount(GetCurrentProcess(),&current);Sample("idle_periodic",current!=prior);prior=current;}Sleep(50);}Sample("idle_end",true);}
            else
            {T::EvidenceFile ledger((dir/(L"rendered-"+std::to_wstring(cycle)+L".csv")).wstring());V::Decoder decoder(gpu,ledger,gate,epoch);Sample("manager_initialized",true);
                for(session=0;session<3;++session){frame=0;firstDecoded=firstPresent=false;++epoch;Sample("before_decoder",true);V::checkpoint=Trace;decoder.Reset(session+1);Sample("decoder_initialized",true);
                    std::ifstream file(std::filesystem::path(argv[2]),std::ios::binary);P::Require(bool(file),"fixture open");unsigned nframes=cycle==0&&session==0?unsigned(requested):120;
                    for(unsigned i=0;i<nframes;++i){gpu.PollSparse();P::Require(gpu.Pump(),"owner closed resource probe");uint8_t head[12]{};file.read(reinterpret_cast<char*>(head),12);P::Require(file.gcount()==12,"fixture header");auto length=P::U32(head);P::Require(length&&length<=P::MaxAu,"AU length");
                        V::Frame f;f.session=session+1;f.generation=epoch;f.sequence=i+3;f.received=T::Now();f.info.id=i+1;f.info.pts=P::U64(head+4);f.info.width=2400;f.info.height=1080;f.au.resize(length);file.read(reinterpret_cast<char*>(f.au.data()),length);P::Require(file.gcount()==length,"AU truncated");f.info.flags=P::NalFlags(f.au.data(),length);frame=i+1;decoder.Push(std::move(f));
                        if(frame%60==0)Sample("periodic",true);Sleep(17);
                    }
                    decoder.Drain();gpu.FinishSparse();Sample("steady_end",true);decoder.Reset(0);Sample("post_flush_release",true);V::checkpoint=nullptr;
                }
                Sample("before_decoder_destructor",true);
            }Sample("decoder_destroyed",true);
        }Sample("renderer_destroyed",true);Sleep(200);Sample("post_cleanup_wait",true);
    }
    V::checkpoint=nullptr;MFShutdown();Sample("mf_shutdown",true);CoUninitialize();Sample("com_shutdown",true);printf("PROBE_COMPLETE; independent trend and content review required\n");
 }catch(const V::Failure& e){fprintf(stderr,"PROBE_ERROR %s HRESULT=0x%08lX\n",e.what(),ULONG(e.hr));code=1;}catch(const std::exception& e){fprintf(stderr,"PROBE_ERROR %s\n",e.what());code=1;}V::checkpoint=nullptr;return code;
}
