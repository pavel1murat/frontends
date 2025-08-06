//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
//-----------------------------------------------------------------------------
let g_hostname = 'undefined';
let g_pcie     = 0;
let g_roc      = 0;
//-----------------------------------------------------------------------------      
function choose_dtc_id(evt, dtc_id) {
  var i, tablinks;
  tablinks = document.getElementsByClassName("tablinks");
  for (i=0; i<tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }
  document.getElementById(dtc_id).style.display = "block";
  evt.currentTarget.className += " active";
  
  if (dtc_id == 'dtc0') { g_pcie = 0; } else {g_pcie = 1;} ;
  console.log('g_pcie=',g_pcie);

  createTable();
}
//-----------------------------------------------------------------------------      
function choose_roc_id(evt, roc_id) {
  var i, roctabs;
  roctabs = document.getElementsByClassName("roctabs");
  for (i=0; i<roctabs.length; i++) {
    roctabs[i].className = roctabs[i].className.replace(" active", "");
  }
  document.getElementById(roc_id).style.display = "block";
  evt.currentTarget.className += " active";
  
  g_roc = Number(roc_id.charAt(3)); // forth character: 'rocX'
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
function dtc_command_save(cmd) {
  let msg = { "client_name":g_hostname,
              "cmd":cmd,
              "max_reply_length":100000,
              "args":'{"eq_type":"dtc","pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
  
  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
    let s = rpc1.result.reply
    console.log(s.length);
    let y = '<br>'+s.replaceAll(/\n/gi,'<br>').replace(/ /g, '&nbsp;');
    
    const el = document.getElementById("content");
    el.innerHTML += y;
    el.style.fontFamily = 'monospace';
    // el.scrollIntoView();
    const sel = (el || document.body);
    sel.scrollTop = sel.scrollHeight;
    const scrollToBottom = (id) => {
      el.scrollTop = el.scrollHeight;
    }
    el.classList.toggle('force-redraw');
    
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
//    let y = '<br>'+s.replaceAll(/\n/gi,'<br>').replace(/ /g, '&nbsp;');
//    
//    const el = document.getElementById("content");
//    el.innerHTML += y;
//    el.style.fontFamily = 'monospace';
//    // el.scrollIntoView();
//    const sel = (el || document.body);
//    sel.scrollTop = sel.scrollHeight;
//    const scrollToBottom = (id) => {
//      el.scrollTop = el.scrollHeight;
//    }
//    el.classList.toggle('force-redraw');
    
  }).catch(function(error){
    mjsonrpc_error_alert(error);
  });
}

//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
function dtc_load_parameters() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/DTC${g_pcie}`,0);
}

//-----------------------------------------------------------------------------
// clear output window
//-----------------------------------------------------------------------------
function dtc_clear_output() {
  const el = document.getElementById("content");
  el.innerHTML = '';
  el.classList.toggle('force-redraw');
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function dtc_command_set_odb(cmd,logfile) {

  let p0 = '/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie;

  var paths=[p0+'/Name',
             p0+'/ParameterPath',
             p0+'/Finished'
            ];
  
  
  mjsonrpc_db_paste(paths, [cmd,p0,0]).then(function(rpc) {
    result=rpc.result;	      
    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status

    // parameters are set, trigger the execution by setting odb["/Mu2e/Commands/Tracker/Run"] = 1

    var paths=[p0+'/Run'];
  
    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
      result=rpc.result;	      
      // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
      
      // javascript does not wait till the command completes

    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
    
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=[p0+'/Run',p0+'/Finished'];
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
  
  displayFile(logfile, 'output_window');
}

//-----------------------------------------------------------------------------
function dtc_reset_output(element) {
  //  clear_window(element)
  dtc_command_set_odb("reset_output",'trkdtc.log')
}

// //-----------------------------------------------------------------------------
// function dtc_load_parameters_init_readout() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/init_readout',0);
// }
//       
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_read() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_read',0);
// }
//       
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_digi_rw() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_digi_rw',0);
// }
//       
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_pulser_on() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_pulser_on',0);
// }
//       
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_pulser_off() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_pulser_off',0);
// }
//       
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_rates() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_rates',0);
// }
// 
// //-----------------------------------------------------------------------------
// function dtc_load_parameters_control_roc_read_ddr() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_roc_read_ddr',0);
// }
      
//-----------------------------------------------------------------------------
function dtc_load_cmd_parameters(cmd) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/DTC'+g_pcie+'/'+cmd,0);
}
      
//    emacs
//    Local Variables:
//    tab-width: 8
//    c-basic-offset: 2
//    js-indent-level: 0
//    indent-tabs-mode: nil
//    End:
