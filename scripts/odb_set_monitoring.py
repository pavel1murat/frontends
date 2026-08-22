#!/usr/bin/env python
#------------------------------------------------------------------------------
# PM: load channel map for active run configuration to ODB
# all channels except those specified in the file named
#
#  $MU2E_DAQ_DIR/config/tracker/station_00/{thresholds_dir}/channel_map.json
#
# the value of {thresholds_dir} is defined by
#
#    odb_path=/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration/ThresholdsDir
#
# are presumed good. The present file format:
#[
#    {"name": "MN224", "channel": 0,"status":0},
#    {"name": "MN224", "channel":90,"status":0}
#]
# call signature:
#
#  python v001/frontends/scripts/set_monitoring.py --rates on
#------------------------------------------------------------------------------
import  midas,TRACE
import  midas.client
import  os, psycopg2, socket, sys, getopt, time, copy
import  argparse
import  json
import  logging;
from    pathlib                  import Path
from    datetime                 import datetime
from    zoneinfo                 import ZoneInfo

import TRACE ; TRACE_NAME='odb_channel_masks'

logger = logging.getLogger('odb_set_monitoring')
#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
class OdbChannelMasks:
    
    def __init__(self):
        TRACE.INFO(f'-- START:')
        self.slot_lo    = None;
        self.slot_hi    = None;
        self.dtc_lo     = 0;
        self.dtc_hi     = 1;
        self.link_lo    = 0;
        self.link_hi    = 5;
        self.threshold  = None;
        self.channels   = [];
        self.diag_level = 0;
        self.op         = None;
        
        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        TRACE.INFO(f'node:{node} experiment_name:{experiment_name}',TRACE_NAME)
#        self.client     = midas.client.MidasClient("odb_set_monitoring",'localhost',experiment_name,None)
        self.client     = midas.client.MidasClient("odb_set_monitoring",None,experiment_name,None)
        TRACE.INFO(f'-- END:')
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.diag_level): return 0;
        now = time.strftime('%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
        message = now+' [ TestXmlrpc::'+Name+' ] '+Message
        print(message)

#------------------------------------------------------------------------------
    def parse_parameters(self):
        TRACE.INFO(f'-- START:')
        name = 'parse_parameters'
        
#        logger.info('Starting')
#        logger.info(f'sys.argv:{sys.argv}')
        
        parser = argparse.ArgumentParser()

        parser.add_argument("--diag_level"      , type=int, default=0,    help="Path to the configuration file")
        parser.add_argument('--rates'           , default=None,           help="turn on/off rates monitoring")
        parser.add_argument('--node'            , default="All",          help="select one node, if needed")
        
        parser.add_argument('--disk'            , default=None,           help="turn on/off disk monitoring")
        parser.add_argument('--dtc'             , default=None,           help="turn on/off rates monitoring")
        parser.add_argument('--spi'             , default=None,           help="turn on/off rates monitoring")
        parser.add_argument('--artdaq'          , default=None,           help="turn on/off rates monitoring")
        parser.add_argument('--dry-run'         , action='store_true',    help="dry run if set")

        args = parser.parse_args()

        logger.info(f'self.diag_level = {args.diag_level}'   )

        # some parameters need parsing

        TRACE.INFO(f'-- END: args:{args}')
#        logger.info(f'------------------------------------- Done')
        return args
    
#------------------------------------------------------------------------------
# update channel mask with only specified channels enabled
#---v--------------------------------------------------------------------------
    def set_monitoring(self,args):
        TRACE.INFO('-- START',TRACE_NAME)

        logger.info("Initializing : set")

        odb_path_nodes  = "/Mu2e/ActiveRunConfiguration/DAQ/Nodes"
        hkey            = self.client._odb_get_hkey(odb_path_nodes)
        child_keys   = self.client._odb_enum_key(hkey)
        # print(f'child_keys:{child_keys}')
        
        for (child_hkey, child_key) in child_keys:
            print(f'--------------  child_key:{child_key}')
            node_name     = f'{child_key.name.decode("utf-8")}'

            # manage only mu2e-trk-xx nodes
            if (not 'mu2e-trk' in node_name):
                continue
            
            if (args.node != 'All') and (node_name != args.node):
                continue
            
            odb_path_mon = f'{odb_path_nodes}/{node_name}/Monitor';
            mon_record   = self.client.odb_get(odb_path_mon)

            # up
            if (args.rates):
                mon_record['Rates'] = args.rates;

            if (args.disk):
                mon_record['Disk']  = args.disk;

            if (args.dtc):
                mon_record['DTC']  = args.dtc;

            if (args.artdaq):
                mon_record['Artdaq']  = args.artdaq;

            if (not args.dry_run):
                self.client.odb_set(odb_path_mon,mon_record)


            # print monitoring settings
            print(f'mon Disk        : {mon_record["Disk"]}')
            print(f'mon DTC         : {mon_record["DTC"]}')
            print(f'mon SPI         : {mon_record["SPI"]}')
            print(f'mon Artdaq      : {mon_record["Artdaq"]}')
            print(f'mon RocRegisters: {mon_record["RocRegisters"]}')
            print(f'mon Rates       : {mon_record["Rates"]}')

                
        
        
#        if (not args.dry_run):
#            TRACE.INFO('writing updated channel channel mask to ODB')
#            self.client.odb_set(panel_odb_path+f'/ch_mask',chmask)

        self.client.disconnect()
        TRACE.INFO('-- END',TRACE_NAME)
        return 0

#------------------------------------------------------------------------------
if __name__ == "__main__":
    TRACE.INFO(f'-- starting',TRACE_NAME)
    
    x = OdbChannelMasks();
    
    args = x.parse_parameters();
#------------------------------------------------------------------------------
# figure out what to do
#------------------------------------------------------------------------------
    x.set_monitoring(args)

    sys.exit(0)
