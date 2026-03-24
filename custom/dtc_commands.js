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
function cfo_command(cmd) {
  let msg = { "client_name":"cfo_emu_fe", "cmd":cmd, "args":'{}'};
  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
    let s = rpc1.result.reply;
    const regex = /\n/gi
    let y = s.replaceAll(regex,'<br>');
    
    const el = document.getElementById("content");
    el.innerHTML += y;
    el.style.fontFamily = 'monospace';
    el.scrollIntoView({ behavior: 'smooth', block: 'end' });
    
  }).catch(function(error){
    mjsonrpc_error_alert(error);
  });
}

//-----------------------------------------------------------------------------
function dtc_command(cmd) {
  let msg = { "client_name":g_hostname,
              "cmd":cmd,
              "max_reply_length":100000,
              "args":'{"eq_type":"dtc","pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
  
  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
    let s = rpc1.result.reply
    console.log(s.length);
    
  }).catch(function(error){
    mjsonrpc_error_alert(error);
  });
}

//-----------------------------------------------------------------------------
// clear output window
//-----------------------------------------------------------------------------
//function dtc_clear_output() {
//  const el = document.getElementById("content");
//  el.innerHTML = '';
//  el.classList.toggle('force-redraw');
//}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
async function dtc_command_set_odb(cmd,logfile) {

  let p0 = '/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie;

  const paths=[p0+'/Name',
               p0+'/link',
               p0+'/ParameterPath',
               p0+'/Finished',
  ];

  try {
    let rpc = await mjsonrpc_db_paste(paths, [cmd,g_roc,p0+'/'+cmd,0]);
    // parameters are set, trigger the execution by setting odb["/Mu2e/Commands/Tracker/Run"] = 1  }
    try {
      let rpc = mjsonrpc_db_paste([p0+'/Run'], [1]);
      
      // wait till the command is finished, actual wait happens in the frontend
      let finished = 0;
      
      while (finished == 0) {
        let run      = 1;
        sleep(1000);
        try {
          const paths=[p0+'/Run',p0+'/Finished'];
          let rpc  = await mjsonrpc_db_get_values(paths);
          run      = rpc.result.data[0];
          finished = rpc.result.data[1];
        }
        catch(error) {
          mjsonrpc_error_alert(error);
        };
      };
      
      displayFile(logfile, 'messageFrame');
    }
    catch(error) {
      mjsonrpc_error_alert(error);
    };
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function dtc_command_set_odb_B(cmd) {

  const ppath = cmd.func_parameter_path(cmd);

  let logfile = cmd.logfile;
  
  if (logfile == null) { logfile   = g_logfile; }
  else                 { g_logfile = logfile  ; }
    //-----------------------------------------------------------------------------
    // passing parameters, setting Run to 1 sets everythign in action
    //-----------------------------------------------------------------------------
    const paths=[ppath+'/Name',
                 ppath+'/ParameterPath',
                 ppath+'/logfile',
                 ppath+'/Run',
    ];

    try {
        let rpc = await mjsonrpc_db_paste(paths, [cmd.name,ppath+'/'+cmd.name,logfile,1]);
        let result=rpc.result;	      
    }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
  
  let done = 0;

  while(done == 0) {
    // check whether the command has finished, finished is set by the frontend
    const paths=[ppath+'/Run', ppath+'/Finished'];
    let run      = 1;
    let finished = 0;
    sleep(500);
    try {
      let rpc = await mjsonrpc_db_get_values(paths);
      run      = rpc.result.data[0];
      finished = rpc.result.data[1];
    }
    catch(error) {
      mjsonrpc_error_alert(error);
    };
    done = finished;
  };
  
  // display the logfile. THis is the only non-generic place
  // if the command was a struct with the logfile being one of its parameters,
  // this function became completely generic

  displayFile(logfile, 'messageFrame');
}

//-----------------------------------------------------------------------------
// and this one updates ODB
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


////-----------------------------------------------------------------------------
//async function dtc_command_write_roc_register(cmd) {
//
//  const ppath = cmd.func_parameter_path(cmd);
//
//  let logfile = cmd.logfile;
//  
//  if (logfile == null) { logfile   = g_logfile; }
//  else                 { g_logfile = logfile  ; }
////-----------------------------------------------------------------------------
//// passing DTC-specific parameters
//// and link and register to read
//  //-----------------------------------------------------------------------------
//  cppath = ppath+'/'+cmd.name;
//  const paths=[ppath+'/link',cpath+'/register'];
//
//  let result = null;
//  mjsonrpc_db_paste(paths, [g_roc]).then(function(rpc) {
//    // succeeded, call mu2e_command_set_odb_B
//    result=rpc.result;
//
//    let rpc1 = mu2e_command_set_odb_B(cmd);
//    
//  }).catch(function(error) {mjsonrpc_error_alert(error);});
//  
//}

//-----------------------------------------------------------------------------
function dtc_load_cmd_parameters(cmd) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie+'/'+cmd,0);
}

//-----------------------------------------------------------------------------
// input: Command_B
//-----------------------------------------------------------------------------
function dtc_make_exec_button_B(cmd) {
  const btn   = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.title;
  btn.onclick = function() { cmd.func(cmd) ; }
  return btn;
}

//    emacs
//    Local Variables:
//    mode: web
//    End:
