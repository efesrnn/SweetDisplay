// Diagnostic acceptance only. Never changes the driver/video transport ABI.
#pragma once
#include <cstdint>
namespace SweetDisplay::Content {
enum Class:uint32_t {A=1,B=2,C=3,D=4,E=5};
inline Class Classify(bool integrity,bool pattern,bool regression,bool referenceMatch){
 if(!integrity)return D;
 if(pattern&&!regression)return A;
 if(referenceMatch)return pattern&&regression?C:B;
 return E;
}
}
