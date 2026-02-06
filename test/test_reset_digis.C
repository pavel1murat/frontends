//

//-----------------------------------------------------------------------------
int test_reset_digis(trkdaq:DtcInterface* dtc_i, int Link) {
  // reset digis : according to Vadim, write 0 , then 1 to regirster 103
  dtc_i->fDtc->WriteROCRegister(DTCLib::DTC_Link_ID(Link),103,0x0,false,1000);
  dtc_i->fDtc->WriteROCRegister(DTCLib::DTC_Link_ID(Link),103,0x1,false,1000);
  return 0;
}
