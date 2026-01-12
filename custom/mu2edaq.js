//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// could think of having a cmd.name and cmd.title, but in absense of pointers 
// that could complicate things
// parameter_path points to the path where the name, parameter_path, and Finished
// should be stored
//-----------------------------------------------------------------------------
class Command {
  constructor(name,table_id,parameter_path) {
    this.name           = name;
    this.table_id       = table_id;
    this.parameter_path = parameter_path;
  }
}

class Command_A {
  constructor(title,name,func,table_id,parameter_path) {
    this.title          = title;
    this.name           = name;
    this.func           = func;
    this.table_id       = table_id;
    this.parameter_path = parameter_path;
  }
}

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
function node_artdaq_parameter_path(hostname) {
  return `/Mu2e/Commands/DAQ/Nodes/${hostname}/Artdaq`;
}

//-----------------------------------------------------------------------------
function tfm_cmd_parameter_path(hostname) { return `/Mu2e/Commands/DAQ/Tfm`; }

//-----------------------------------------------------------------------------
// common javascript functions
// DAQ colors. Each element has 'Enabled' and 'Status' field
//-----------------------------------------------------------------------------
function set_colors(path, statusCell) {
  // Fetch values for Enabled and Status
  const path_enabled = path+`/Enabled`;
  const path_status  = path+`/Status`;
  mjsonrpc_db_get_values([path_enabled, path_status]).then(function (rpc) {
    let enabled = rpc.result.data[0];
    let status = rpc.result.data[1];
    
    // Apply color styles
    if (enabled === 0) {
      statusCell.style.backgroundColor = "gray";
      statusCell.style.color = "white";
    } else if (enabled === 1) {
      if (status === 0) {
        statusCell.style.backgroundColor = "green";
        statusCell.style.color = "white";
      } else if (status < 0) {
        statusCell.style.backgroundColor = "red";
        statusCell.style.color = "white";
      } else if (status > 0) {
        statusCell.style.backgroundColor = "yellow";
        statusCell.style.color = "black";
      }
    } else {
      statusCell.style.backgroundColor = "gray";
      statusCell.style.color = "white";
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
      // document.getElementById(elementId).textContent = text;

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
function tfm_control(hostname) {
//  window.location.href = `tfm_control.html?hostname=${hostname}&facility=tfm`;
  window.open(`tfm_control.html?hostname=${hostname}&facility=tfm`,'_blank');
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
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
