//-----------------------------------------------------------------------------
// global variables which names start with 'g_' are defined in global_variables.js
// which needs to included before this script
// messages go to tracker.log
//-----------------------------------------------------------------------------
function trk_panel_control(is, iplane, ipanel, mnid) {
  window.open(`tracker_control.html?station=${is}&plane=${iplane}&panel=${ipanel}&mnid=${mnid}&facility=tracker`,'_blank');
}

//-----------------------------------------------------------------------------
function trk_load_help_page(cmd) {
  const url = 'https://mu2einternalwiki.fnal.gov/wiki/Tracker_control_page#';
  window.open(url,'_blank');
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
