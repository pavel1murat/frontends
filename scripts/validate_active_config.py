#!/usr/bin/env python
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
    
    TRACE.INFO(f'nodes:{nodes}')
        
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

    client   = midas.client.MidasClient("validate_active_config", "mu2e-dl-01", "tracker", None)
    n_errors = 0
#------------------------------------------------------------------------------
# ODB address:
#------------------------------------------------------------------------------
    tracker_config_path = f'/Mu2e/ActiveRunConfiguration/Tracker' ## Station_00/Plane_0{ipl}/Panel_0{link[panel_name]}'
    first_station =  client.odb_get(tracker_config_path+'/FirstStation');
    last_station  =  client.odb_get(tracker_config_path+'/LastStation');

    TRACE.INFO(f'1. ---------------------- validating tracker configuration: first_station:{first_station} last_station:{last_station}')

    for i in range(first_station,last_station+1):
        station_config_path = tracker_config_path+f'/Station_{i:02d}'
        TRACE.DEBUG(1,f'--- validating slot {i:02d} config path:{station_config_path}')
        for plane in range (0,2):
            plane_config_path   = station_config_path+f'/Plane_{plane:02d}'
            plane_name = client.odb_get(plane_config_path+'/Name');
            TRACE.DEBUG(0,f'plane 1 (0:U, 1:D) ,  config path:{plane_config_path} plane_name:{plane_name}')
            
            for panel in range(0,6):
                panel_config_path = plane_config_path+f'/Panel_{panel:02d}'
                panel_name = client.odb_get(panel_config_path+'/Name');
                link       =  client.odb_get(panel_config_path+'/Link');
                TRACE.DEBUG(1,f'plane config path:{plane_config_path} panel_name:{panel_name}')
                # now find the DTC this panel is connected to
                dtc_detector_element_path = plane_config_path+f'/DTC/Link{link}/DetectorElement'
                TRACE.DEBUG(1,f'DTC detector element path:{dtc_detector_element_path}')
                dtc_panel_name = client.odb_get(dtc_detector_element_path+'/Name');
                TRACE.DEBUG(1,f'panel config path:{panel_config_path} panel_name:{panel_name} name2:{dtc_panel_name}')
                if (panel_name != dtc_panel_name):
                    TRACE.ERROR(f'panel_config_path:{panel_config_path} panel_name:{panel_name} dtc_panel_name:{dtc_panel_xsname}')
                    n_errors += 1
                else:
                    TRACE.DEBUG(1,f' panel {panel_name} at ODB config path:{panel_config_path} is OK')

    TRACE.INFO(f'2. ---------------------- validating DTC configuration')
    
    nodes_config_path = f'/Mu2e/ActiveRunConfiguration/DAQ/Nodes'
    nodes = client.odb_get(nodes_config_path);
#------------------------------------------------------------------------------
# using hw CFO: all DTCs should ha JA mode of 0x10
#------------------------------------------------------------------------------
    nominal_ja_mode = 0x10;
    for node_name in nodes.keys():
        node = nodes[node_name];
        for key in node.keys():
            if ((key == "DTC0") or (key == "DTC1")):
                dtc = node[key]
                # validate JAMode
                if (dtc['JAMode'] != nominal_ja_mode):
                    TRACE.ERROR(f'node:{node_name} dtc:{key} has JAMode:0x{dtc["JAMode"]:04x} different from 0x:{nominal_ja_mode:04x}')
                    n_errors += 1
                else:
                    TRACE.DEBUG(1,f'{key}@{node_name}: JAMode = 0x{dtc["JAMode"]:04x} - OK')
        
    TRACE.INFO(f'3. ---------------------- validating default DTC command parameters')
    
    nodes_config_path = f'/Mu2e/Commands/DAQ/Nodes'
    nodes = client.odb_get(nodes_config_path);

#------------------------------------------------------------------------------
# using hw CFO: check if there are DTCs configured to emulate CFO
#------------------------------------------------------------------------------
    for node_name in nodes.keys():
        node = nodes[node_name];
        for eq_name in node.keys():
            if (( eq_name == "DTC0") or (eq_name == "DTC1")):
                commands = node[eq_name]
                for cmd_name in commands.keys():
                    if (cmd_name == "init_readout"):
                        cmd = commands[cmd_name]
                
                        # validate init_readout
                        if (cmd['emulate_cfo'] == 1):
                            TRACE.ERROR(f'WARNING: node:{node_name} dtc:{eq_name} init_readout.emulate_cfo=1')
                            n_errors += 1
                        else:
                            TRACE.DEBUG(1,f'node:{node_name} dtc:{eq_name} init_readout parameters: OK')

    TRACE.INFO(f'total number of detected errors: {n_errors}')
        
    
#------------------------------------------------------------------------------
# DTC parameters
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
if __name__ == "__main__":

#    test1()
#    test2_set_thresholds(25,'MN261')
    validate_active_config();
