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
// CFO frontend : the name should be fixed! 
//-----------------------------------------------------------------------------
// 2026-03-29 PMfunction cfo_command(cmd) {
// 2026-03-29 PM  let msg = { "client_name":"cfo_emu_fe", "cmd":cmd, "args":'{}'};
// 2026-03-29 PM  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
// 2026-03-29 PM    let s = rpc1.result.reply;
// 2026-03-29 PM    const regex = /\n/gi
// 2026-03-29 PM    let y = s.replaceAll(regex,'<br>');
// 2026-03-29 PM    
// 2026-03-29 PM    const el = document.getElementById("content");
// 2026-03-29 PM    el.innerHTML += y;
// 2026-03-29 PM    el.style.fontFamily = 'monospace';
// 2026-03-29 PM    el.scrollIntoView({ behavior: 'smooth', block: 'end' });
// 2026-03-29 PM    
// 2026-03-29 PM  }).catch(function(error){
// 2026-03-29 PM    mjsonrpc_error_alert(error);
// 2026-03-29 PM  });
// 2026-03-29 PM}

//-----------------------------------------------------------------------------
// 2026-03-29 PMfunction dtc_command(cmd) {
// 2026-03-29 PM  let msg = { "client_name":g_hostname,
// 2026-03-29 PM              "cmd":cmd,
// 2026-03-29 PM              "max_reply_length":100000,
// 2026-03-29 PM              "args":'{"eq_type":"dtc","pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
// 2026-03-29 PM  
// 2026-03-29 PM  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
// 2026-03-29 PM    let s = rpc1.result.reply
// 2026-03-29 PM    console.log(s.length);
// 2026-03-29 PM    
// 2026-03-29 PM  }).catch(function(error){
// 2026-03-29 PM    mjsonrpc_error_alert(error);
// 2026-03-29 PM  });
// 2026-03-29 PM}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
// 2026-03-29 PMasync function dtc_command_set_odb(cmd,logfile) {
// 2026-03-29 PM
// 2026-03-29 PM  let p0 = '/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie;
// 2026-03-29 PM
// 2026-03-29 PM  const paths=[p0+'/Name',
// 2026-03-29 PM               p0+'/link',
// 2026-03-29 PM               p0+'/ParameterPath',
// 2026-03-29 PM               p0+'/Finished',
// 2026-03-29 PM  ];
// 2026-03-29 PM
// 2026-03-29 PM  try {
// 2026-03-29 PM    let rpc = await mjsonrpc_db_paste(paths, [cmd,g_roc,p0+'/'+cmd,0]);
// 2026-03-29 PM    // parameters are set, trigger the execution by setting odb["/Mu2e/Commands/Tracker/Run"] = 1  }
// 2026-03-29 PM    try {
// 2026-03-29 PM      let rpc = mjsonrpc_db_paste([p0+'/Run'], [1]);
// 2026-03-29 PM      
// 2026-03-29 PM      // wait till the command is finished, actual wait happens in the frontend
// 2026-03-29 PM      let finished = 0;
// 2026-03-29 PM      
// 2026-03-29 PM      while (finished == 0) {
// 2026-03-29 PM        let run      = 1;
// 2026-03-29 PM        sleep(1000);
// 2026-03-29 PM        try {
// 2026-03-29 PM          const paths=[p0+'/Run',p0+'/Finished'];
// 2026-03-29 PM          let rpc  = await mjsonrpc_db_get_values(paths);
// 2026-03-29 PM          run      = rpc.result.data[0];
// 2026-03-29 PM          finished = rpc.result.data[1];
// 2026-03-29 PM        }
// 2026-03-29 PM        catch(error) {
// 2026-03-29 PM          mjsonrpc_error_alert(error);
// 2026-03-29 PM        };
// 2026-03-29 PM      };
// 2026-03-29 PM      
// 2026-03-29 PM      displayFile(logfile, 'messageFrame');
// 2026-03-29 PM    }
// 2026-03-29 PM    catch(error) {
// 2026-03-29 PM      mjsonrpc_error_alert(error);
// 2026-03-29 PM    };
// 2026-03-29 PM  }
// 2026-03-29 PM  catch(error) {
// 2026-03-29 PM    mjsonrpc_error_alert(error);
// 2026-03-29 PM  };
// 2026-03-29 PM}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
// 2026-03-29 PMasync function dtc_command_set_odb_B(cmd) {
// 2026-03-29 PM
// 2026-03-29 PM  const ppath = cmd.func_parameter_path(cmd);
// 2026-03-29 PM
// 2026-03-29 PM  let logfile = cmd.logfile;
// 2026-03-29 PM  
// 2026-03-29 PM  if (logfile == null) { logfile   = g_logfile; }
// 2026-03-29 PM  else                 { g_logfile = logfile  ; }
// 2026-03-29 PM  //-----------------------------------------------------------------------------
// 2026-03-29 PM  // passing parameters, setting Run to 1 sets everythign in action
// 2026-03-29 PM  //-----------------------------------------------------------------------------
// 2026-03-29 PM  const paths=[ppath+'/Name',
// 2026-03-29 PM               ppath+'/ParameterPath',
// 2026-03-29 PM               ppath+'/link',
// 2026-03-29 PM               ppath+'/logfile',
// 2026-03-29 PM               ppath+'/Run',
// 2026-03-29 PM  ];
// 2026-03-29 PM
// 2026-03-29 PM  try {
// 2026-03-29 PM        let rpc = await mjsonrpc_db_paste(paths, [cmd.name,ppath+'/'+cmd.name,g_roc,logfile,1]);
// 2026-03-29 PM        let result=rpc.result;	      
// 2026-03-29 PM    }
// 2026-03-29 PM  catch(error) {
// 2026-03-29 PM    mjsonrpc_error_alert(error);
// 2026-03-29 PM  };
// 2026-03-29 PM  
// 2026-03-29 PM  let done = 0;
// 2026-03-29 PM
// 2026-03-29 PM  while(done == 0) {
// 2026-03-29 PM    // check whether the command has finished, finished is set by the frontend
// 2026-03-29 PM    const paths=[ppath+'/Run', ppath+'/Finished'];
// 2026-03-29 PM    let run      = 1;
// 2026-03-29 PM    let finished = 0;
// 2026-03-29 PM    sleep(500);
// 2026-03-29 PM    try {
// 2026-03-29 PM      let rpc = await mjsonrpc_db_get_values(paths);
// 2026-03-29 PM      run      = rpc.result.data[0];
// 2026-03-29 PM      finished = rpc.result.data[1];
// 2026-03-29 PM    }
// 2026-03-29 PM    catch(error) {
// 2026-03-29 PM      mjsonrpc_error_alert(error);
// 2026-03-29 PM    };
// 2026-03-29 PM    done = finished;
// 2026-03-29 PM  };
// 2026-03-29 PM  
// 2026-03-29 PM  // display the logfile. THis is the only non-generic place
// 2026-03-29 PM  // if the command was a struct with the logfile being one of its parameters,
// 2026-03-29 PM  // this function became completely generic
// 2026-03-29 PM
// 2026-03-29 PM  displayFile(logfile, 'messageFrame');
// 2026-03-29 PM}

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

// 2026-03-29 PM//-----------------------------------------------------------------------------
// 2026-03-29 PMfunction dtc_load_cmd_parameters(cmd) {
// 2026-03-29 PM  const table     = document.getElementById('cmd_params');
// 2026-03-29 PM  table.innerHTML = '';
// 2026-03-29 PM  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie+'/'+cmd,0);
// 2026-03-29 PM}

// 2026-03-29 PM//-----------------------------------------------------------------------------
// 2026-03-29 PM// input: Command_B
// 2026-03-29 PM//-----------------------------------------------------------------------------
// 2026-03-29 PMfunction dtc_make_exec_button_B(cmd) {
// 2026-03-29 PM  const btn   = document.createElement('input');
// 2026-03-29 PM  btn.type    = 'button'
// 2026-03-29 PM  btn.value   = cmd.title;
// 2026-03-29 PM  btn.onclick = function() { cmd.func(cmd) ; }
// 2026-03-29 PM  return btn;
// 2026-03-29 PM}

//    emacs
//    Local Variables:
//    mode: web
//    End:
