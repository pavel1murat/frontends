#ifndef __frontends_trk_dcs_utils_hh__
#define __frontends_trk_dcs_utils_hh__

#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTC.h"
#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTCSoftwareCFO.h"

void monica_digi_clear     (DTCLib::DTC* dtc, int Link = 0);
void monica_dtc_reset      (DTCLib::DTC* Dtc);
void monica_var_link_config(DTCLib::DTC* dtc, int Link = 0);

                                        // assume nw 2-byte words

void print_buffer          (const void* ptr, int nw);
void print_dtc_registers   (DTCLib::DTC* Dtc);
void print_roc_registers   (DTCLib::DTC* Dtc, DTCLib::DTC_Link_ID RocID, const char* Header);

mu2e_databuff_t* readDTCBuffer(mu2edev* device, bool& readSuccess, bool& timeout, size_t& sts);


#endif
