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

// class Command_A {
//   constructor(title,name,func,table_id,parameter_path) {
//     this.title          = title;
//     this.name           = name;
//     this.func           = func;
//     this.table_id       = table_id;
//     this.parameter_path = parameter_path;
//   }
// }

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
function artdaq_process_config_path(cmd) {
  const path = `/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/Artdaq/${g_process_label}`;
  return path;
}

//-----------------------------------------------------------------------------
function node_artdaq_parameter_path(hostname) {
  return `/Mu2e/Commands/DAQ/Nodes/${hostname}/Artdaq`;
}

//-----------------------------------------------------------------------------
function tfm_cmd_parameter_path   (hostname) { return `/Mu2e/Commands/DAQ/Tfm`; }
function tfm_config_parameter_path(hostname) { return `/Mu2e/ActiveRunConfiguration/DAQ/Tfm`; }

//-----------------------------------------------------------------------------
function test_cmd_parameter_path(cmd) { return `/Mu2e/Commands/Test`; }

//-----------------------------------------------------------------------------
function trk_config_path(cmd) { return `/Mu2e/ActiveRunConfiguration/Tracker`; }
function trk_cmd_path   (cmd) { return `/Mu2e/Commands/Tracker`; }

//-----------------------------------------------------------------------------
function trk_panel_config_path(cmd) {
  const stn = g_station.toString().padStart(2,'0');
  const pln = g_plane.toString().padStart(2,'0');
  const pnl = g_panel.toString().padStart(2,'0');
  const path = `/Mu2e/ActiveRunConfiguration/Tracker/Station_${stn}/Plane_${pln}/Panel_${pnl}`;
  return path;
}

//-----------------------------------------------------------------------------
// common javascript functions
// DAQ colors. Each element has 'Enabled' and 'Status' field
//-----------------------------------------------------------------------------
function set_colors(path, cell) {
  // Fetch values for Enabled and Status
  const path_enabled = path+`/Enabled`;
  const path_status  = path+`/Status`;
  mjsonrpc_db_get_values([path_enabled, path_status]).then(function (rpc) {
    let enabled = rpc.result.data[0];
    let status = rpc.result.data[1];
    
    // Apply color styles
    if (enabled === 0) {
      cell.style.backgroundColor = "gray";
      cell.style.color = "white";
    } else if (enabled === 1) {
      if (status === 0) {
        cell.style.backgroundColor = "green";
        cell.style.color = "white";
      } else if (status < 0) {
        cell.style.backgroundColor = "red";
        cell.style.color = "white";
      } else if (status > 0) {
        cell.style.backgroundColor = "yellow";
        cell.style.color = "black";
      }
    } else {
      cell.style.backgroundColor = "gray";
      cell.style.color = "white";
    }
  }).catch(function (error) {
    console.error("Error fetching values:", error);
  });
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
async function writeToLog(filePath,textLine) {
  fetch(filePath,{
      method : 'POST',
      headers: {'Content-Type': 'application/json'},
      body   : JSON.stringify({ content: textLine })
  }).then(res => res.json())
    .then(result => console.log("Server says:",result))
    .catch(err =>console.error("Failed:",err));
}

//-----------------------------------------------------------------------------
// load ODB table corresponding to a given 'odb_path' to HTML table with a given 'table_id'
//-----------------------------------------------------------------------------
function odb_load_table(odb_path,table_id) {
  const table     = document.getElementById(table_id);
  table.innerHTML = '';
  odb_browser(table_id,odb_path,0);
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
function node_control(hostname) {
//  window.location.href = `node_control.html?hostname=${hostname}`;
  window.open(`node_control.html?hostname=${hostname}`,'_blank');
}

//-----------------------------------------------------------------------------
function tfm_control(hostname,process) {
//  window.location.href = `artdaq_process_control.html?hostname=${hostname}&process=${process}`;
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
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmda.title;
//  btn.onclick = function() { odb_load_table(cmda.parameter_path,cmda.table_id) ; }
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
//  btn.onclick = function() { odb_load_table(cmda.parameter_path,cmda.table_id) ; }
  btn.onclick = function() { cmda.func(cmda) ; }
  return btn;
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the Command_B command parameter record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function mu2e_command_set_odb_B(cmd) {

  const ppath = cmd.func_parameter_path(g_hostname);
  
  const paths=[ppath+'/Name',
               ppath+'/ParameterPath',
               ppath+'/Finished',
               ppath+'/Run',
               ppath+'/logfile',
  ];

  let logfile = cmd.logfile;
  
  if (logfile == null) { logfile   = g_logfile; }
  else                 { g_logfile = logfile  ; }
  
  try {
    
    let rpc = await mjsonrpc_db_paste(paths, [cmd.name,ppath+'/'+cmd.name,0,1,logfile]);
    let result=rpc.result;	      
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    const paths=[ppath+'/Run', ppath+'/Finished'];
    let run      = 1;
    let finished = 0;
    sleep(100);
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
// load ODB table corresponding to a given 'odb_path' to HTML table with a given 'table_id'
// input: cmd: Command_B
//-----------------------------------------------------------------------------
function mu2e_odb_load_table(cmd) {
  const table     = document.getElementById(cmd.table_id);
  table.innerHTML = '';
  odb_browser(cmd.table_id,cmd.func_parameter_path(cmd),0);
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
  d1_1.onclick   = function() { mu2e_odb_load_table(cmd) ; };
  d1.appendChild(d1_1);

  const d1_2   = document.createElement('div');
  d1_2.innerHTML = 'Run'
  d1_2.onclick   = function() { cmd.func(cmd) ; };

  d1.appendChild(d1_2);

  btn.appendChild(d1);
  return btn;
}

//-----------------------------------------------------------------------------
function mu2e_make_exec_button_B(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.title;
  btn.onclick = function() { cmd.func(cmd) ; }
  // btn.onclick = test_mu2e_odb_load_table(cmd); 
  return btn;
}

//-----------------------------------------------------------------------------
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
