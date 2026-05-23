#------------------------------------------------------------------------------
# validate active configuration
#------------------------------------------------------------------------------
import  midas,TRACE
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


#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
def validate_active_config():
    
    logger.info("Initializing : validate_active_config")

    client = midas.client.MidasClient("validate_active_config", "mu2e-dl-01", "tracker", None)
#------------------------------------------------------------------------------
# ODB address:
#------------------------------------------------------------------------------
    tracker_config_path = f'/Mu2e/RunConfigurations/tracker_mc2/Tracker' ## Station_00/Plane_0{ipl}/Panel_0{link[panel_name]}'
    first_station =  client.odb_get(tracker_config_path+'/FirstStation');
    last_station  =  client.odb_get(tracker_config_path+'/LastStation');

    TRACE.INFO(f'first_station:{first_station} last_station:{last_station}')

    for i in range(first_station,last_station+1):
        station_config_path = tracker_config_path+f'/Station_{i:02d}'
        print (f'station config path:{station_config_path}')
        for plane in range (0,2):
            plane_config_path   = station_config_path+f'/Plane_{plane:02d}'
            plane_name = client.odb_get(plane_config_path+'/Name');
            print (f'plane config path:{plane_config_path} plane_name:{plane_name}')
            
            for panel in range(0,6):
                panel_config_path = plane_config_path+f'/Panel_{panel:02d}'
                panel_name = client.odb_get(panel_config_path+'/Name');
                link       =  client.odb_get(panel_config_path+'/Link');
                print (f'plane config path:{plane_config_path} panel_name:{panel_name}')
                # now find the DTC this panel is connected to
                dtc_detector_element_path = plane_config_path+f'/DTC/Link{link}/DetectorElement'
                print(f'dtc_detector_element_path:{dtc_detector_element_path}')
                name2 = client.odb_get(dtc_detector_element_path+'/Name');
                print (f'plane config path:{plane_config_path} panel_name:{panel_name} name2:{name2}')
                if (name2 != panel_name):
                    TRACE.ERROR(f'panel_config_path:{panel_config_path} panel_name:{panel_name} name2:{name2}')

#------------------------------------------------------------------------------
if __name__ == "__main__":

#    test1()
#    test2_set_thresholds(25,'MN261')
    validate_active_config();
