// Transport acceptance is separate from source-content provenance.
#pragma once
#include "../shared/ContentClassification.h"
namespace SweetDisplay::TransportContent {
// PatternOracle::Regression == 4. All other reasons remain fatal if classified E.
// Caller has already checked binary cells, producer ledger and resource identity.
inline bool CounterOnly(bool enabled,uint32_t kind,uint32_t reason,bool validPattern){
 return enabled && kind==Content::E && reason==4 && validPattern;
}
}
