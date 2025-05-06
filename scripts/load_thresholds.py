#!/usr/bin/env python

import  midas,TRACE
import  midas.client
import  os, psycopg2
import  json
import  logging;

logger = logging.getLogger('midas')

#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
def load_thresholds(station):
    
    logger.info("Initializing : test2_set_thresholds")

    client = midas.client.MidasClient("load_thresholds", "mu2edaq09", "test_025", None)
#------------------------------------------------------------------------------
# ODB address: 
#------------------------------------------------------------------------------
    station_path = f'/Mu2e/ActiveRunConfiguration/Tracker/Station_{station:02d}' #/Plane_0{ipl}/Panel_0{link[panel_name]}'
    print(station_path);
    # loop over the planes
    for plane in range(0,2):
        for panel in range(0,6):
            panel_odb_path = station_path+f'/Plane_{plane:02d}/Panel_{panel:02d}';
            panel_name = client.odb_get(panel_odb_path+'/Name');
            print (f'panel_name:{panel_name}');
#------------------------------------------------------------------------------
# read input file
#------------------------------------------------------------------------------
            fn = f'config/tracker/station_00/thresholds/{panel_name}.json';
            print (f'-------------- opening file:{fn}')
            with open(fn, 'r') as file:
                data = json.load(file)
                n = len(data)
                for d in data:
                    print(d)
            # {'channel': 84, 'type': 'hv', 'threshold': 414, 'gain': 370}
                    ich = d['channel']
                    client.odb_set(panel_odb_path+f'/gain_{d["type"]}[{ich}]',d["gain"]);
                    client.odb_set(panel_odb_path+f'/threshold_{d["type"]}[{ich}]',d["threshold"]);
# channel map is read in separately
                    # client.odb_set(panel_odb_path+f'/ch_mask[{ich}]',1)
#    print(data)

  
       

#------------------------------------------------------------------------------
if __name__ == "__main__":

#    test1()
    load_thresholds(station=0)
