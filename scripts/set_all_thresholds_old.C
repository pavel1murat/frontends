
int set_all_thresholds(int Plane) {

  int pcie_addr;

  if (Plane == 25) {
    pcie_addr = 0;

    dtc_control_roc_set_thresholds(0,"config/tracker/station_00/thresholds/MN261.json",pcie_addr) ;
    //    dtc_i->ControlRoc_MeasureThresholds(0);
    dtc_control_roc_set_thresholds(1,"config/tracker/station_00/thresholds/MN248.json",pcie_addr);
    //    dtc_i->ControlRoc_MeasureThresholds(1);
    dtc_control_roc_set_thresholds(2,"config/tracker/station_00/thresholds/MN224.json",pcie_addr);
    //    dtc_i->ControlRoc_MeasureThresholds(2);
    dtc_control_roc_set_thresholds(3,"config/tracker/station_00/thresholds/MN262.json",pcie_addr);
    //dtc_i->ControlRoc_MeasureThresholds(3);
    dtc_control_roc_set_thresholds(4,"config/tracker/station_00/thresholds/MN273.json",pcie_addr);
    //    dtc_i->ControlRoc_MeasureThresholds(4);
    dtc_control_roc_set_thresholds(5,"config/tracker/station_00/thresholds/MN276.json",pcie_addr);
    //    dtc_i->ControlRoc_MeasureThresholds(5);
  }
  else if (Plane == 21) {
    pcie_addr = 1;
  
    dtc_control_roc_set_thresholds(0,"config/tracker/station_00/thresholds/MN253.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(0);
    dtc_control_roc_set_thresholds(1,"config/tracker/station_00/thresholds/MN101.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(1);
    dtc_control_roc_set_thresholds(2,"config/tracker/station_00/thresholds/MN219.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(2);
    dtc_control_roc_set_thresholds(3,"config/tracker/station_00/thresholds/MN213.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(3);
    dtc_control_roc_set_thresholds(4,"config/tracker/station_00/thresholds/MN235.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(4);
    dtc_control_roc_set_thresholds(5,"config/tracker/station_00/thresholds/MN247.json",pcie_addr);
    // dtc_i->ControlRoc_MeasureThresholds(5);

  }

  dtc_control_roc_read(-1,0,0,0,0,0,1,0xffffffff,0xffffffff,0xffffffff,pcie_addr);
  
  return 0;
}
