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
class OdbChannelMasks:
    
    def __init__(self):
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
        self.client          = midas.client.MidasClient("odb_channel_map",node,experiment_name,None)
        
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
#        logger.info(f'sys.argv:{sys.argv}')
        
        parser = argparse.ArgumentParser()

        parser.add_argument("--diag_level"      , type=int, default=0,                 help="Path to the configuration file")
        parser.add_argument('--save'            , action='store_const', dest='op', const='save',  help="operation: enable,disable,load,save,set_mask")
        parser.add_argument('--load'            , action='store_const', dest='op', const='load',     help="operation: enable,disable,load,save,set_mask")
        parser.add_argument('--ena'             , action='store_const', dest='op', const='enable',     help="operation: enable,disable,load,save,set_mask")
        parser.add_argument('--dis'             , action='store_const', dest='op', const='disable',     help="operation: enable,disable,load,save,set_mask")
        parser.add_argument('--set'             , action='store_const', dest='op', const='set_mask',     help="operation: enable,disable,load,save,set_mask")
        parser.add_argument('--dtc'             , default=None,           help="DTCs, operation: enable,disable,load,save,set,update")
        parser.add_argument('--link'            , default=None,           help="links, operation: enable,disable,load,save,set,update")
        parser.add_argument('--mnid'            , default=None,           help="panel MnID")
        parser.add_argument('--node'            , default=None,           help="nodename")
        parser.add_argument('-c', '--channels'  , default=None,           help="list of channels")
        parser.add_argument('--dry-run'         , action='store_true',    help="dry run if set")
        parser.add_argument('-s', '--slots'     , default=None,           help="slots, i.e. 1-10")
        parser.add_argument('--thr'             , default=None,           help="threshold in mV")

        args = parser.parse_args()

        logger.info(f'self.diag_level = {args.diag_level}'   )

        # some parameters need parsing

        if args.slots:
            slots        = args.slots.split('-')
            self.slot_lo = int(slots[ 0])
            self.slot_hi = int(slots[-1])
        
        if args.node:
            # assume single slot, cant have both --slots and --node
            slot_odb_path = f'/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{args.node}/slot'
            self.slot_lo  = self.client.odb_get(slot_odb_path)
            self.slot_hi  = self.slot_lo
        
        if args.dtc:
            dtc         = args.dtc.split('-')
            self.dtc_lo = int(dtc[ 0])
            self.dtc_hi = int(dtc[-1])
        
        
        if args.link:
            link         = args.link.split('-')
            self.link_lo = int(link[ 0])
            self.link_hi = int(link[-1])
        
#        logger.info(f'------------------------------------- Done')
        return args
    

# 2026-07-03 PM#------------------------------------------------------------------------------
# 2026-07-03 PM    def extract_values(self,data, panel_name):
# 2026-07-03 PM        # print(f'--- panel_name:{panel_name}')
# 2026-07-03 PM        results = []
# 2026-07-03 PM        for v in data:
# 2026-07-03 PM            # print (f'v:{v}')
# 2026-07-03 PM            if (v['name'] == panel_name):
# 2026-07-03 PM                results.append(v)
# 2026-07-03 PM                # print('-- appended:',v);
# 2026-07-03 PM        return results;
# 2026-07-03 PM
# 2026-07-03 PM

#------------------------------------------------------------------------------

    def print_channel_mask(self, chmask):
        for i in range(0,6):
            for j in range(0,16):
                print (f'{chmask[6*i+j]}',end='');
            print(" ",end='')        
        print('')
        return

#------------------------------------------------------------------------------
# different good channel maps for different thresholds
# location: $thresholds_dir/{panel_name}_channel_mask.json
#---v--------------------------------------------------------------------------
    def load_channel_mask(self,slot,dtx,link,args):

        logger.info("Initializing : odb_channel_map")

        TRACE.INFO(f'-- START: slot:{slot} DTC:{dtc} link:{link}');

        ro_cfg_path     = '/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration';
        thresholds_dir  = self.client.odb_get(ro_cfg_path+'/thresholds_dir');

        slot_odb_path   = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        print(slot_odb_path);

        node            = self.client.odb_get(slot_odb_path+'/daq_server');
        panel_odb_path  = f'/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{node}/DTC{dtc}/Link{link}/DetectorElement'
        TRACE.INFO(f'panel_odb_path:{panel_odb_path}',TRACE_NAME);
        panel_name      = self.client.odb_get(panel_odb_path+'/Name')
        
        fn = f'config/tracker/slot_{slot:02d}/{thresholds_dir}/{panel_name}_channel_mask.json';
        print (f'-------------- opening file:{fn}')
        
        with open(fn, 'r') as file:
#------------------------------------------------------------------------------
# print data
#------------------------------------------------------------------------------
            data = json.load(file)
            n = len(data)
            TRACE.INFO(f'number of masked off channels:{n}',TRACE_NAME)
            for d in data:
                 TRACE.DEBUG(1,f'{d}')
#------------------------------------------------------------------------------
# /Plane_0{ipl}/Panel_0{link[panel_name]}'
# loop over the planes
# print('--- looping over the panels');
#------------------------------------------------------------------------------
            chmask = [1]*96;
            
            for r in data:
                ich         = r['channel']
                status      = r['status' ] ; # zeroes
                chmask[ich] = status;
    
            TRACE.INFO(f'slot:{slot} node:{node:13s} DTC:{dtc:02d} link:{link} panel_name:{panel_name} args.dry_run:{args.dry_run} channel mask to load:',TRACE_NAME)
            for i in range(0,6):
                for j in range(0,16):
                    print (f'{chmask[6*i+j]}',end='');
                print(" ",end='')
                    
            print('')
                
            if (not args.dry_run):
                TRACE.INFO(f'args.dry_run:{args.dry_run}, updating ODB',TRACE_NAME)
                self.client.odb_set(panel_odb_path+f'/ch_mask',chmask)
            else:
                TRACE.INFO(f'args.dry_run:{args.dry_run}, NOT updating ODB',TRACE_NAME)
                
        TRACE.INFO(f'-- END:')
        return
#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
    def save_channel_mask(self,slot,dtc,link,args):

        logger.info("Initializing : save_channel_mask")

        ro_cfg_path     = '/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration';

        slot_odb_path   = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        TRACE.INFO(f'slot_odb_path:{slot_odb_path}');

        # a list of masked off channels in .json format is written into a file named by the panel MNID
        # the file may contain an empty list

        node            = self.client.odb_get(slot_odb_path+'/daq_server');
        panel_odb_path  = f'/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{node}/DTC{dtc}/Link{link}/DetectorElement'
        TRACE.INFO(f'panel_odb_path:{panel_odb_path}',TRACE_NAME);
        panel_name      = self.client.odb_get(panel_odb_path+'/Name')
        
        fn    = os.environ.get("MU2E_DAQ_DIR")+f'/config/tracker/slot_{slot:02d}/thresholds-{args.thr}-mV/{panel_name}_channel_mask.json';
        fpath = Path(fn)

        TRACE.INFO(f'fn:{fn}',TRACE_NAME);
        if (fpath.exists()):
            # channel mask file exists, save it
            tstamp = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
            cmd    = f'cp {fn} {fn}.save.{tstamp}'
            os.system(cmd)
        
        print (f'-------------- saving channel masks to file:{fn}')
        
        with open(fn, 'w') as file:
#------------------------------------------------------------------------------
# /Plane_0{ipl}/Panel_0{link[panel_name]}'
# loop over the planes
# print('--- looping over the panels');
#------------------------------------------------------------------------------
           
            chmask = self.client.odb_get(panel_odb_path+f'/ch_mask')
            r = []
            for ch in range(0,96):
                if (chmask[ch] == 0):
                    d            = {}
                    d['name'   ] = panel_name;
                    d['channel'] = ch;
                    d['status' ] = 0
                    r.append(d)
                    TRACE.INFO(f'{d}');
                    
            file.write("[\n");
            for d in r:
                json.dump(d, file, separators=(",", ":"))
                if d != r[-1]:
                    file.write(',\n');
                else:
                    file.write('\n')
                    
            file.write("]\n");

        return 0;
#------------------------------------------------------------------------------
# done
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
    def get_panel_odb_path(self,slot,dtc,link):
        node_odb_path  = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}/daq_server'
        node           = self.client.odb_get(node_odb_path);
        panel_odb_path = f'/Mu2e/ActiveRunConfiguration/DAQ/Nodes/{node}/DTC{dtc}/Link{link}/DetectorElement'
        TRACE.DEBUG(1,f'slot:{slot} dtc:{dtc} link:{link} node_odb_path:{node_odb_path} node:{node} panel_odb_path:{panel_odb_path}',TRACE_NAME);

        return panel_odb_path;

#------------------------------------------------------------------------------
    def get_panel_name(self,slot,dtc,link):
        panel_odb_path  = self.get_panel_odb_path(slot,dtc,link);
        panel_name      = self.client.odb_get(panel_odb_path+'/Name')
        TRACE.DEBUG(1,f'slot:{slot} dtc:{dtc} link:{link} panel_odb_path:{panel_odb_path} panel_name:{panel_name}',TRACE_NAME);
        return panel_name
        
#------------------------------------------------------------------------------
# update channel mask with only specified channels enabled
#---v--------------------------------------------------------------------------
    def update_channel_mask(self,slot,dtc,link,args):

        logger.info("Initializing : set")

        panel_odb_path = self.get_panel_odb_path(slot,dtc,link)
        panel_name     = self.client.odb_get(panel_odb_path+'/Name');               
        chmask_in      = self.client.odb_get(panel_odb_path+f'/ch_mask')

        channels = []
        if args.channels:
            # could be a list of ranges '1,2,3,10-12,30-39'
            for r in args.channels.split(','):
                rs = r.split('-')
                if (len(rs) == 2):
                    for ch in range(int(rs[0]),int(rs[1]+1)):
                        channels.append(ch);
                else:
                    channels.append(int(r))

        TRACE.DEBUG(1,f'panel_name:{panel_name} channels:{channels}',TRACE_NAME);

        chmask = copy.deepcopy(chmask_in);
        
        if (args.op == 'enable'):
            for i in channels:
                chmask[i] = 1;
        elif (args.op == 'disable'):
            for i in channels:
                chmask[i] = 0;
        elif (args.op == 'set_mask'):
            for i in range(0,96):
                if (i in channels):
                    chmask[i] = 1
                else:
                    chmask[i] = 0;

        TRACE.INFO(f'slot:{slot} dtc:{dtc} link:{link} panel_name:{panel_name} initial and updated channel masks:',TRACE_NAME)
        self.print_channel_mask(chmask_in);
        self.print_channel_mask(chmask);
        
        if (not args.dry_run):
            TRACE.INFO('writing updated channel channel mask to ODB')
            self.client.odb_set(panel_odb_path+f'/ch_mask',chmask)

        return 0

#------------------------------------------------------------------------------
if __name__ == "__main__":
    x = OdbChannelMasks();
    
    args = x.parse_parameters();
#------------------------------------------------------------------------------
# figure out what to do
#------------------------------------------------------------------------------
    if (x.slot_lo == None):
        sys.exit(-1)

    if (args.op  == 'load'):
        for slot in range(x.slot_lo,x.slot_hi+1):
            for dtc in range(x.dtc_lo,x.dtc_hi+1):
                for link in range(x.link_lo,x.link_hi+1):
                    x.load_channel_mask(slot,dtc,link,args)

    elif (args.op == 'save'):
        for slot in range(x.slot_lo,x.slot_hi+1):
            for dtc in range(x.dtc_lo,x.dtc_hi+1):
                for link in range(x.link_lo,x.link_hi+1):
                    x.save_channel_mask(slot,dtc,link,args)

    elif (args.op == 'enable') or (args.op == 'disable') or (args.op == 'set_mask'):
        # update existing mask by enabling/disabling specified channels
        # 'enable' and 'disable' don't change the rest channels
        # 'set_mask' resets them to 0
        for slot in range(x.slot_lo,x.slot_hi+1):
            for dtc in range(x.dtc_lo,x.dtc_hi+1):
                for link in range(x.link_lo,x.link_hi+1):
                    x.update_channel_mask(slot,dtc,link,args)
        
    x.client.disconnect()
    sys.exit(0)
