import  midas,TRACE
import  midas.client
import  os, psycopg2
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
                
        

#------------------------------------------------------------------------------
if __name__ == "__main__":

    test1()
