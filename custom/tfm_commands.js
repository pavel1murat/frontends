//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
// global variables
//-----------------------------------------------------------------------------
// import * as fs   from 'fs/promises'
// import * as path from 'path'

let g_node     = 1;             // integer index of the active node
let g_process  = 0;
let g_run_conf = 'undefined'
  
//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function tfm_get_node_name(index) {

  let node_name = null;
  let tabs = document.getElementsByClassName("nodetabs");
  for (let i=0; i<tabs.length; i++) {
    let node_index = Number(tabs[i].id.substring(4,7));
    if (node_index == index) {
      node_name = tabs[i].innerText;
      break;
    }
  }

  return node_name;
}

//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function tfm_choose_node_id(evt, id) {
  var i, tabs;
  tabs = document.getElementsByClassName("nodetabs");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  document.getElementById(id).style.display = "block";
  evt.currentTarget.className += " active";

  let num    = id.substring(4,7);
  g_node = Number(num); // parse the number out of 'nodeXXX'
  g_hostname = tfm_get_node_name(g_node);
  console.log(`g_node=${g_node} g_hostname:${g_hostname}`);
}

//-----------------------------------------------------------------------------
// 'createTable' is a function loading the DTC control page
//-----------------------------------------------------------------------------
function tfm_update_node_id(evt, id) {
  tfm_choose_node_id(evt,id);
  updateNodeTable(g_node);
}

//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function tfm_choose_artdaq_process_id(evt, id) {
  var i, tabs;
  tabs = document.getElementsByClassName("process_tabs");
  for (i=0; i<tabs.length; i++) {
    tabs[i].className = tabs[i].className.replace(" active", "");
  }
  document.getElementById(id).style.display = "block";
  evt.currentTarget.className += " active";

  let num   = id.substring(13,14);
  g_process = Number(num);           // parse number out of 'process_btn_XX'
  console.log('g_node=',g_node);
}


//-----------------------------------------------------------------------------
function tfm_cmd_parameter_path(cmd) {
  return '/Mu2e/Commands/DAQ/Tfm';
}

//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function tfm_get_artdaq_process_label(id) {

  let   label = "undefined";
  const eid   = 'process_btn_'+id.toString().padStart(2,'0');
  const tabs  = document.getElementsByClassName("process_tabs");
  
  for (let i=0; i<tabs.length; i++) {
    if (tabs[i].id == eid) {
      label = tabs[i].innerText;
      break;
    }
  }
//  g_process = Number(num);           // parse number out of 'process_btn_XX'
//  console.log('g_node=',g_node);
  return label;
}


//-----------------------------------------------------------------------------
function tfm_get_odb_object(path) {
  let result = null;
  mjsonrpc_db_get_values([path]).then(function(rpc) {
    result      = rpc.result.data[0];
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  return result;
};

//-----------------------------------------------------------------------------
async function tfm_get_list_of_nodes() {
  const  paths=["/Mu2e/ActiveRunConfiguration/DAQ/Nodes",];
  let    result = null;
  mjsonrpc_db_get_values(paths).then(function(rpc) {
    result      = rpc.result.data[0];
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  return result;
};

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function tfm_command_set_odb(cmd) {
  
  var paths=[cmd.parameter_path+'/Name',
             cmd.parameter_path+'/ParameterPath',
             cmd.parameter_path+'/Finished',
             cmd.parameter_path+'/Run',
  ];

  try {
    let rpc = await mjsonrpc_db_paste(paths, [cmd.name,cmd.parameter_path+'/'+cmd.name,0,1]);
    let result=rpc.result;	      
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=[cmd.parameter_path+'/Run', cmd.parameter_path+'/Finished'];
    let run      = 1;
    let finished = 0;
    sleep(500);
    try {
      let rpc  = await mjsonrpc_db_get_values(paths);
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
  displayFile('artdaq.log', 'messageFrame');
}

//-----------------------------------------------------------------------------
// and this one updates ODB
// the command parameters record is expected to be in path+${cmd}
//-----------------------------------------------------------------------------
async function tfm_command_set_odb_B(cmd) {

  let ppath = cmd.func_parameter_path(g_hostname);
  
  var paths=[ppath+'/Name',
             ppath+'/ParameterPath',
             ppath+'/Finished',
             ppath+'/Run',
  ];

  try { 
    let rpc = await mjsonrpc_db_paste(paths, [cmd.name,ppath+'/'+cmd.name,0,1]);
    let result=rpc.result;	      
  }
  catch(error) {
    mjsonrpc_error_alert(error);
  };
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=[ppath+'/Run', ppath+'/Finished'];
    let run      = 1;
    let finished = 0;
    sleep(500);
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
  displayFile('artdaq.log', 'messageFrame');
}

//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
function tfm_load_table(table_id,odb_path) {
  const table     = document.getElementById(table_id);
  table.innerHTML = '';
  odb_browser(table_id,odb_path,0);
}

//-----------------------------------------------------------------------------
// how does one know the parameter values ? - assume that print_level and
// run_configuration have already been defined, so only need to set the hostname
// and the artdaq process label
// cmd.parameter_path points to the parameter root directory (Name,ParameterPath,etc) 
//-----------------------------------------------------------------------------
async function tfm_load_pars_and_execute(cmd) {
  // step 1 : load parameters
  let ppath = cmd.parameter_path+'/'+cmd.name;

  let node_name       = tfm_get_node_name(g_node);
  let process_label   = tfm_get_artdaq_process_label(g_process);
  let active_run_conf = await odb_get_active_run_conf_name();
  
  var paths=[ppath+'/host',
             ppath+'/process',
             ppath+'/run_conf',
  ];
  
//  mjsonrpc_db_paste(paths, [node_name,process_label,active_run_conf]).then(function(rpc) {
  mjsonrpc_db_paste(paths, [g_hostname,process_label,active_run_conf]).then(function(rpc) {
      result=rpc.result;	      
      
      // OK, ODB parameters set, now execute
      
      tfm_command_set_odb(cmd);
      
    }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
}

//-----------------------------------------------------------------------------
function tfm_make_dropup_button(cmd) {
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
  d1_1.onclick   = function() { tfm_load_table(cmd.table_id,cmd.parameter_path+'/'+cmd.name)};
  d1.appendChild(d1_1);

  const d1_2   = document.createElement('div');
  d1_2.innerHTML = 'Run'
  d1_2.onclick   = function() { tfm_command_set_odb(cmd)};

  d1.appendChild(d1_2);

  btn.appendChild(d1);
  return btn;
}

//-----------------------------------------------------------------------------
function tfm_make_exec_button(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { tfm_command_set_odb(cmd) ; }
  return btn;
}

//-----------------------------------------------------------------------------
// input: Command_B
//-----------------------------------------------------------------------------
function tfm_make_dropup_button_B(cmd) {
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
  d1_1.onclick   = function() { tfm_load_table(cmd.table_id,cmd.funparameter_path+'/'+cmd.name)};
  d1.appendChild(d1_1);

  const d1_2   = document.createElement('div');
  d1_2.innerHTML = 'Run'
  d1_2.onclick   = function() { cmd.func(cmd)};

  d1.appendChild(d1_2);

  btn.appendChild(d1);
  return btn;
}

//-----------------------------------------------------------------------------
// input: Command_B
//-----------------------------------------------------------------------------
function tfm_make_exec_button_B(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { cmd.func(cmd) ; }
  return btn;
}

//-----------------------------------------------------------------------------
// this buttom sets all parameters in ODB and then executes - different onclick method
//-----------------------------------------------------------------------------
function tfm_make_exec_button_par(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { tfm_load_pars_and_execute(cmd) ; }
  return btn;
}

//-----------------------------------------------------------------------------
function tfm_make_simple_button(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { tfm_load_table(cmd.table_id,cmd.parameter_path) ; }
  return btn;
}

//    emacs
//    Local Variables:
//    mode: web
//    End:
