// Bounded per-thread diagnostic journal. No disk I/O or locks in Record().
#pragma once
#include "../protocol/TransportSender.h"
#include <memory>
namespace SweetDisplay::Visual {
struct StageRow {
    uint64_t ns, session, frame, generation, value;
    const char* name;
    long result;
};
class StageTrace {
    static constexpr size_t Capacity=262144;
    std::unique_ptr<StageRow[]> rows{new StageRow[Capacity]{}};
    size_t count=0;
    uint64_t lost=0;
public:
    uint64_t session=0,frame=0,generation=0;
    size_t Count() const {return count;}
    uint64_t Lost() const {return lost;}
    static constexpr size_t Bound=Capacity;
    void Record(const char* name,uint64_t value=0,long result=0) noexcept {
        if(count==Capacity){++lost;return;}
        rows[count++]={Transport::Now(),session,frame,generation,value,name,result};
    }
    void Save(const std::wstring& path) {
        Transport::EvidenceFile file(path);
        fprintf(file.value,"ns,session,frame_id,generation,event,value,result\n");
        for(size_t i=0;i<count;++i){const auto& r=rows[i];
            fprintf(file.value,"%llu,%llu,%llu,%llu,%s,%llu,%ld\n",r.ns,r.session,r.frame,r.generation,r.name,r.value,r.result);}
        fprintf(file.value,"%llu,0,0,0,trace_lost,%llu,0\n",Transport::Now(),lost);file.Flush();
        Protocol::Require(lost==0,"bounded timing trace exhausted; measurements incomplete");
    }
};
inline thread_local StageTrace* stageTrace=nullptr;
inline void Stage(const char* name,uint64_t value=0,long result=0){if(stageTrace)stageTrace->Record(name,value,result);}
inline void Identity(uint64_t session,uint64_t frame,uint64_t generation){if(stageTrace){stageTrace->session=session;stageTrace->frame=frame;stageTrace->generation=generation;}}
}
