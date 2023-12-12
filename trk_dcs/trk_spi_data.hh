#ifndef __trk_spi_data_hh__
#define __trk_spi_data_hh__

struct TrkSpiData_t {
  uint16_t  I3_3;
  uint16_t  I2_5;
  uint16_t  I1_8HV;
  uint16_t  IHV5_0;
  uint16_t  VDMBHV5_0;
  uint16_t  V1_8HV;
  uint16_t  V3_3HV;
  uint16_t  V2_5;
  uint16_t  A0;
  uint16_t  A1;
  uint16_t  A2;
  uint16_t  A3;
  uint16_t  I1_8CAL;
  uint16_t  I1_2;
  uint16_t  ICAL5_0;
  uint16_t  ADCSPARE;
  uint16_t  V3_3;
  uint16_t  VCAL5_0;
  uint16_t  V1_8CAL;
  uint16_t  V1_0;
  uint16_t  ROCPCBTEMP;
  uint16_t  HVPCBTEMP;
  uint16_t  CALPCBTEMP;
  uint16_t  RTD;
  uint16_t  ROC_RAIL_1V;
  uint16_t  ROC_RAIL_1_8V;
  uint16_t  ROC_RAIL_2_5V;
  uint16_t  ROC_TEMP;
  uint16_t  CAL_RAIL_1V;
  uint16_t  CAL_RAIL_1_8V;
  uint16_t  CAL_RAIL_2_5V;
  uint16_t  CAL_TEMP;
  uint16_t  HV_RAIL_1V;
  uint16_t  HV_RAIL_1_8V;
  uint16_t  HV_RAIL_2_5V;
  uint16_t  HV_TEMP;

  TrkSpiData_t*  spiData() { return (TrkSpiData_t*) &I3_3; }
  static int     nWords () { return sizeof(Data_t)/2;      }
};

#endif
