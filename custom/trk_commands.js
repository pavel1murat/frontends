//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
//-----------------------------------------------------------------------------
let g_hostname = 'undefined';
let g_station  = 0;
let g_plane    = 0;
let g_panel    = 0;

//-----------------------------------------------------------------------------
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}


//-----------------------------------------------------------------------------
function displayFile(filePath, elementId) {
//  // const fs = require('fs');
//  try {
//    const response = await fetch(filePath);
//    if (!response.ok) {
//      throw new Error(`HTTP error! status: ${response.status}`);
//    }
//    const text = await response.text();
//    document.getElementById(elementId).textContent = text;
//  } catch (error) {
//    document.getElementById(elementId).textContent = 'Error: ' + error.message;
  //  }

  var result = null;
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.open("GET", filePath, false);
  xmlhttp.send();
  if (xmlhttp.status==200) {
    document.getElementById(elementId).textContent = xmlhttp.responseText;
  }
  return result;
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function trk_command_set_odb(cmd) {
  
  var paths=["/Mu2e/Commands/Tracker/Name",
             "/Mu2e/Commands/Tracker/ParameterPath",
             "/Mu2e/Commands/Tracekr/Finished",
             "/Mu2e/Commands/Tracker/Run"];
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker/TRK",0,1]).then(function(rpc) {
    result=rpc.result;	      
    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status

    // somewhere need to wait till command completes

  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=["/Mu2e/Commands/Tracker/Run",
               "/Mu2e/Commands/Tracker/Finished"];
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
  
  displayFile('junk.log', 'output_window');
}

//-----------------------------------------------------------------------------
function trk_clear_window(element) {
//  clear_window(element)
  trk_command_set_odb("trk_reset_output")
}

//-----------------------------------------------------------------------------
// this function sends messages
//-----------------------------------------------------------------------------
function trk_command_msg(cmd) {
  let msg = { "client_name":"trk_cfg", "cmd":cmd, "max_reply_length":100000,
              "args":'{"pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
  mjsonrpc_call("jrpc",msg).then(function(rpc1) {
    let s = rpc1.result.reply
    console.log(s.length);
    let y = '<br>'+s.replaceAll(/\n/gi,'<br>').replace(/ /g, '&nbsp;');
    
    const el = document.getElementById("output_window");
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
// load table with the Panel parameters
//-----------------------------------------------------------------------------
function trk_station_load_parameters(station) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  const istn = station.toString().padStart(2,'0');
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/Tracker/Station_${istn}`,0);
}

//-----------------------------------------------------------------------------
// load table with the Panel parameters
//-----------------------------------------------------------------------------
function trk_panel_load_parameters(station,plane,panel) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  const istn = station.toString().padStart(2,'0');
  const ipln = plane.toString().padStart(2,'0');
  const ipnl = panel.toString().padStart(2,'0');
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/Tracker/Station_${istn}/Plane_${ipln}/Panel_${ipnl}`,0);
}

//-----------------------------------------------------------------------------
function trk_load_parameters_init_readout() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/init_readout',0);
}
      

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    tab-width: 8
//    c-basic-offset: 2
//    js-indent-level: 0
//    indent-tabs-mode: nil
//    End:
