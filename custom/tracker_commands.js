//-----------------------------------------------------------------------------
// global variables which names start with 'g_' are defined in global_variables.js
// which needs to included before this script
// messages go to tracker.log
//-----------------------------------------------------------------------------
function trk_panel_control(is, iplane, ipanel, mnid) {
  window.open(`tracker_control.html?station=${is}&plane=${iplane}&panel=${ipanel}&mnid=${mnid}&facility=tracker`,'_blank');
}

//-----------------------------------------------------------------------------      
function trk_choose_station_id(evt, station_id) {
  let tabs = document.getElementsByClassName("trk_station_tab");
  for (let i=0; i<tabs.length; i++) {
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
async function trk_choose_panel_id(evt, panel_id) {

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
// after that, may need to redefine the plane, station stays the same
//-----------------------------------------------------------------------------
  var x1 = Number(panel_id.charAt(2))*100 + Number(panel_id.charAt(3))*10;
  var x  = x1.toString().padStart(3,'0');
  
  var path = '/Mu2e/ActiveRunConfiguration/Tracker/PanelMap/'+x+'/'+panel_id+'/Panel/slot_id';
  
  try {
    const rpc = await mjsonrpc_db_get_values([path]);
    slot_id = rpc.result.data[0];

//    g_station = station;
    let plane = Math.trunc(slot_id/10);
    g_plane   = plane%2;
    g_panel   = slot_id-plane*10;
  }
  catch(error) {
  //  g_station = -1;
    g_plane   = -1;
    g_panel   = -1;

    mjsonrpc_error_alert(error);
  };
  console.log('g_panel=',g_mnid,' g_plane:',g_plane,' g_station:',g_station);
}

//-----------------------------------------------------------------------------
function trk_cmd_parameter_path(cmd) {
  return '/Mu2e/Commands/Tracker';
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
             "/Mu2e/Commands/Tracker/"+cmd+"/mnid",
  ];
  
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker",0,g_mnid]).then(function(rpc) {
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
// input Comamnd_A type, 
//-----------------------------------------------------------------------------
async function trk_panel_command_set_odb(cmda) {
  
  //------------------------------------------------------------------------------
  // set input parameters
  //------------------------------------------------------------------------------
  try {
    const paths=["/Mu2e/Commands/Tracker/Name",
                 "/Mu2e/Commands/Tracker/ParameterPath",
                 "/Mu2e/Commands/Tracker/Finished",
                 "/Mu2e/Commands/Tracker/mnid",
    ];
    
    const rpc = await mjsonrpc_db_paste(paths, [cmda.name,cmda.parameter_path+'/'+cmda.name,0,g_mnid]);
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  }

  //-----------------------------------------------------------------------------
  // run command: set Run=1
  //-----------------------------------------------------------------------------
  try {
    const paths=["/Mu2e/Commands/Tracker/Run"];
    const rpc = await mjsonrpc_db_paste(paths, [1])
  }
  catch (error) {
    mjsonrpc_error_alert(error);
  };

  // should arrive here only after the command execution has started
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
             "/Mu2e/Commands/Tracker/"+cmd+"/station",
            ];
  
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker",0,g_station]).then(function(rpc) {
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
// load table with the tracekr parameters
//-----------------------------------------------------------------------------
function trk_load_parameters(station) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/Tracker`,0);
}

//-----------------------------------------------------------------------------
// load table with the station parameters
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
  odb_browser('cmd_params','/Mu2e/Commands/Tracker/panel_'+cmd,0);
}
      
//-----------------------------------------------------------------------------
function trk_load_parameters_init_readout() {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params','/Mu2e/Commands/Tracker/init_readout',0);
}
      

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    mode: web
//    End:
