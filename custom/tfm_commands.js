//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
// global variables
//-----------------------------------------------------------------------------
let g_node = 0;   // integer index of the active node
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function choose_node_id(evt, id) {
  var i, roctabs;
  tabs = document.getElementsByClassName("nodetabs");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  document.getElementById(id).style.display = "block";
  evt.currentTarget.className += " active";

  let num    = id.substring(4,7);
  g_node = Number(num); // parse the number out of 'nodeXXX'
  console.log('g_node=',g_node);
}


//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function tfm_command_set_odb(cmd) {
  
  var paths=["/Mu2e/Commands/DAQ/Tfm/Name",
             "/Mu2e/Commands/DAQ/Tfm/ParameterPath",
             "/Mu2e/Commands/DAQ/Tfm/Finished",
             "/Mu2e/Commands/DAQ/Tfm/get_state/print_level",
            ];
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/DAQ/Tfm",0,1]).then(function(rpc) {
    result=rpc.result;	      

    // 'Run' is set to 1 , then tfm_frontend is called

    var paths=["/Mu2e/Commands/DAQ/Tfm/Run"];
  
    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
      result=rpc.result;	      
      // tfm_frontend waits till command completes and always returns,
      // but with different return codes...
    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=["/Mu2e/Commands/DAQ/Tfm/Run",
               "/Mu2e/Commands/DAQ/Tfm/Finished"];
    let run      = 1;
    let finished = 1;
    sleep(1000);
    mjsonrpc_db_get_values(paths).then(function(rpc) {
      run      = rpc.result.data[0];
      finished = rpc.result.data[1];
    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
    done = finished;
  };
  
  // I wonder if this line is needed at all....
  displayFile('tfm.log', 'output_window');
}

//-----------------------------------------------------------------------------
function tfm_get_state(element) {
//  clear_window(element)
  tfm_command_set_odb("get_state")
}

//-----------------------------------------------------------------------------
function tfm_clear_window(element) {
//  clear_window(element)
  tfm_command_set_odb("reset_output")
}

//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
function tfm_load_parameters() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Tfm`,0);
}

//-----------------------------------------------------------------------------
function tfm_load_parameters_get_state() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/get_state',0);
}
      
//-----------------------------------------------------------------------------
function tfm_load_parameters_generate_fcl() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/generate_fcl',0);
}
      
//-----------------------------------------------------------------------------
function tfm_load_parameters_print_fcl() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/print_fcl',0);
}
      

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    mode: web
//    End:
