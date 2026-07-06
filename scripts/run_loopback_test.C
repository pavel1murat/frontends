///////////////////////////////////////////////////////////////////////////////
// prerequisite: one DTC in the CFO timing chain placed into a loopback mode
// the script runs on a node with the CFO in it
// prepare the DTC:
// dtc0 = dtc_init("mu2e-trk-10",0);
// dtc0->InitReadout(0,1)              // make sure that all links specified in a call to run_loopback_test are locked and enabled
// dtc0->fDtc->EnableCFOLoopback();
//
// after that - run loopback test to measure delays.
// so far, it is not obvious what the emasured numbers represent
// this script to be run on a CFO node (currently - mu2e-calo-13)
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// need a mask here, because some links may be disabled/lot locked
//-----------------------------------------------------------------------------
int run_loopback_test(int TimeChain, const char* Node, int PcieAddr, int LinkMask, int NEvents = 100, int PrintLevel = 0) {

  auto cfo_i = cfo_init("mu2e-calo-13",1);
  CFO* cfo   = cfo_i->fCfo;

  const bool clockMarkerWasOn = cfo->ReadEmbeddedClockMarkerEnable();
  if (clockMarkerWasOn) cfo->DisableEmbeddedClockMarker();

  CFO_Link_ID  time_chain_link = CFO_Link_ID(TimeChain);
  
  cfo->EnableLink (time_chain_link,DTC_LinkEnableMode(true,true),1);
  // cfo->DisableBeamOffMode(time_chain_link);
  // cfo->SoftReset();
  
  gStyle->SetStatW(0.4);     // to make the statbox readable

  TCanvas* c = new TCanvas("c","c",1500,900);
  c->Divide(3,2);

  TH1F* h_delay[6];
  const double delay_unit(5./8.);
  int n_exp = 3;
  const float wait_time = 1000. * 10.;  // 10 ms

  for (int lnk=0; lnk<6; lnk++) {
    int enabled = (LinkMask >> 4*lnk) & 0x1;
    if (not enabled) continue;

    h_delay[lnk] = new TH1F(Form("h_%i",lnk),Form("%s:DTC:%i link %i",Node,PcieAddr,lnk),10000,0,10000);
    TH1F* hist = h_delay[lnk];
  
    for(int i=0; i<NEvents; ++i) {

      cfo->SetCableDelayMeasureExponentialCount(n_exp); // N(markers) = (1 << n_exp)
      usleep(wait_time);
      cfo->RunCableDelayLoopbackTest();
      usleep(wait_time);

      bool done(false);
      int retries(0);
      int val = -1;
      while (retries < 3) {
        val = cfo->ReadCableDelayMeasurement(time_chain_link, lnk, done);
        if (done) break;
        usleep(wait_time);
        std::cout << std::format(" -------- link:{} event:{:4d} retries:{} val:{:4d} done:{}\n",lnk,i,retries,val,done);
        retries++;
      }
      // histogram

      float delay = val*delay_unit;

      hist->Fill(delay);
    }
    
    c->cd(lnk+1);
//-----------------------------------------------------------------------------
// the histogram is done, draw +/- 50 bins from the maximum
//-----------------------------------------------------------------------------
    int max_bin = -1;
    float ymax(-1.);
    for (int ib=0; ib<10000; ib++) {
      float y = hist->GetBinContent(ib+1);
      if (y > ymax) {
        ymax = y;
        max_bin=ib;
      }
    }

    hist->GetXaxis()->SetRangeUser(max_bin-50,max_bin+50);
    hist->Draw();
  }

  c->Draw();
  
  if (PrintLevel > 0) {
    cfo_i->PrintStatus();
  }

  if (clockMarkerWasOn) cfo->EnableEmbeddedClockMarker();
  return 0;
}


