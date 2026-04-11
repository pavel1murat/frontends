//-----------------------------------------------------------------------------
// could think of having a cmd.name and cmd.title, but in absense of pointers 
// that could complicate things
// parameter_path points to the ODB path where the command name, parameter_path,
// and 'Finished' should be stored
//-----------------------------------------------------------------------------
class Command {
  constructor(name,table_id,parameter_path) {
    this.name           = name;
    this.table_id       = table_id;
    this.parameter_path = parameter_path;
  }
}
//-----------------------------------------------------------------------------
// func_parameter_path is expected to return the ODB path to common command parameters of the DTC, tracker, etc
// to get parameters of the command itself, the value returned by func_parameter_path
// needs to be appended with the `/${cmd.name}`
//-----------------------------------------------------------------------------
class Command_B {
  constructor(title,name,func,table_id,func_parameter_path,logfile) {
    this.title               = title;
    this.name                = name;
    this.func                = func;
    this.table_id            = table_id;
    this.func_parameter_path = func_parameter_path;
    this.logfile             = logfile;
  }
}

//-----------------------------------------------------------------------------
class Command_D { // aaaa
  constructor(title,name,menu_id,f_handle_left_click,table_id,f_parameter_path,logfile) { // hack
    this.title               = title;
    this.name                = name;
    this.menu_id             = menu_id;              // defines right click actions
    this.f_handle_left_click = f_handle_left_click;  // the number of parameters : TBD
    this.table_id            = table_id;             // ID of the HTML table element to be used by MIDAS JS for displaying
    this.func_parameter_path = f_parameter_path;
    if (logfile == 'trkdtc.log') {
      // this is a transitioning step
      logfile = `${g_hostname}_dtc_${g_pcie}.log`
    }
    else {
      this.logfile             = logfile;
    }
  }  
}

//-----------------------------------------------------------------------------
function artdaq_process_config_path(cmd) {
  const path = `/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/Artdaq/${g_process_label}`;
  return path;
}

//-----------------------------------------------------------------------------
function cfo_config_path(cmd) {
  const path = '/Mu2e/ActiveRunConfiguration/DAQ/CFO';
  return path;
}

//-----------------------------------------------------------------------------
// command name is added elsewhere
function cfo_cmd_parameter_path(cmd) {
  const path = '/Mu2e/Commands/DAQ/CFO';
  return path;
}

//-----------------------------------------------------------------------------
// load table with the CFO parameters
// has to follow the Command_B interface for mu2e_make_exec_button
//-----------------------------------------------------------------------------
function cfo_load_parameters(cmd) {
  const table     = document.getElementById('cmd_params');
  table.innerHTML = '';
  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/CFO`,0);
}

//-----------------------------------------------------------------------------
function node_artdaq_parameter_path(cmd) {
  return `/Mu2e/Commands/DAQ/Nodes/${g_hostname}/Artdaq`;
}

//-----------------------------------------------------------------------------
function dtc_config_path(cmd) {
  const path = `/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/DTC${g_pcie}`;
  return path;
}

//-----------------------------------------------------------------------------
// cmd is of Command_B type
// returns ODB path of the _top_ DTC command parameters
// address of parameters of the command itself adds '/cmd.name'
//-----------------------------------------------------------------------------
function dtc_cmd_parameter_path(cmd) {
  const path = `/Mu2e/Commands/DAQ/Nodes/${g_hostname}/DTC${g_pcie}`;
  return path;
}

//-----------------------------------------------------------------------------
function tfm_cmd_parameter_path   (hostname) { return `/Mu2e/Commands/DAQ/Tfm`; }
function tfm_config_parameter_path(hostname) { return `/Mu2e/ActiveRunConfiguration/DAQ/Tfm`; }

//-----------------------------------------------------------------------------
function test_cmd_parameter_path(cmd) { return `/Mu2e/Commands/Test`; }

//-----------------------------------------------------------------------------
function trk_config_path       (cmd) { return `/Mu2e/ActiveRunConfiguration/Tracker`; }
function trk_cmd_parameter_path(cmd) { return '/Mu2e/Commands/Tracker'; }
function trk_cmd_path          (cmd) { return `/Mu2e/Commands/Tracker`; }

//-----------------------------------------------------------------------------
function trk_panel_config_path(cmd) {
  const stn  = g_station.toString().padStart(2,'0');
  const pln  = g_plane.toString().padStart(2,'0');
  const pnl  = g_panel.toString().padStart(2,'0');
  const path = `/Mu2e/ActiveRunConfiguration/Tracker/Station_${stn}/Plane_${pln}/Panel_${pnl}`;
  return path;
}

//-----------------------------------------------------------------------------
// common javascript functions
// DAQ colors. Each element has 'Enabled' and 'Status' field
//-----------------------------------------------------------------------------
async function set_colors(path, cell) {
  // Fetch values for Enabled and Status
  const path_enabled = path+`/Enabled`;
  const path_status  = path+`/Status`;
  try {
    let rpc = await mjsonrpc_db_get_values([path_enabled, path_status]); // .then(function (rpc) {
    const enabled = rpc.result.data[0];
    const status  = rpc.result.data[1];
    
    // Apply color styles
    if (enabled === 0) {
      cell.style.backgroundColor = "gray";
      cell.style.color = "white";
    }
    else if (enabled === 1) {
      if (status === 0) {
        cell.style.backgroundColor = "green";
        cell.style.color = "white";
      }
      else if (status < 0) {
        cell.style.backgroundColor = "red";
        cell.style.color = "white";
      }
      else if (status > 0) {
        cell.style.backgroundColor = "yellow";
        cell.style.color = "black";
      }
    }
    else {
      cell.style.backgroundColor = "gray";
      cell.style.color = "white";
    }
  }
  catch(error) {
    console.error("Error fetching values:", error);
  };
}

//-----------------------------------------------------------------------------
// clear output window
//-----------------------------------------------------------------------------
function clear_window(element_id) {
  const el = document.getElementById(element_id);
  el.innerHTML = '';
  el.classList.toggle('force-redraw');
}

//-----------------------------------------------------------------------------
function displayFile(filePath, elementId) {

  let result = null;
  fetch(filePath)
    .then(response => response.text())
    .then(text => {
      log = document.getElementById(elementId);
      
      const lines = text.split('\n');

      for (line of lines) {
        const newline = document.createElement('div');
        newline.textContent = line;
        log.appendChild(newline);
      }
    })
    .catch(err => {
      console.error("Error loading file:", err);
    });
  
  return result;
}

//-----------------------------------------------------------------------------
// cmd.logfile is a stream, it has two files - .msg and .log - associated with it
// normally, read the output of the last command from .msg file
//-----------------------------------------------------------------------------
async function display_result_new(cmd,elementId) {

  const filename = cmd.logfile;
  
  const response = await fetch(`http://localhost:3226/logs?stream=${encodeURIComponent(filename)}`);
  const data     = await response.text();  // .text(), not .json()
    
  const buffer   = document.getElementById(elementId);

  const entry    = document.createElement('div');
  entry.style.borderBottom = "1px solid #222";
  entry.style.padding      = "2px 0";

  if (cmd.name == 'reset_output') {
    buffer.innerText = data;
  }
  else {
    // output of the last command: place it at the TOP of the list
    entry.innerText     = data;
    buffer.prepend(entry);
  }
  
  // Auto-scroll to top so the newest is always visible
  buffer.scrollTop = 0;

  // console.log(e.innerHTML);
}

//-----------------------------------------------------------------------------
// not sure this function works - may need more debugging (or the way I tried to use it
// was wrong
//-----------------------------------------------------------------------------
async function odb_path_exists(path,key) {
  let exists = null;
  const p    = path+'/'+key;
  try {
    let rpc    = await mjsonrpc_db_get_values([p]);
    let res    = rpc.result.data[0];
    if (res != null) {
      exists = 1;
    }
  }
  catch (error) {
    console.error("Error fetching values:", error);
  };
  return exists;
}

//-----------------------------------------------------------------------------
// not sure this function works - may need more debugging (or the way I tried to use it
// was wrong
// in case of failure, returns null
//-----------------------------------------------------------------------------
async function odb_get_active_run_conf_name() {
  const p = '/Mu2e/ActiveRunConfiguration/Name';
  try {
    let rpc = await mjsonrpc_db_get_values([p]);
    let res = rpc.result.data[0];
    return res;
  }
  catch (error) {
    console.error('failed to get /Mu2e/ActiveRunConfiguration/Name ');
    return null;
  }
}

//-----------------------------------------------------------------------------
// REDIRECT BROWSER TO CONTROL PAGES OPENING NEW TABS
//-----------------------------------------------------------------------------
function dtc_control(hostname,pcie) {
//  window.location.href = `dtc_control.html?hostname=${hostname}&pcie=${pcie}`;
  window.open(`dtc_control.html?hostname=${hostname}&pcie=${pcie}`,'_blank');
}

//-----------------------------------------------------------------------------
function artdaq_process_control(hostname,process) {
//  window.location.href = `artdaq_process_control.html?hostname=${hostname}&process=${process}`;
  window.open(`artdaq_process_control.html?hostname=${hostname}&process=${process}`,'_blank');
}

//-----------------------------------------------------------------------------
// the second parameter may not be necessary, keep it for the moment
//-----------------------------------------------------------------------------
function cfo_control(hostname,pcie) {
//  window.location.href = `artdaq_process_control.html?hostname=${hostname}&process=${process}&facility=tfm`;
  window.open(`cfo_control.html?hostname=${hostname}`,'_blank');
}

//-----------------------------------------------------------------------------
function node_control(hostname) {
//  window.location.href = `node_control.html?hostname=${hostname}`;
  window.open(`node_control.html?hostname=${hostname}`,'_blank');
}

//-----------------------------------------------------------------------------
function tfm_control(hostname,process) {
//  window.location.href = `artdaq_process_control.html?hostname=${hostname}&process=${process}&facility=tfm`;
  window.open(`tfm_control.html?hostname=${hostname}&process=${process}`,'_blank');
}

//-----------------------------------------------------------------------------
function cfo_status(hostname) {
  // window.location.href = `cfo_status.html`;
  window.open(`cfo_status.html`,'_blank');
}

//-----------------------------------------------------------------------------
async function fetch_url(url, divId) {
  try {
    const response = await fetch(url);
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    const html = await response.text();
    document.getElementById(divId).innerHTML = html;
  } catch (error) {
    console.error("Failed to load page:", error);
    document.getElementById(divId).innerHTML = "<p>Failed to load content.</p>";
  }
}

//-----------------------------------------------------------------------------
// the difficult part is to determine in which sequence the parameters should be passed
// and how many of them are there
//-----------------------------------------------------------------------------
function make_load_table_button(cmda) {
    let btn     = document.createElement('input');
    btn.type    = 'button'
    btn.value   = cmda.title;
    btn.onclick = function() { cmda.func(cmda.parameter_path,cmda.table_id) ; }
    return btn;
}

//-----------------------------------------------------------------------------
// cmda.func takes only one parameter - cmda
//-----------------------------------------------------------------------------
function make_set_odb_button(cmda) {
    let btn    = document.createElement('input');
    btn.type    = 'button'
    btn.value   = cmda.title;
    btn.onclick = function() { cmda.func(cmda) ; }
    return btn;
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the Command_B command parameter record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function mu2e_command_set_odb_B(cmd) {

  const ppath = cmd.func_parameter_path(cmd);
  
  const paths=[ppath+'/Name',
               ppath+'/ParameterPath',
               ppath+'/Finished',
               ppath+'/logfile',
               ppath+'/Run',                         // better be the last one
  ];

  let logfile = cmd.logfile;
  
  if (logfile == null) { logfile   = g_logfile; }
  else                 { g_logfile = logfile  ; }
  
  try {
    
    let rpc = await mjsonrpc_db_paste(paths, [cmd.name,ppath+'/'+cmd.name,0,logfile,1]);
    let result=rpc.result;	      
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
  
  let finished = 0;

  while (finished == 0) {
      // check whether the command has finished
    const paths=[ppath+'/Run', ppath+'/Finished'];
    sleep(100);
    try {
      let rpc = await mjsonrpc_db_get_values(paths);
      finished = rpc.result.data[1];
    }
    catch(error) {
      mjsonrpc_error_alert(error);
    };
  };
  
  // display the logfile. THis is the only non-generic place
  // if the command was a struct with the logfile being one of its parameters,
  // this function became completely generic

//2026-04-09 PM  displayFile(logfile, 'messageFrame');
  display_result_new(cmd,'messageFrameA');
}

//-----------------------------------------------------------------------------
// load ODB table corresponding to a given 'odb_path' to HTML table with a given 'table_id'
// input: cmd: Command_B
//-----------------------------------------------------------------------------
function mu2e_load_conf_table(cmd) {
  const table     = document.getElementById(cmd.table_id);
  table.innerHTML = '';
  const cmd_parameter_path = cmd.func_parameter_path(cmd); // +`/${cmd.name}`;
  odb_browser(cmd.table_id,cmd_parameter_path,0);
}

// parameter - Command_B
// assume that func_parameter_path points to the object command top directory
// and the command parameter path has an extra '/'+cmd.name in it
//-----------------------------------------------------------------------------
function mu2e_load_cmd_table(cmd) {
  const table     = document.getElementById(cmd.table_id);
  table.innerHTML = '';
  const cmd_parameter_path = cmd.func_parameter_path(cmd)+'/'+cmd.name;
  odb_browser(cmd.table_id,cmd_parameter_path,0);
}

function mu2e_odb_load_table(cmd) {
  const table     = document.getElementById(cmd.table_id);
  table.innerHTML = '';
  const cmd_parameter_path = cmd.func_parameter_path(cmd); // +`/${cmd.name}`;
  odb_browser(cmd.table_id,cmd_parameter_path,0);
}

//-----------------------------------------------------------------------------
function mu2e_load_help_page(cmd) {
  const url = 'https://mu2einternalwiki.fnal.gov/wiki/DTC_control_page#ROC_control_commands';
  window.open(url,'_blank');
}

//-----------------------------------------------------------------------------
function mu2e_make_dropup_button_B(cmd) {
  let btn    = document.createElement('div');
  btn.className = 'dropup';

  let inp       = document.createElement('input');
  inp.type      = 'button';
  inp.className = 'dropbtn';
  inp.value     = cmd.name;
  btn.appendChild(inp);

  let d1       = document.createElement('div');
  d1.className = 'dropup-content';

  const d1_1   = document.createElement('div');
  d1_1.innerHTML = 'Load Parameters'
  // cmd.parameter_path corresponds to the ODB handle which has the command name one of its members
  d1_1.onclick   = function() { mu2e_load_cmd_table(cmd) ; };
  d1.appendChild(d1_1);

  const d1_2   = document.createElement('div');
  d1_2.innerHTML = 'Run'
  d1_2.onclick   = function() { cmd.func(cmd) ; };

  d1.appendChild(d1_2);

  btn.appendChild(d1);
  return btn;
}

//-----------------------------------------------------------------------------
function hideAllMenus() {
  const menus = document.querySelectorAll(".daq_menu");
  menus.forEach(m => {
    m.style.display = "none";
  });
}

//-----------------------------------------------------------------------------
function showMenu(e, menu) {
  e.preventDefault();
  hideAllMenus();
  menu.style.display = "block";
  menu.style.left    = e.pageX + 15 + "px";
  menu.style.top     = e.pageY + 15 + "px";
  menu.style.position = "fixed";   
  e.stopPropagation();
}

//-----------------------------------------------------------------------------
// parameter of type Command_D
//-----------------------------------------------------------------------------
function mu2e_make_dropup_button_D(cmd) {

  let btn = document.createElement('div');
  btn.className = 'btn';
  btn.innerHTML = cmd.name;
  btn.cmd       = cmd;
  // left click
  btn.addEventListener("click", () => cmd.f_handle_left_click(cmd));

  // a command also has a menu
  // Right click - deals with the menu
  const m = document.getElementById(cmd.menu_id);

  let mn = m.cloneNode(m);
  btn.appendChild(mn);
  
  mn.menu_actions = {};
  
  const items = mn.querySelectorAll('div');
  items.forEach(item => {
    if (item.dataset.action == 'load_cmd_par') {
      mn.menu_actions['load_cmd_par'] = mu2e_load_cmd_table;
    }
    else if (item.dataset.action == 'help') {
      mn.menu_actions['help'] = mu2e_load_help_page;
    }
  });
  
  btn.addEventListener("contextmenu", (e) => showMenu(e, mn));
  
  // Menu handler
  mn.addEventListener("click", (e) => {
    const action = e.target.dataset.action;   // 'dataset.action' seems to get translated into 'data-action' in HTML
    if (action && mn.menu_actions[action]) {
      // can pass parameters when it is called
      mn.menu_actions[action](cmd);
      hideAllMenus();
    }
  });
  
  mn.addEventListener("click", e => {
    e.stopPropagation();
  });
  
  btn.addEventListener("contextmenu", (e) => showMenu(e, mn));
  
  return btn;
}

//-----------------------------------------------------------------------------
function mu2e_make_exec_button_B(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.title;
  btn.onclick = function() { cmd.func(cmd) ; }
  return btn;
}

//-----------------------------------------------------------------------------
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
