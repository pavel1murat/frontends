#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------

import  midas,TRACE ; TRACE_NAME = 'test_odb' ;
import  midas.client
import  os, psycopg2
import  json
import  logging;

logger = logging.getLogger('midas')

def test1():
    logger.info("Initializing %s" % "get_next_run")
    client = midas.client.MidasClient("get_next_run", "mu2edaq22", "test_025", None)
   
    daq_nodes_hkey = client._odb_get_hkey("/Mu2e/ActiveRunConfiguration/DAQ/Nodes")
    nodes          = client._odb_enum_key(daq_nodes_hkey);
    
    TRACE.TRACE(TRACE.TLVL_INFO,f'nodes:{nodes}')
        
    for node in nodes: 
        print('--',node, '...', node[1], "...",node[1].name,"...",node[1].type)

        k_node = "/Mu2e/ActiveRunConfiguration/DAQ/Nodes/"+node[1].name.decode('ascii');
        node_enabled = client.odb_get(k_node+'/Enabled');
        print(f'node.name:{node[1].name.decode("ascii")} node_enabled:{node_enabled}');

        if (node_enabled):
            k_artdaq = k_node+'/Artdaq';
            artdaq_enabled = client.odb_get(k_artdaq+'/Enabled');
            print(f'  artdaq_enabled:{artdaq_enabled}');

            if (artdaq_enabled):
#------------------------------------------------------------------------------
# loop over the artdaq processes
#------------------------------------------------------------------------------
                artdaq_hkey = client._odb_get_hkey(k_artdaq)
                processes   = client._odb_enum_key(daq_nodes_hkey);
                print(f'  processes:{processes}')

def test_003():
    logger.info("Initializing %s" % "test_003")
    client = midas.client.MidasClient("test_003", None, "tracker", None)
   
    val   = client.odb_get("/Mu2e/ActiveRunConfiguration/DAQ/FclTemplates/br_demo/daq")
    TRACE.INFO(f'val:{val} len(val):{len(val)} type:{type(val)} dict:{isinstance(val,dict)}')

    val   = client.odb_get("/Mu2e/ActiveRunConfiguration/DAQ/FclTemplates/br_demo/daq/fragment_receiver/fragment_ids")
    TRACE.INFO(f'val:{val} len(val):{len(val)}')


def test_004():
    logger.info("Initializing %s" % "test_004")
    client = midas.client.MidasClient("test_odb", None, None, None)
   
    daq   = client.odb_get("/Mu2e/ActiveRunConfiguration/DAQ")
    TRACE.INFO(f'start_5ns:{daq["digitization_start_5ns"]} stop_5ns:{daq["digitization_stop_5ns"]}',TRACE_NAME)


def test_005():
    logger.info("Initializing %s" % "test_005")
    client = midas.client.MidasClient("test_odb", None, None, None)
   
    tt   = client.odb_get("/Mu2e/ActiveRunConfiguration/Trigger/tt_tracker_mc2_v002")

    print(tt);

    for k0 in tt.keys():
        print(f'key: {k0} is_dict:{isinstance(tt[k0],dict)}')
        if (k0 == 'physics'):
            s0 = f'art.physics'
            # loop over 
            for k1 in tt[k0].keys():
                if (k1 == 'filters'):
                    s0 += '.filters'
                    # loop over modules
                    filters = tt[k0][k1]
                    for k2 in filters.keys():
                        print(f'k2:{k2}')
                        s0 += f'.{k2}'
                        module = filters[k2]
                        # next go module parameters, to begin with, consider the simplest case
                        for k3 in module.keys():
                            s0 += f'.{k3} : {module[k3]}'
                            print(s0)
    
    # TRACE.INFO(f'start_5ns:{daq["digitization_start_5ns"]} stop_5ns:{daq["digitization_stop_5ns"]}',TRACE_NAME)


#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
def test2_set_thresholds(plane,panel_name):
    
    logger.info("Initializing : test2_set_thresholds")

    client = midas.client.MidasClient("set_thresholds", "mu2edaq09", "test_025", None)

    link          = {}
    
    link['MN261'] = 0;
    link['MN248'] = 1;
    link['MN224'] = 2;
    link['MN262'] = 3;
    link['MN273'] = 4;
    link['MN276'] = 5;

    link['MN253'] = 0;
    link['MN101'] = 1;
    link['MN219'] = 2;
    link['MN213'] = 3;
    link['MN235'] = 4;
    link['MN247'] = 5;
#------------------------------------------------------------------------------
# ODB address:
#------------------------------------------------------------------------------
    ipl = 0;
    if (plane == 21): ipl = 1;

    odb_path = f'/Mu2e/RunConfigurations/train_station/Tracker/Station_00/Plane_0{ipl}/Panel_0{link[panel_name]}'
    print(odb_path);
#------------------------------------------------------------------------------
# read input file
#------------------------------------------------------------------------------
    fn = f'config/tracker/station_00/plane_{plane}/{panel_name}.json';
    with open(fn, 'r') as file:
        data = json.load(file)
        n = len(data)
        for d in data:
            print(d)
            # {'channel': 84, 'type': 'hv', 'threshold': 414, 'gain': 370}
            ich = d['channel']
            # client.odb_set(odb_path+f'/on_off[{ich}]',1)
            if (d['type'] == 'cal'):
                client.odb_set(odb_path+f'/gain_{d["type"]}[{ich}]',d["gain"]);
                client.odb_set(odb_path+f'/threshold_{d["type"]}[{ich}]',d["threshold"]);
            

#    print(data)

  
       

#------------------------------------------------------------------------------
if __name__ == "__main__":

#    test1()
#    test2_set_thresholds(25,'MN261')
#    test_004();
    test_005()
