//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
// global variables
//-----------------------------------------------------------------------------
// import * as fs   from 'fs/promises'
// import * as path from 'path'

let g_node     = 1;             // integer index of the active node
let g_process  = 0;
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
  console.log('g_node=',g_node);
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
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function tfm_command_set_odb(cmd, path) {
  
  var paths=[path+'/Name',
             path+'/ParameterPath',
             path+'/Finished',
             path+'/Run',
  ];
  
  mjsonrpc_db_paste(paths, [cmd,path+'/'+cmd,0,1]).then(function(rpc) {
    result=rpc.result;	      

    // 'Run' is set to 1 , then tfm_frontend is called

   var paths=[ path ];

    mjsonrpc_db_paste(paths, [1]).then(function(rpc) {
      result=rpc.result;	      
      // tfm_frontend waits till command completes and always returns,
      // but with different return codes...
    }).catch(function(error) {
      mjsonrpc_error_alert(error);
    });
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
  
  let done = 0;

  while(done == 0) {
      // check whether the command has finished
    var paths=["/Mu2e/Commands/DAQ/Tfm/Run",
               "/Mu2e/Commands/DAQ/Tfm/Finished"];
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
  
  // I wonder if this line is needed at all....
  displayFile('tfm.log', 'messageFrame');
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
function tfm_make_simple_button(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { tfm_load_table(cmd.table_id,cmd.parameter_path) ; }
  return btn;
}

//-----------------------------------------------------------------------------
function tfm_make_exec_button(cmd) {
  let btn    = document.createElement('input');
  btn.type    = 'button'
  btn.value   = cmd.name;
  btn.onclick = function() { tfm_command_set_odb(cmd.name,cmd.parameter_path) ; }
  return btn;
}

//-----------------------------------------------------------------------------
// how does one know the parameter values ? - assume that print_level and
// run_configuration have already been defined, so only need to set the hostname
// and the artdaq process label
// cmd.parameter_path points to the parameter root directory (Name,ParameterPath,etc) 
//-----------------------------------------------------------------------------
function tfm_load_pars_and_execute(cmd) {
  // step 1 : load parameters
  let ppath = cmd.parameter_path+'/'+cmd.name;

  let node_name     = tfm_get_node_name(g_node);
  let process_label = tfm_get_artdaq_process_label(g_process);
  
  var paths=[ppath+'/host',
             ppath+'/process',
  ];
  
  mjsonrpc_db_paste(paths, [node_name,process_label]).then(function(rpc) {
    result=rpc.result;	      
    
    // OK, ODB parameters set, now execute
    
    tfm_command_set_odb(cmd.name,cmd.parameter_path);
    
  }).catch(function(error) {
    mjsonrpc_error_alert(error);
  });
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
  d1_2.onclick   = function() { tfm_command_set_odb(cmd.name,cmd.parameter_path)};

  d1.appendChild(d1_2);

  btn.appendChild(d1);
  return btn;
}


//-----------------------------------------------------------------------------
function tfm_get_state(element) {
  //  clear_window(element)
  tfm_command_set_odb("get_state")
}

//-----------------------------------------------------------------------------
function tfm_reset_output(element) {
//  clear_window(element)
  tfm_command_set_odb("reset_output")
}

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    mode: web
//    End:
