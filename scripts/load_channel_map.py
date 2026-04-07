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
#                     load_channel_map.py --slot=10 --thr=15
# or
#                     load_channel_map.py --slot=10 --channel=1
# or
#                     load_channel_map.py --save --slot=10 --fn=slot_10.json
#                     load_channel_map.py --load --slot=10 --thr=15mV --fn=slot_10.json
#------------------------------------------------------------------------------
import  midas,TRACE
import  midas.client
import  os, psycopg2, socket, sys, getopt, time
import  json
import  logging;

logger = logging.getLogger('midas')
#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
class LoadChannelMap:
    
    def __init__(self):
        self.slot       = None;
        self.threshold  = None;
        self.channels   = [];
        self.diag_level = 0;
        self.op         = None;
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.diag_level): return 0;
        now = time.strftime('%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
        message = now+' [ TestXmlrpc::'+Name+' ] '+Message
        print(message)

#------------------------------------------------------------------------------
    def parse_parameters(self):
        name = 'parse_parameters'
        
        self.Print(name,2,'Starting')
        self.Print(name,2, '%s' % sys.argv)

        try:
            optlist, args = getopt.getopt(sys.argv[1:], '',
                     ['diag_level=', 'enable=', 'load', 'reset', 'save', 'slot=', 'thr=' ] )
 
        except getopt.GetoptError:
            self.Print(name,0,'%s' % sys.argv)
            self.Print(name,0,'Errors arguments did not parse')
            return 110

        for key, val in optlist:

            # print('key,val = ',key,val)

            if (key == '--diag_level'):
                self.diag_level = int(val)
            elif   (key == '--enable'):
                self.op = 'enable';
                for ch in val.split(','):
                    self.channels.append(int(ch));
            elif   (key == '--load'):
                self.op = 'load';
            elif   (key == '--reset'):
                self.op = 'reset';
            elif   (key == '--save'):
                self.op = 'save';
            elif   (key == '--slot'):
                self.slot = int(val)
            elif   (key in ('-t', '--thr')):
                self.threshold = int(val)

        self.Print(name,1,'channels  = %s' % self.channels)
        self.Print(name,1,'op        = %s' % self.op)
        self.Print(name,1,'slot      = %s' % self.slot)
        self.Print(name,1,'threshold = %s' % self.threshold)
        self.Print(name,1,'diag_level= %s' % self.diag_level)
        self.Print(name,1,'------------------------------------- Done')
        
        return 0

#------------------------------------------------------------------------------
    def extract_values(self,data, panel_name):
        # print(f'--- panel_name:{panel_name}')
        results = []
        for v in data:
            # print (f'v:{v}')
            if (v['name'] == panel_name):
                results.append(v)
                # print('-- appended:',v);
        return results;

#------------------------------------------------------------------------------
# different good channel maps for different thresholds
# location: $thresholds_dir/channel_map.json
#---v--------------------------------------------------------------------------
    def load_channel_map(self,slot):

        logger.info("Initializing : load_channel_map")

        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        client          = midas.client.MidasClient("load_channel_map",node,experiment_name,None)

        ro_cfg_path     = '/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration';
        thresholds_dir  = client.odb_get(ro_cfg_path+'/thresholds_dir');

        slot_path = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        print(slot_path);

        fn = f'config/tracker/slot_{slot:02d}/thresholds-{self.threshold}-mV/channel_map.json';
        print (f'-------------- opening file:{fn}')
        
        with open(fn, 'r') as file:
#------------------------------------------------------------------------------
# print data
#------------------------------------------------------------------------------
            data = json.load(file)
            n = len(data)
            for d in data:
                 print(d)
#------------------------------------------------------------------------------
# /Plane_0{ipl}/Panel_0{link[panel_name]}'
# loop over the planes
# print('--- looping over the panels');
#------------------------------------------------------------------------------
            for plane in range(0,2):
                for panel in range(0,6):
                    panel_odb_path = slot_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
                    panel_name     = client.odb_get(panel_odb_path+'/Name');
                    print(f'-- panel_name:{panel_name} ODB path:{panel_odb_path}');
            
                    # initialize the channel map, all channels good
            
                    chmask = [];
                    for ch in range(0,96):
                        chmask.append(1)
                        
                            #print(i,ip,pmask)
                    #    return;
                    
                    res = self.extract_values(data,panel_name)
                    print(f'res:{res}')
            
                    for r in res:
                        ich         = r['channel']
                        status      = r['status' ] ; # normally - zero
                        chmask[ich] = status;
            
                    # print('chmask:',chmask);
                    # select this panel in the data
            
                    for ich in range(0,96):
                        client.odb_set(panel_odb_path+f'/ch_mask[{ich}]',chmask[ich])
                
#            client.odb_set(panel_odb_path+f'/gain_{d["type"]}[{ich}]',d["gain"]);
#            client.odb_set(panel_odb_path+f'/threshold_{d["type"]}[{ich}]',d["threshold"]);
#

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
    def save_channel_map(self,slot):

        logger.info("Initializing : save_channel_map")

        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        client          = midas.client.MidasClient("save_channel_map",node,experiment_name,None)

        ro_cfg_path     = '/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration';

        slot_path       = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        print(slot_path);

        fn = f'disabled_channel_map_slot_{slot:02d}.json'
        print (f'-------------- opening file:{fn}')
        
        with open(fn, 'w') as file:
#------------------------------------------------------------------------------
# print data
#------------------------------------------------------------------------------
            r               = [];
#------------------------------------------------------------------------------
# /Plane_0{ipl}/Panel_0{link[panel_name]}'
# loop over the planes
# print('--- looping over the panels');
#------------------------------------------------------------------------------
            for plane in range(0,2):
                for panel in range(0,6):
                    panel_odb_path = slot_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
                    panel_name     = client.odb_get(panel_odb_path+'/Name');
                    print(f'-- panel_name:{panel_name} ODB path:{panel_odb_path}');

                    chmask = client.odb_get(panel_odb_path+f'/ch_mask')
                   
                    for ch in range(0,96):
                        if (chmask[ch] == 0):
                            d = {}
                            d['name'   ] = panel_name;
                            d['channel'] = ch;
                            d['status' ] = 0
                            r.append(d);
#------------------------------------------------------------------------------
# now the list of masked off channels is defined
#---------------v--------------------------------------------------------------
            json.dump(r,file,indent=4);
#------------------------------------------------------------------------------
# done
#------------------------------------------------------------------------------
        return 0;

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
    def reset_channel_map(self,slot):

        logger.info("Initializing : reset_channel_map")

        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        client          = midas.client.MidasClient("reset_channel_map",node,experiment_name,None)

        ro_cfg_path     = '/Mu2e/ActiveRunConfiguration/Tracker/ReadoutConfiguration';

        slot_path       = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        print(slot_path);

#------------------------------------------------------------------------------
# /Plane_0{ipl}/Panel_0{link[panel_name]}'
# loop over the planes
# print('--- looping over the panels');
#------------------------------------------------------------------------------

        for plane in range(0,2):
            for panel in range(0,6):
                panel_odb_path = slot_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
                panel_name     = client.odb_get(panel_odb_path+'/Name');
                print(f'-- panel_name:{panel_name} ODB path:{panel_odb_path}');

                zero = [0]*100;
                client.odb_set(panel_odb_path+f'/ch_mask',zero)
                    
#------------------------------------------------------------------------------
# done
#------------------------------------------------------------------------------
        return 0;

#------------------------------------------------------------------------------
# enable readout of only one channel for all panels of a station in a fiven slot
#---v--------------------------------------------------------------------------
    def enable(self,slot,enabled_channels):

        logger.info("Initializing : enable_one_channel")

        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        client          = midas.client.MidasClient("load_channel_map",node,experiment_name,None)

        slot_path = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{slot:02d}'
        print(slot_path);

        for plane in range(0,2):
            for panel in range(0,6):
                panel_odb_path = slot_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
                panel_name     = client.odb_get(panel_odb_path+'/Name');
                print(f'-- panel_name:{panel_name} ODB path:{panel_odb_path}');
            
                # initialize the channel map, all channels good
            
                chmask = [0]*96;
                for ch in enabled_channels:
                    chmask[ch] = 1;

                client.odb_set(panel_odb_path+f'/ch_mask',chmask)

#------------------------------------------------------------------------------
if __name__ == "__main__":
    x = LoadChannelMap();
    x.parse_parameters();
#------------------------------------------------------------------------------
# figure out what to do
#------------------------------------------------------------------------------
    if (x.op == 'load'):
        if (x.slot != None):
            x.load_channel_map(x.slot)

    elif (x.op == 'save'):
        if (x.slot != None):
            # save bad channels into a file to be used later
            rc = x.save_channel_map(x.slot);

    elif (x.op == 'enable'):
        if (x.slot != None):
            # reset all channel flags to zero
            rc = x.enable(x.slot,x.channels);

    elif (x.op == 'reset'):
        if (x.slot != None):
            # reset all channel flags to zero
            rc = x.reset_channel_map(x.slot);
