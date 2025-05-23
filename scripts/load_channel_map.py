#!/usr/bin/env python
#------------------------------------------------------------------------------
# PM: load channel map to ODB
# all channels except those specified in the file named
#
#  $MU2E_DAQ_DIR/config/tracker/station_00/channel_map/channel_map.json
#
# are presumed good. The present file format:
#[
#    {"name": "MN224", "channel": 0,"status":0},
#    {"name": "MN224", "channel":90,"status":0}
#]
#------------------------------------------------------------------------------
import  midas,TRACE
import  midas.client
import  os, psycopg2, socket
import  json
import  logging;

logger = logging.getLogger('midas')
#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
def extract_values(data, panel_name):
    # print(f'--- panel_name:{panel_name}')
    results = []
    for v in data:
        # print (f'v:{v}')
        if (v['name'] == panel_name):
            results.append(v)
            # print('-- appended:',v);
    return results;

def load_channel_map(station):
    
    logger.info("Initializing : load_channel_map")

    node            = socket.gethostname().split('.')[0];
    experiment_name = "test_025";
    client          = midas.client.MidasClient("load_channel_map",node,experiment_name,None)


    station_path = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{station:02d}'
    print(station_path);

    fn = f'config/tracker/station_00/channel_map/channel_map.json';
    print (f'-------------- opening file:{fn}')
    with open(fn, 'r') as file:
        data = json.load(file)
        n = len(data)
        for d in data:
             print(d)
            # {'channel': 84, 'type': 'hv', 'threshold': 414, 'gain': 370}
            # ich = d['channel']

            # return
        #------------------------------------------------------------------------------
        # /Plane_0{ipl}/Panel_0{link[panel_name]}'
        # loop over the planes
        # print('--- looping over the panels');
        for plane in range(0,2):
            for panel in range(0,6):
                panel_odb_path = station_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
                panel_name     = client.odb_get(panel_odb_path+'/Name');
                print(f'-- panel_name:{panel_name} ODB path:{panel_odb_path}');
    
                # initialize the channel map, all channels good
    
                chmask = [];
                for ch in range(0,96):
                    chmask.append(1)
                    
                        #print(i,ip,pmask)
                #    return;
                
                res = extract_values(data,panel_name)
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
# ODB address: 
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# read input file
#------------------------------------------------------------------------------
# channel map is read in separately
#    print(data)

  
       

#------------------------------------------------------------------------------
if __name__ == "__main__":

#    test1()
    load_channel_map(station=0)
