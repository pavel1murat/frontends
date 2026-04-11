//-----------------------------------------------------------------------------
// global variables which names start with 'g_' are defined in global_variables.js
// DTC id = 'dtc0' or 'dtc1'
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
function dtc_load_parameters() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/DTC${g_pcie}`,0);
}

//-----------------------------------------------------------------------------
function choose_dtc_id(evt, id) {
  let i, dtctabs;
  dtctabs = document.getElementsByClassName("dtctabs");
  // reset all tabs
  for (i=0; i<dtctabs.length; i++) {
    dtctabs[i].className = dtctabs[i].className.replace(" active", "");
  }
  document.getElementById(id).style.display = "block";
  evt.currentTarget.className += " active";
  
  if (id == 'dtc0') { g_pcie = 0; } else {g_pcie = 1;} ;
  console.log('g_pcie=',g_pcie);
}

//-----------------------------------------------------------------------------
// 'createTable' is a function loading the DTC control page
//-----------------------------------------------------------------------------
function update_dtc_id(evt, id) {
  choose_dtc_id(evt,id);
  loadPage();
  // createRocCommandsTable();
  dtc_load_parameters();
}

//-----------------------------------------------------------------------------      
// ROC id = 'roc${i'
//-----------------------------------------------------------------------------
function choose_roc_id(evt, id) {
  let i, roctabs;
  roctabs = document.getElementsByClassName("roctabs");
  for (i=0; i<roctabs.length; i++) {
    roctabs[i].className = roctabs[i].className.replace(" active", "");
  }
  document.getElementById(id).style.display = "block";
  evt.currentTarget.className += " active";
  
  g_roc = Number(id.charAt(3)); // forth character: 'rocX'
  // -1: all ROCs
  if (g_roc == 6) {g_roc = -1};
  console.log('g_roc=',g_roc);
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// cmd type: Command_B
// the command parameters record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function dtc_command_set_odb_C(cmd) {

  const ppath = cmd.func_parameter_path(cmd);

  let logfile = cmd.logfile;
  
  if (logfile == null) { logfile   = g_logfile; }
  else                 { g_logfile = logfile  ; }
//-----------------------------------------------------------------------------
// passing DTC-specific parameters
//-----------------------------------------------------------------------------
  const paths=[ppath+'/link'];

  let result = null;
  mjsonrpc_db_paste(paths, [g_roc]).then(function(rpc) {
    // succeeded, call mu2e_command_set_odb_B
    result=rpc.result;

    let rpc1 = mu2e_command_set_odb_B(cmd);
    
  }).catch(function(error) {mjsonrpc_error_alert(error);});
  
}

//    emacs
//    Local Variables:
//    mode: web
//    End:
