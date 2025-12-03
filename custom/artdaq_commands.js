//-----------------------------------------------------------------------------
// 2025-07-21: this is still a placeholder
// the hostname should be the same within the scope of this script
//-----------------------------------------------------------------------------
let g_hostname    = null;
let g_process     = null;                // artdaq component
//-----------------------------------------------------------------------------      
// function artdaq_command(cmd) {
//   let msg = { "client_name":g_hostname, "cmd":cmd, "max_reply_length":10000,
//               "args":'{"pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
//   mjsonrpc_call("jrpc",msg).then(function(rpc1) {
//     let s = rpc1.result.reply
//     console.log(s.length);
//     let y = '<br>'+s.replaceAll(/\n/gi,'<br>').replace(/ /g, '&nbsp;');
//         
//     const el = document.getElementById("content");
//     el.innerHTML += y;
//     el.style.fontFamily = 'monospace';
//     // el.scrollIntoView();
//     const scrollToBottom = (id) => {
//       el.scrollTop = el.scrollHeight;
//     }
//     
//   }).catch(function(error){
//     mjsonrpc_error_alert(error);
//   });
// }
// 

//-----------------------------------------------------------------------------
function choose_artdaq_process_id(evt, btn_id) {
  var i, tabs;
  tabs = document.getElementsByClassName("artdaq_process_tabs");
  for (i=0; i<roctabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  document.getElementById(btn_id).style.display = "block";
  evt.currentTarget.className += " active";
  
  g_artdaq_pid = Number(btn_id);   // not sure what it stands for
  // -1: all processes
  // if (g_artdaq_pid == 6) {g_roc = -1};
  console.log('g_artdaq_pid=',g_artdaq_pid);
}

//-----------------------------------------------------------------------------
function artdaq_command(cmd) {
  let msg = { "client_name":g_hostname,
              "eq_type:":artdaq,
              "cmd":cmd,
              "max_reply_length":100000,
              "args":'{"eq_type":"artdaq","process":'+g_process+'}'};
  
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
// load table with the artdaq process parameters
//-----------------------------------------------------------------------------
function artdaq_load_parameters() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/Artdaq`,0);
}

//-----------------------------------------------------------------------------
// clear output window
//-----------------------------------------------------------------------------
function artdaq_clear_output() {
  const el = document.getElementById("content");
  el.innerHTML = '';
  el.classList.toggle('force-redraw');
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function artdaq_command_set_odb(cmd) {

  let p0 = '/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/Artdaq';

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
  
  displayFile('trkdtc.log', 'output_window');
}

//-----------------------------------------------------------------------------
function artdaq_reset_output(element) {
//  clear_window(element)
  artdaq_command_set_odb("reset_output")
}
     
//-----------------------------------------------------------------------------
function artdaq_load_cmd_parameters(cmd) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/Artdaq/'+cmd,0);
}

//-----------------------------------------------------------------------------
function artdaq_load_odb_fcl(cmd) {
  const table     = document.getElementById('fcl_params');
  table.innerHTML = '';
  odb_browser('fcl_params','/Mu2e/Commands/DAQ/Nodes/'+g_hostname+'/Artdaq/Fcl',0);
}
      
//    emacs
//    Local Variables:
//    tab-width: 8
//    c-basic-offset: 2
//    js-indent-level: 0
//    indent-tabs-mode: nil
//    End:
