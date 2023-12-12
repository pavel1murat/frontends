#ifndef __trk_utils_c__
#define __trk_utils_c__

#define __CLING__ 1

#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTC.h"
#include "srcs/mu2e_pcie_utils/dtcInterfaceLib/DTCSoftwareCFO.h"


int gSleepTimeDTC      =  1000;  // [us]
int gSleepTimeROC      =  2000;  // [us]
int gSleepTimeROCReset =  4000;  // [us]

//-----------------------------------------------------------------------------
void monica_digi_clear(DTCLib::DTC* dtc, int Link = 0) {
//-----------------------------------------------------------------------------
//  Monica's digi_clear
//  this will proceed in 3 steps each for HV and CAL DIGIs:
// 1) pass TWI address and data toTWI controller (fiber is enabled by default)
// 2) write TWI INIT high
// 3) write TWI INIT low
//-----------------------------------------------------------------------------
// rocUtil write_register -l $LINK -a 28 -w 16 > /dev/null
  auto link = DTCLib::DTC_Link_ID(Link);

  dtc->WriteROCRegister(link,28,0x10,false,1000); // 

  // Writing 0 & 1 to  address=16 for HV DIGIs ??? 
  // rocUtil write_register -l $LINK -a 27 -w  0 > /dev/null # write 0 
  // rocUtil write_register -l $LINK -a 26 -w  1 > /dev/null ## toggle INIT 
  // rocUtil write_register -l $LINK -a 26 -w  0 > /dev/null
  dtc->WriteROCRegister(link,27,0x00,false,1000); // 
  dtc->WriteROCRegister(link,26,0x01,false,1000); // toggle INIT 
  dtc->WriteROCRegister(link,26,0x00,false,1000); // 


  // rocUtil write_register -l $LINK -a 27 -w  1 > /dev/null # write 1  
  // rocUtil write_register -l $LINK -a 26 -w  1 > /dev/null # toggle INIT
  // rocUtil write_register -l $LINK -a 26 -w  0 > /dev/null
  dtc->WriteROCRegister(link,27,0x01,false,1000); // 
  dtc->WriteROCRegister(link,26,0x01,false,1000); // 
  dtc->WriteROCRegister(link,26,0x00,false,1000); // 

  // echo "Writing 0 & 1 to  address=16 for CAL DIGIs"
  // rocUtil write_register -l $LINK -a 25 -w 16 > /dev/null
  dtc->WriteROCRegister(link,25,0x10,false,1000); // 

// rocUtil write_register -l $LINK -a 24 -w  0 > /dev/null # write 0
// rocUtil write_register -l $LINK -a 23 -w  1 > /dev/null # toggle INIT
// rocUtil write_register -l $LINK -a 23 -w  0 > /dev/null
  dtc->WriteROCRegister(link,24,0x00,false,1000); // 
  dtc->WriteROCRegister(link,23,0x01,false,1000); // 
  dtc->WriteROCRegister(link,23,0x00,false,1000); // 

// rocUtil write_register -l $LINK -a 24 -w  1 > /dev/null # write 1
// rocUtil write_register -l $LINK -a 23 -w  1 > /dev/null # toggle INIT
// rocUtil write_register -l $LINK -a 23 -w  0 > /dev/null
  dtc->WriteROCRegister(link,24,0x01,false,1000); // 
  dtc->WriteROCRegister(link,23,0x01,false,1000); // 
  dtc->WriteROCRegister(link,23,0x00,false,1000); // 
}

//-----------------------------------------------------------------------------
// this implements Monica's bash DTC_Reset
//-----------------------------------------------------------------------------
void monica_dtc_reset(DTCLib::DTC* Dtc) {

  mu2edev* dev = Dtc->GetDevice();

  dev->write_register(0x9100,100,0x80000000);
  dev->write_register(0x9100,100,0x00008000);
  dev->write_register(0x9118,100,0xffff00ff);
  dev->write_register(0x9118,100,0x00000000);
}

//-----------------------------------------------------------------------------
void monica_var_link_config(DTCLib::DTC* dtc, int Link = 0) {
  mu2edev* dev = dtc->GetDevice();

  auto link = DTCLib::DTC_Link_ID(Link);

  dev->write_register(0x91a8,100,0);
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeDTC));

  dtc->WriteROCRegister(link,14,     1,false,1000);              // reset ROC
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROCReset));

  dtc->WriteROCRegister(link, 8,0x030f,false,1000);             // configure ROC to read all 4 lanes
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROC));

  // added register for selecting kind of data to report in DTC status bits
  // Use with pattern data. Set to zero, ie STATUS=0x55, when taking DIGI data 
  // rocUtil -a 30  -w 0  -l $LINK write_register > /dev/null

  dtc->WriteROCRegister(link,30,0x0000,false,1000);        // configure ROC to read all 4 lanes
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROC));

  // echo "Setting packet format version to 1"
  // rocUtil -a 29  -w 1  -l $LINK write_register > /dev/null

  dtc->WriteROCRegister(link,29,0x0001,false,1000);        // configure ROC to read all 4 lanes
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeROC));
}

//-----------------------------------------------------------------------------
// print data starting from ptr , 'nw' 2-byte words
// printout: 16 bytes per line, grouped in 2-byte words
//-----------------------------------------------------------------------------
void print_buffer(const void* ptr, int nw) {

  // int     nw  = nbytes/2;
  ushort* p16 = (ushort*) ptr;
  int     n   = 0;

  for (int i=0; i<nw; i++) {
    if (n == 0) printf(" 0x%08x: ",i*2);

    ushort  word = p16[i];
    printf("0x%04x ",word);

    n   += 1;
    if (n == 8) {
      printf("\n");
      n = 0;
    }
  }

  if (n != 0) printf("\n");
}

//-----------------------------------------------------------------------------
void print_dtc_registers(DTCLib::DTC* Dtc) {
  uint32_t res; 
  int      rc;

  mu2edev* dev = Dtc->GetDevice();

  rc = dev->read_register(0x9100,100,&res); printf("0x9100: DTC status       : 0x%08x\n",res); // expect: 0x40808404
  rc = dev->read_register(0x9138,100,&res); printf("0x9138: SERDES Reset Done: 0x%08x\n",res); // expect: 0xbfbfbfbf
  rc = dev->read_register(0x91a8,100,&res); printf("0x9158: time window      : 0x%08x\n",res); // expect: 
  rc = dev->read_register(0x91c8,100,&res); printf("0x91c8: debug packet type: 0x%08x\n",res); // expect: 0x00000000
}


//-----------------------------------------------------------------------------
void print_roc_registers(DTCLib::DTC* Dtc, DTCLib::DTC_Link_ID RocID, const char* Header) {
  uint16_t  roc_reg[100];
  printf("---------------------- %s print_roc_registers\n",Header);

  // roc_reg[11] = Dtc->ReadROCRegister(RocID,11,100);
  // printf("roc_reg[11] = 0x%04x\n",roc_reg[11]);
  // roc_reg[13] = Dtc->ReadROCRegister(RocID,13,100);
  // printf("roc_reg[13] = 0x%04x\n",roc_reg[13]);
  // roc_reg[14] = Dtc->ReadROCRegister(RocID,14,100);
  // printf("roc_reg[14] = 0x%04x\n",roc_reg[14]);

  printf("---------------------- END print_roc_registers\n");
}

//-----------------------------------------------------------------------------
mu2e_databuff_t* readDTCBuffer(mu2edev* device, bool& readSuccess, bool& timeout, size_t& sts) {
  mu2e_databuff_t* buffer;
  auto tmo_ms = 1500;
  readSuccess = false;

  sts = device->read_data(DTC_DMA_Engine_DAQ, reinterpret_cast<void**>(&buffer), tmo_ms);
  std::this_thread::sleep_for(std::chrono::microseconds(gSleepTimeDTC));
  
  if (sts > 0) {
    readSuccess   = true;
    void* readPtr = &buffer[0];
    uint16_t bufSize = static_cast<uint16_t>(*static_cast<uint64_t*>(readPtr));
    readPtr = static_cast<uint8_t*>(readPtr) + 8;
    
    timeout = false;
    if (sts > sizeof(DTCLib::DTC_EventHeader) + sizeof(DTCLib::DTC_SubEventHeader) + 8) {
      // Check for 'dead' or 'cafe' in first packet
      readPtr = static_cast<uint8_t*>(readPtr) + sizeof(DTCLib::DTC_EventHeader) + sizeof(DTCLib::DTC_SubEventHeader);
      std::vector<size_t> wordsToCheck{ 1, 2, 3, 7, 8 };
      for (auto& word : wordsToCheck) 	{
				uint16_t* wordPtr = static_cast<uint16_t*>(readPtr) + (word - 1);
				if ((*wordPtr == 0xcafe) or (*wordPtr == 0xdead)) {
					printf(" Buffer Timeout detected! word=%5lu data: 0x%04x\n",word, *wordPtr);
					DTCLib::Utilities::PrintBuffer(readPtr, 16, 0, /*TLVL_TRACE*/4 + 3);
					timeout = true;
					break;
				}
      }
    }
  }
  return buffer;
}

#endif
