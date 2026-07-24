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
#                     odb_channel_masks.py --load --slot=0-18 --thr=15
# or
#                     odb_channel_masks.py --slot=10 --enable=44,91
# or
#                     odb_channel_masks.py --save --slot=10 --fn=slot_10.json
#                     odb_channel_masks.py --load --slot=10 --thr=15mV --fn=slot_10.json
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

import TRACE ; TRACE_NAME='artdaq'

logger = logging.getLogger('midas')
#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
class Test:
    
    def __init__(self):
        
        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        self.client          = midas.client.MidasClient("test",node,experiment_name,None)
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.diag_level): return 0;
        now = time.strftime('%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
        message = now+' [ TestXmlrpc::'+Name+' ] '+Message
        print(message)

#------------------------------------------------------------------------------
    def parse_parameters(self):
        name = 'parse_parameters'
        
#        logger.info('Starting')
#        logger.info(f'takes 0 positional arguments but 1 wassys.argv:{sys.argv}')
        
        parser = argparse.ArgumentParser()

        parser.add_argument("--diag_level"      , type=int, default=0,          help="Path to the configuration file")
        parser.add_argument('-t', '--test'       , type=int, default=0,          help="test number")

        args = parser.parse_args()

        logger.info(f'self.diag_level = {args.diag_level}'   )

        return args
    
#------------------------------------------------------------------------------
# update channel mask with only specified channels enabled
#---v--------------------------------------------------------------------------
# figure out active DTCs
#------------------------------------------------------------------------------
    def test1(self):

        logger.info("Initializing : set")

        nodes_odb_path = "/Mu2e/ActiveRunConfiguration/DAQ/Nodes"

        hkey = self.client._odb_get_hkey(nodes_odb_path)
        y = self.client._odb_get_key_from_hkey(hkey)
        print(f'y:{y}')

        print('-- after hkey')
        child_keys   = self.client._odb_enum_key(hkey)
        print(f'child_keys:{child_keys}')

        for (child_hkey, child_key) in child_keys:
            print(f'child_key:{child_key}')
            node_name = f'{child_key.name.decode("utf-8")}'
            print(f'name:{node_name}')

            node_path=nodes_odb_path+f'/{node_name}'
            h1key = self.client._odb_get_hkey(node_path)
            c1_keys   = self.client._odb_enum_key(h1key)

            # check if node is enabled
            
            # if it is 
            for (c1_hkey, c1_key) in c1_keys:
                print(f'c1_key:{c1_key}')
                c1_name = f'{c1_key.name.decode("utf-8")}'
                print(f'name:{c1_name}')

                if ('DTC' in c1_name) and (c1_name.index('DTC') == 0):
                    # check if DTC is  '/Enabled'
                    # if it is, got to equipment and check differnces...
                    print(f'DTC found: name:{c1_name}')
            
        print('-- after kk')
        
        return 0

#------------------------------------------------------------------------------
if __name__ == "__main__":
    x = Test();
    
    args = x.parse_parameters();
    print("-- parsed")
#------------------------------------------------------------------------------
# figure out what to do
#------------------------------------------------------------------------------
    if (args.test == 1):
        print('-- calling test1')
        x.test1()

    x.client.disconnect()
    sys.exit(0)
