//-----------------------------------------------------------------------------
// 2024-12-27 P.Murat: compared to a year ago, there are changes
//                     in the output format, parsing will have to change
//-----------------------------------------------------------------------------
#ifndef __ArtdaqMetrics_hh__
#define __ArtdaqMetrics_hh__

#include <initializer_list>

/*
br01 statistics:
  Fragments read: 20202 fragments generated at 336.642 getNext calls/sec, fragment rate = 336.675 fragments/sec, monitor window = 60.0045 sec, min::max read size = 1::2 fragments Average times per fragment:  elapsed time = 0.00297052 sec
  Fragment output statistics: 20202 fragments sent at 336.675 fragments/sec, effective data rate = 3.06434 MB/sec, monitor window = 60.0045 sec, min::max event size = 3.8147e-05::0.0118866 MB
  Input wait time = 0.00298349 s/fragment, buffer wait time = 2.98028e-06 s/fragment, request wait time = 0.00298268 s/fragment, output wait time = 1.93648e-06 s/fragment
fragment_id:0 nfragments:0 nbytes:0 max_nf:1000 max_nb:1048576000
 */

struct BrMetrics_t {
  int    nf_read;                       // [ 0]
  float  getNextRate;                   // [ 1] per second;
  float  fr_rate;                       // [ 2]
  float  time_window;                   // [ 3] sec
  int    min_nf;                        // [ 4] per getNext call
  int    max_nf;                        // [ 5] per getNext call
  float  elapsed_time;                  // [ 6] sec, end of line 1
  int    nf_sent;                       // [ 7] 
  float  output_fr;                     // [ 8] output, frag/sec
  float  output_dr;                     // [ 9] output, MB/sec
  float  output_mw;                     // [10] sec
  float  min_evt_size;                  // [11] MB
  float  max_evt_size;                  // [12] MB , end of line 2
  float  inp_wait_time;                 // [13] input wait time, sec
  float  buf_wait_time;                 // [14] buffering wait time, sec
  float  req_wait_time;                 // [15] request wait time, sec
  float  out_wait_time;                 // [16] output wait time, per fragment; end of laine 3
  int    fr_id [5];                     // [17-21] fragment IDs
  int    nf_shm[5];                     // [22-26] n(fragments) with fragID[i] currently in the SHM buffer
  int    nb_shm[5];                     // [27-31] n(bytes)     for  fragID[i] currently in the SHM buffer
};

enum { NBrDataWords = sizeof(BrMetrics_t)/sizeof(int) };

extern std::initializer_list<const char*> BrVarName;

//-----------------------------------------------------------------------------
// event builder
//-----------------------------------------------------------------------------
/*
eb01 statistics:
  Event statistics: 20000 events released at 333.309 events/sec, effective data rate = 3.89229 MB/sec, monitor window = 60.0043 sec, min::max event size = 0.00606537::0.0197067 MB
  Average time per event:  elapsed time = 0.00300022 sec
  Fragment statistics: 20000 fragments received at 333.309 fragments/sec, effective data rate = 3.87958 MB/sec, monitor window = 60.0043 sec, min::max fragment size = 0.00602722::0.0196686 MB
  Event counts: Run -- 60400 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. 
shm_nbb :250:1153440:250:0:0:0
*/
struct EbMetrics_t {
                                      // line 0 doesn't have any numbers in it

  int    nev_read;                    // [ 0] n(events) within the time window
  int    evt_rate;                    // [ 1] end of line
  float  data_rate;                   // [ 2] MB/sec
  float  time_window;                  // [ 3]
  float  min_evt_size;                // [ 4] MB
  float  max_evt_size;                // [ 5] MB , end of line 1
  float  etime;                       // [ 6] elapsed time, sec, end of line 2

  int    nfr_read;                    // [ 7] N(fragments) within the time window
  float  nfr_rate;                    // [ 8] 
  float  fr_data_rate;                // [ 9] fragment data rate, MB/sec
  int    min_fr_size;                 // [10] per getNext call
  int    max_fr_size;                 // [11] per getNext call , end of line 3

  int    nev_tot_rn;                  // [12]
  int    nev_bad_rn;                  // [13]
  int    nev_tot_sr;                  // [14]
  int    nev_bad_sr;                  // [15] end of line 4

  int    nbuf_shm_tot;                // [16] total number of allocated SHM buffers
  int    nbytes_shm_tot;              // [17] total number of allocated SHM buffers
  int    nbuf_shm_empty;              // [18] n empty (free) buffers
  int    nbuf_shm_write;              // [19] N buffers being written to 
  int    nbuf_shm_full;               // [20] N full buffers 
  int    nbuf_shm_read;               // [21] N buffers being read from
};

enum { NEbDataWords = sizeof(EbMetrics_t)/sizeof(int) };
extern std::initializer_list<const char*> EbVarName;
/* ------------------------------------------------------------------------------
 the DL metrics has he same format as the EB metrics
 -----------------------------------------------
dl01 statistics:
  Event statistics: 17052 events released at 284.181 events/sec, effective data rate = 6.51481 MB/sec, monitor window = 60.0041 sec, min::max event size = 0.00696564::0.0305862 MB
  Average time per event:  elapsed time = 0.00351889 sec
  Fragment statistics: 17052 fragments received at 284.181 fragments/sec, effective data rate = 6.50397 MB/sec, monitor window = 60.0041 sec, min::max fragment size = 0.00692749::0.0305481 MB
  Event counts: Run -- 1082246 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. 
shm_nbb :250:2306872:250:0:0:0
 */
struct DlMetrics_t {
                                      // line 0 doesn't have any numbers in it

  int    nev_read;                    // [ 0] n(events) within the time window
  int    evt_rate;                    // [ 1] end of line
  float  data_rate;                   // [ 2] MB/sec
  float  mon_window;                  // [ 3]
  float  min_evt_size;                // [ 4] MB
  float  max_evt_size;                // [ 5] MB , end of line 1
  float  etime;                       // [ 6] elapsed time, sec, end of line 2

  int    nfr_read;                    // [ 7] N(fragments) within the time window
  float  nfr_per_sec;                 // [ 8] 
  float  fr_dr;                       // [ 9] fragment data rate, MB/sec
  int    min_fr_size;                 // [10] per getNext call
  int    max_fr_size;                 // [11] per getNext call , end of line 3

  int    nev_tot_rn;                  // [12]
  int    nev_bad_rn;                  // [13]
  int    nev_tot_sr;                  // [14]
  int    nev_bad_sr;                  // [15] end of line 4

  int    nbuf_shm_tot;                // [16] total number of allocated SHM buffers
  int    nbytes_shm_tot;              // [17] total number of allocated SHM buffers
  int    nbuf_shm_empty;              // [18] n empty (free) buffers
  int    nbuf_shm_write;              // [19] N buffers being written to 
  int    nbuf_shm_full;               // [20] N full buffers 
  int    nbuf_shm_read;               // [21] N buffers being read from
};

enum { NDlDataWords = sizeof(DlMetrics_t)/sizeof(int) };
extern std::initializer_list<const char*> DlVarName;
//-----------------------------------------------------------------------------
// placeholder for dispatcher
//-----------------------------------------------------------------------------
struct DsMetrics_t {
};

enum { NDsDataWords = sizeof(DsMetrics_t)/sizeof(int) };
extern std::initializer_list<const char*> DsVarName;

#endif
