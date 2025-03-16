#ifndef __utils_OdbInterface_hh__
#define __utils_OdbInterface_hh__

#include "midas.h"

//-----------------------------------------------------------------------------
class OdbInterface {
public:
  HNDLE                _hDB;            // cached
  static OdbInterface* _instance;       // 

private:

  OdbInterface(HNDLE h_DB = 0);

public:

  static OdbInterface*  Instance      (HNDLE h_DB);

  std::string GetOutputDir();

  int         GetEnabled              (HNDLE h_Conf); //
  int         GetStatus               (HNDLE h_Conf); //

  HNDLE       GetActiveRunConfigHandle();
  std::string GetRunConfigName        (HNDLE h_RunConf);
  HNDLE       GetRunConfigHandle      (std::string& RunConf);

  //  int         GetArtdaqPartition      ();

  int         GetPartitionID          (HNDLE h_RunConf); //
  int         GetEventMode            (HNDLE h_RunConf); //
  int         GetOnSpill              (HNDLE h_RunConf);

                                        // 'Host' - short, w/o the domain name
  HNDLE       GetDaqConfigHandle      (HNDLE h_RunConf);
  int         GetHostConfHandle       (HNDLE h_RunConf, const std::string& Host);
  int         GetFrontendConfHandle   (HNDLE h_RunConf, const std::string& Host);
  HNDLE       GetHostArtdaqConfHandle (HNDLE h_RunConf, const std::string& Host);

  int         GetSkipDtcInit          (HNDLE h_RunConf);


//-----------------------------------------------------------------------------
// CFO supports up to 8 timing chains, so dimension of NDTCs array should be at least 8
// NDTCs[i] : number of DTCs in i-th timing chain
//-----------------------------------------------------------------------------
  int         GetNDTCs                (HNDLE h_DB, HNDLE h_CFO, int* NDTCs);
  
  HNDLE       GetCfoConfHandle        (HNDLE h_RunConf);

  uint64_t    GetNEvents              (HNDLE h_DB, HNDLE h_DTC);
  int         GetEWLength             (HNDLE h_CFO);
  //  uint64_t    GetFirstEWTag       (HNDLE h_DB, HNDLE h_DTC);
  // temporarily, back to INT
  int         GetFirstEWTag           (HNDLE h_CFO);

  int         GetCfoEnabled           (HNDLE h_CFO);
  int         GetCfoEmulatedMode      (HNDLE h_CFO);
  int         GetCfoEventMode         (HNDLE h_CFO);
  int         GetCfoNEventsPerTrain   (HNDLE h_CFO);
  std::string GetCfoRunPlanDir        ();
  std::string GetCfoRunPlan           (HNDLE h_CFO);

  int         GetCfoSleepTime         (HNDLE h_CFO);
   
  int         GetDtcEmulatesCfo   (HNDLE h_Card); // card: DTC
  int         GetDtcID            (HNDLE h_Card); // card: DTC
  int         GetJAMode           (HNDLE h_Card); // card: CFO or DTC
  int         GetLinkMask         (HNDLE h_Card); // card: CFO or DTC
  int         GetDtcMacAddrByte   (HNDLE h_Card); // card: DTC
  int         GetDtcPcieAddress   (HNDLE h_Card); // card: DTC
  int         GetDtcSampleEdgeMode(HNDLE h_Card); // 0:force rising; 1: force falling; 2:auto
  int         GetIsCrv            (HNDLE h_Card);

  int         GetPcieAddress      (HNDLE h_DB, HNDLE h_Card); // card: either CFO or DTC
//-----------------------------------------------------------------------------
// tracker ROC readout mode = 0: var length length ROC patterns
//                            1: digi patterns
//                            2: fixed length ROC patterns
// hDetConf : detector configuration handle 
//-----------------------------------------------------------------------------
  int         GetRocReadoutMode   (HNDLE hDetConf); 

  std::string GetPrivateSubnet    (HNDLE h_RunConf);
  std::string GetPublicSubnet     (HNDLE h_RunConf);

  std::string GetTfmHostName      (HNDLE h_RunConf);
//-----------------------------------------------------------------------------
// "generic" accessors, first - "new style", then "old style"
// slowly getting rid of the old style, need the transition to be transparent
//-----------------------------------------------------------------------------
  HNDLE       GetHandle           (HNDLE hConf, const char* Key);
  int         GetInteger          (HNDLE hConf, const char* Key, int*      Data);
  int         GetUInt32           (HNDLE hConf, const char* Key, uint32_t* Data);
  std::string GetString           (HNDLE hConf, const char* Key);

  HNDLE       GetHandle           (HNDLE hDB, HNDLE hConf, const char* Key);
  int         GetInteger          (HNDLE hDB, HNDLE hConf, const char* Key, int* Data);
  std::string GetString           (HNDLE hDB, HNDLE hConf, const char* Key);

  // set status of a given configuration element
  
  int         SetStatus           (HNDLE hElement, int Status);
//-----------------------------------------------------------------------------
// an element corresponding to a ROC (in case of the tracker - a panel) stores
// expect hLink subdirectory to have the following keys:
// 'RocDeviceSerial', 'RocDesignInfo', 'RocGitCommit'
//-----------------------------------------------------------------------------
  int         SetRocID            (HNDLE hLink, std::string& RocID     );
  int         SetRocDesignInfo    (HNDLE hLink, std::string& DesignInfo);
  int         SetRocFwGitCommit   (HNDLE hLink, std::string& Commit    );
};

#endif
