//-----------------------------------------------------------------------------
// global variables which names start with 'g_' are defined in global_variables.js
// which needs to included before this script
// messages go to tracker.log
//-----------------------------------------------------------------------------
function trk_panel_control(is, iplane, ipanel, mnid) {
  window.open(`tracker_control.html?station=${is}&plane=${iplane}&panel=${ipanel}&mnid=${mnid}&facility=tracker`,'_blank');
}

//-----------------------------------------------------------------------------
// station_id is expected to be a string ... mmm
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

// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PM// and this one updates ODB
// 2026-04-11 PM// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// 2026-04-11 PM// TEquipmentTracker will finally update /Finished
// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PMfunction trk_command_set_odb(cmd) {
// 2026-04-11 PM  
// 2026-04-11 PM  var paths=["/Mu2e/Commands/Tracker/Name",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/ParameterPath",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/Finished",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/"+cmd+"/mnid",
// 2026-04-11 PM  ];
// 2026-04-11 PM  
// 2026-04-11 PM  
// 2026-04-11 PM  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker",0,g_mnid]).then(function(rpc) {
// 2026-04-11 PM    result=rpc.result;	      
// 2026-04-11 PM    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
// 2026-04-11 PM
// 2026-04-11 PM    // somewhere need to wait till command completes
// 2026-04-11 PM
// 2026-04-11 PM    var paths=["/Mu2e/Commands/Tracker/Run"];
// 2026-04-11 PM  
// 2026-04-11 PM    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
// 2026-04-11 PM      result=rpc.result;	      
// 2026-04-11 PM      // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
// 2026-04-11 PM      
// 2026-04-11 PM      // somewhere need to wait till command completes
// 2026-04-11 PM
// 2026-04-11 PM    }).catch(function(error) {
// 2026-04-11 PM      mjsonrpc_error_alert(error);
// 2026-04-11 PM    });
// 2026-04-11 PM    
// 2026-04-11 PM  }).catch(function(error) {
// 2026-04-11 PM    mjsonrpc_error_alert(error);
// 2026-04-11 PM  });
// 2026-04-11 PM  
// 2026-04-11 PM  let done = 0;
// 2026-04-11 PM
// 2026-04-11 PM  while(done == 0) {
// 2026-04-11 PM      // check whether the command has finished
// 2026-04-11 PM    var paths=["/Mu2e/Commands/Tracker/Run",
// 2026-04-11 PM               "/Mu2e/Commands/Tracker/Finished"];
// 2026-04-11 PM    let run      = 1;
// 2026-04-11 PM    let finished = 1;
// 2026-04-11 PM    sleep(1000);
// 2026-04-11 PM    mjsonrpc_db_get_values(paths).then(function(rpc) {
// 2026-04-11 PM      run      = rpc.result.data[0];
// 2026-04-11 PM      finished = rpc.result.data[1];
// 2026-04-11 PM    }).catch(function(error) {
// 2026-04-11 PM      mjsonrpc_error_alert(error);
// 2026-04-11 PM    });
// 2026-04-11 PM    done = finished;
// 2026-04-11 PM  };
// 2026-04-11 PM  
// 2026-04-11 PM  displayFile('tracker.log', 'output_window');
// 2026-04-11 PM}

// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PM// input Command_A type, 
// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PMasync function trk_panel_command_set_odb(cmda) {
// 2026-04-11 PM  
// 2026-04-11 PM  //------------------------------------------------------------------------------
// 2026-04-11 PM  // set input parameters
// 2026-04-11 PM  //------------------------------------------------------------------------------
// 2026-04-11 PM  try {
// 2026-04-11 PM    const paths=["/Mu2e/Commands/Tracker/Name",
// 2026-04-11 PM                 "/Mu2e/Commands/Tracker/ParameterPath",
// 2026-04-11 PM                 "/Mu2e/Commands/Tracker/Finished",
// 2026-04-11 PM                 "/Mu2e/Commands/Tracker/mnid",
// 2026-04-11 PM    ];
// 2026-04-11 PM    
// 2026-04-11 PM    const rpc = await mjsonrpc_db_paste(paths, [cmda.name,cmda.parameter_path+'/'+cmda.name,0,g_mnid]);
// 2026-04-11 PM  }
// 2026-04-11 PM  catch(error) {
// 2026-04-11 PM    mjsonrpc_error_alert(error);
// 2026-04-11 PM  }
// 2026-04-11 PM
// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PM// run command: set Run=1
// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PM  try {
// 2026-04-11 PM    const paths=["/Mu2e/Commands/Tracker/Run"];
// 2026-04-11 PM    const rpc = await mjsonrpc_db_paste(paths, [1])
// 2026-04-11 PM  }
// 2026-04-11 PM  catch (error) {
// 2026-04-11 PM    mjsonrpc_error_alert(error);
// 2026-04-11 PM  };
// 2026-04-11 PM
// 2026-04-11 PM  // should arrive here only after the command execution has started
// 2026-04-11 PM  let done = 0;
// 2026-04-11 PM
// 2026-04-11 PM  while(done == 0) {
// 2026-04-11 PM    // check whether the command has finished
// 2026-04-11 PM    var paths=["/Mu2e/Commands/Tracker/Run",
// 2026-04-11 PM               "/Mu2e/Commands/Tracker/Finished"];
// 2026-04-11 PM    let run      = 1;
// 2026-04-11 PM    let finished = 1;
// 2026-04-11 PM    sleep(1000);
// 2026-04-11 PM    mjsonrpc_db_get_values(paths).then(function(rpc) {
// 2026-04-11 PM      run      = rpc.result.data[0];
// 2026-04-11 PM      finished = rpc.result.data[1];
// 2026-04-11 PM    }).catch(function(error) {
// 2026-04-11 PM      mjsonrpc_error_alert(error);
// 2026-04-11 PM    });
// 2026-04-11 PM    done = finished;
// 2026-04-11 PM  };
// 2026-04-11 PM  
// 2026-04-11 PM  displayFile('tracker.log', 'output_window');
// 2026-04-11 PM}

//-----------------------------------------------------------------------------
// input Command_B type, set MNID , the rest - generic
//-----------------------------------------------------------------------------
async function trk_panel_command_set_odb_B(cmd) {
  
  try {
    const rpc = await mjsonrpc_db_paste(["/Mu2e/Commands/Tracker/mnid" ], [g_mnid]);
    // and after that proceed with teh standard part
    mu2e_command_set_odb_B(cmd);
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  }
}

// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PM// and this one updates ODB
// 2026-04-11 PM// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// 2026-04-11 PM// TEquipmentTracker will finally update /Finished
// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PMfunction trk_station_command_set_odb(cmd) {
// 2026-04-11 PM  
// 2026-04-11 PM  var paths=["/Mu2e/Commands/Tracker/Name",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/ParameterPath",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/Finished",
// 2026-04-11 PM             "/Mu2e/Commands/Tracker/"+cmd+"/station",
// 2026-04-11 PM  ];
// 2026-04-11 PM  
// 2026-04-11 PM  
// 2026-04-11 PM  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/Tracker",0,g_station]).then(function(rpc) {
// 2026-04-11 PM    result=rpc.result;	      
// 2026-04-11 PM    // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
// 2026-04-11 PM
// 2026-04-11 PM    // parameters are set, trigger the execution by setting odb["/Mu2e/Commands/Tracker/Run"] = 1
// 2026-04-11 PM
// 2026-04-11 PM    var paths=["/Mu2e/Commands/Tracker/Run"];
// 2026-04-11 PM  
// 2026-04-11 PM    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
// 2026-04-11 PM      result=rpc.result;	      
// 2026-04-11 PM      // document.getElementById("wstatus").innerHTML = 'Write status '+rpc.result.status
// 2026-04-11 PM      
// 2026-04-11 PM      // javascript does not wait till the command completes
// 2026-04-11 PM
// 2026-04-11 PM    }).catch(function(error) {
// 2026-04-11 PM      mjsonrpc_error_alert(error);
// 2026-04-11 PM    });
// 2026-04-11 PM    
// 2026-04-11 PM  }).catch(function(error) {
// 2026-04-11 PM    mjsonrpc_error_alert(error);
// 2026-04-11 PM  });
// 2026-04-11 PM  
// 2026-04-11 PM  let done = 0;
// 2026-04-11 PM
// 2026-04-11 PM  while(done == 0) {
// 2026-04-11 PM      // check whether the command has finished
// 2026-04-11 PM    var paths=["/Mu2e/Commands/Tracker/Run",
// 2026-04-11 PM               "/Mu2e/Commands/Tracker/Finished"];
// 2026-04-11 PM    let run      = 1;
// 2026-04-11 PM    let finished = 1;
// 2026-04-11 PM    sleep(1000);
// 2026-04-11 PM    mjsonrpc_db_get_values(paths).then(function(rpc) {
// 2026-04-11 PM      run      = rpc.result.data[0];
// 2026-04-11 PM      finished = rpc.result.data[1];
// 2026-04-11 PM    }).catch(function(error) {
// 2026-04-11 PM      mjsonrpc_error_alert(error);
// 2026-04-11 PM    });
// 2026-04-11 PM    done = finished;
// 2026-04-11 PM  };
// 2026-04-11 PM  
// 2026-04-11 PM  displayFile('tracker.log', 'output_window');
// 2026-04-11 PM}
// 2026-04-11 PM
// 2026-04-11 PM// 2026-04-11 PM//-----------------------------------------------------------------------------
// 2026-04-11 PMfunction trk_reset_output(element) {
// 2026-04-11 PM//  clear_window(element)
// 2026-04-11 PM  trk_command_set_odb("reset_output")
// 2026-04-11 PM}

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
// 2026-03-31 PMfunction trk_load_parameters(station) {
// 2026-03-31 PM  const table     = document.getElementById('cmd_params');
// 2026-03-31 PM  table.innerHTML = '';
// 2026-03-31 PM  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/Tracker`,0);
// 2026-03-31 PM}

//-----------------------------------------------------------------------------
// load table with the station parameters
// 2026-03-31 PM : this one is needed
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
// 2026-03-31 PMfunction trk_load_parameters_reset_station_lv(station) {
// 2026-03-31 PM  const table     = document.getElementById('cmd_params');
// 2026-03-31 PM  table.innerHTML = '';
// 2026-03-31 PM  const istn = station.toString().padStart(2,'0');
// 2026-03-31 PM
// 2026-03-31 PM  var paths=["/Mu2e/ActiveRunConfiguration/Tracker/Station_"+istn+"/RPI/Name"];
// 2026-03-31 PM  mjsonrpc_db_get_values(paths).then(function(rpc) {
// 2026-03-31 PM    let rpi      = rpc.result.data[0];
// 2026-03-31 PM    odb_browser('cmd_params',`/Mu2e/Commands/Tracker/RPI/${rpi}/reset_station_lv`,0);
// 2026-03-31 PM  }).catch(function(error) {
// 2026-03-31 PM    mjsonrpc_error_alert(error);
// 2026-03-31 PM  });
// 2026-03-31 PM};

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
// 2026-03-31 PMfunction trk_load_parameters_init_readout() {
// 2026-03-31 PM  const table     = document.getElementById('cmd_params');
// 2026-03-31 PM  table.innerHTML = '';
// 2026-03-31 PM  odb_browser('cmd_params','/Mu2e/Commands/Tracker/init_readout',0);
// 2026-03-31 PM}
      
//-----------------------------------------------------------------------------
// load station / plane / panel and call mu2e_command_set_odb_B
// handle all panels
//-----------------------------------------------------------------------------
function trk_cmd_station_B(cmd) {

  ppath = cmd.func_parameter_path(cmd);
  paths = [ ppath+'/station', ppath+'/plane', ppath+'/mnid' ];

  mjsonrpc_db_paste(paths, [g_station, -1, -1]).then(function(rpc) {
    result=rpc.result;
    // station is set, complete the command
    mu2e_command_set_odb_B(cmd);
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
}

//-----------------------------------------------------------------------------
// g_station = -1 ?
//-----------------------------------------------------------------------------
function trk_cmd_all_stations_B(cmd) {

  ppath = cmd.func_parameter_path(cmd);
  paths = [ ppath+'/station', ppath+'/plane', ppath+'/mnid' ];

  mjsonrpc_db_paste(paths, [-1, -1, -1]).then(function(rpc) {
    result=rpc.result;
    // station is set, complete the command
    mu2e_command_set_odb_B(cmd);
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
}

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    mode: web
//    End:
