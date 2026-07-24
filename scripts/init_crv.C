//-----------------------------------------------------------------------------
// CRV: ROCs 0 and 3
// begin run initialization : write 1 to ROC reg 2
//-----------------------------------------------------------------------------
int init_crv() {
  int rc(0);
  auto dtc1 = dtc_init("mu2e-crv-01",1);
  dtc1->fDtc->ReleaseAllBuffers(DTC_DMA_Engine(1));
  
  dtc1->fDtc->WriteRegister_(0x4949,0x9114);
  
  dtc1->fDtc->WriteROCRegister(DTC_Link_ID(0),2,1,false,1000);
  dtc1->fDtc->WriteROCRegister(DTC_Link_ID(3),2,1,false,1000);

  auto x = dtc1->fDtc->ReadROCRegister(DTC_Link_ID(0),0x35,1000);

  std::cout << "x:" << x << std::endl;

  return rc;
}


