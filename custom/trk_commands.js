//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
//-----------------------------------------------------------------------------
let g_hostname = 'undefined';
let g_station  = 0;
let g_plane    = 0;
let g_panel    = 0;
let g_mnid     = 0;

//-----------------------------------------------------------------------------
// messages go to tracker.log
//-----------------------------------------------------------------------------
function trk_panel_control(is, iplane, ipanel, mnid) {
    window.location.href = `trk_panel_control.html?station=${is}&plane=${iplane}&panel=${ipanel}&mnid=${mnid}&facility=tracker`;
}

//-----------------------------------------------------------------------------      
function trk_choose_station_id(evt, station_id) {
  var i, tabs;
  tabs = document.getElementsByClassName("trk_station_tab");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  elid = 'trk_station_'+station_id;
  document.getElementById(elid).style.display = "block";
  evt.currentTarget.className += " active";

  g_station = parseInt(station_id);
  console.log('g_station=',g_station);
}

//-----------------------------------------------------------------------------      
function trk_choose_plane_id(evt, plane_id) {
  var i, tabs;
  tabs = document.getElementsByClassName("trk_plane_tab");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }

  elid = 'trk_plane_'+plane_id;
  document.getElementById(elid).style.display = "block";
  evt.currentTarget.className += " active";
  
  g_plane = parseInt(plane_id);

  console.log('g_plane=',g_plane);
}

//-----------------------------------------------------------------------------      
function trk_choose_panel_id(evt, panel_id) {
  var i, tabs;
  tabs = document.getElementsByClassName("trk_panel_tab");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  
  elid = panel_id;
  document.getElementById(elid).style.display = "block";
  evt.currentTarget.className += " active";
  
  g_mnid = parseInt(panel_id.substring(2));
//-----------------------------------------------------------------------------
// after that, need to redefine geo indices
  //-----------------------------------------------------------------------------
  var x1 = Number(panel_id.charAt(2))*100;
  var x2 = x1.toString();
  var x  = x2.padStart(3,'0');
  var panel_path = '/Mu2e/Subsystems/Tracker/PanelMap/'+x+'/'+panel_id+'/Panel';
  var paths=[panel_path+'/GeoStation', panel_path+'/GeoPlane', panel_path+'/GeoPanel'];
  let station = -1;
  let plane   = -1;
  let panel   = -1;
  mjsonrpc_db_get_values(paths).then(function(rpc) {
    station = rpc.result.data[0];
    plane   = rpc.result.data[1];
    panel   = rpc.result.data[2];

//    trk_choose_station_id(evt,station)
//    trk_choose_plane_id  (evt,plane  )
    g_station = station;
    g_plane   = plane;
    g_panel   = panel;
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });

  

  console.log('g_panel=',g_mnid);
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function trk_command_set_odb(cmd) {
  
  var paths=["/Mu2e/Commands/Tracker/Name",
             "/Mu2e/Commands/Tracker/ParameterPath",
             "/Mu2e/Commands/Tracker/Finished",
             "/Mu2e/Commands/Tracker/TRK/"+cmd+"/mnid",
            ];
  
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker/TRK",0,g_mnid]).then(function(rpc) {
    result=rpc.result;	      
    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status

    // somewhere need to wait till command completes

    var paths=["/Mu2e/Commands/Tracker/Run"];
  
    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
      result=rpc.result;	      
      // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
      
      // somewhere need to wait till command completes

    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
    
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
  
  displayFile('tracker.log', 'output_window');
}

//-----------------------------------------------------------------------------
function trk_panel_command_set_odb(cmd) {
  
  var paths=["/Mu2e/Commands/Tracker/Name",
             "/Mu2e/Commands/Tracker/ParameterPath",
             "/Mu2e/Commands/Tracker/Finished",
             "/Mu2e/Commands/Tracker/TRK/"+cmd+"/mnid",
            ];
  
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker/TRK",0,g_mnid]).then(function(rpc) {
    result=rpc.result;	      
    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status

    // somewhere need to wait till command completes

    var paths=["/Mu2e/Commands/Tracker/Run"];
  
    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
      result=rpc.result;	      
      // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
      
      // somewhere need to wait till command completes

    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
    
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
  
  displayFile('tracker.log', 'output_window');
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function trk_station_command_set_odb(cmd) {
  
  var paths=["/Mu2e/Commands/Tracker/Name",
             "/Mu2e/Commands/Tracker/ParameterPath",
             "/Mu2e/Commands/Tracker/Finished",
             "/Mu2e/Commands/Tracker/TRK/"+cmd+"/station",
            ];
  
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker/TRK",0,g_station]).then(function(rpc) {
    result=rpc.result;	      
    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status

    // parameters are set, trigger the execution by setting odb["/Mu2e/Commands/Tracker/Run"] = 1

    var paths=["/Mu2e/Commands/Tracker/Run"];
  
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
  
  displayFile('tracker.log', 'output_window');
}

//-----------------------------------------------------------------------------
function trk_reset_output(element) {
//  clear_window(element)
  trk_command_set_odb("reset_output")
}

//-----------------------------------------------------------------------------
// this function sends RPC messages
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
// load parameters
//-----------------------------------------------------------------------------
function trk_load_parameters_reset_station_lv(station) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  const istn = station.toString().padStart(2,'0');

  var paths=["/Mu2e/ActiveRunConfiguration/Tracker/Station_"+istn+"/RPI/Name"];
  mjsonrpc_db_get_values(paths).then(function(rpc) {
    let rpi      = rpc.result.data[0];
    odb_browser('cmd_params',`/Mu2e/Commands/Tracker/RPI/${rpi}/reset_station_lv`,0);
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
};

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
function trk_panel_load_cmd_parameters(cmd) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/Tracker/TRK/panel_'+cmd,0);
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
//    js-indent-level: 2
//    indent-tabs-mode: nil
//    End:
