//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
// global variables
//-----------------------------------------------------------------------------
let g_node    = 1;   // integer index of the active node
let g_process = 0;
//-----------------------------------------------------------------------------
// could think of having a cmd.name and cmd.title, but in absense of pointers 
// that could complicate things
//-----------------------------------------------------------------------------
class Command {
  constructor(name,table_id,parameter_root) {
    this.name           = name;
    this.table_id       = table_id;
    this.parameter_path = parameter_root+'/'+name;
  }
}
  
//-----------------------------------------------------------------------------      
// node id = 'nodeXXX' - provide for 1000 nodes
//-----------------------------------------------------------------------------
function tfm_get_node_name(index) {

  let tabs = document.getElementsByClassName("nodetabs");
  let node_name = tabs[index-1].innerHTML.toLowerCase();

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
// and this one updates ODB
// the command parameters record is expected to be in /Mu2e/Commands/Tracker/TRK/${cmd}
// TEquipmentTracker will finally update /Finished
//-----------------------------------------------------------------------------
function tfm_command_set_odb(cmd, path) {
  
  var paths=["/Mu2e/Commands/DAQ/Tfm/Name",
             "/Mu2e/Commands/DAQ/Tfm/ParameterPath",
             "/Mu2e/Commands/DAQ/Tfm/Finished",
             "/Mu2e/Commands/DAQ/Tfm/get_state/print_level",
  ];
  
  mjsonrpc_db_paste(paths, [cmd,"/Mu2e/Commands/DAQ/Tfm",0,1]).then(function(rpc) {
    result=rpc.result;	      

    // 'Run' is set to 1 , then tfm_frontend is called

    var paths=["/Mu2e/Commands/DAQ/Tfm/Run"];
  
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
  displayFile('tfm.log', 'output_window');
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
  d1_1.onclick   = function() { tfm_load_table(cmd.table_id,cmd.parameter_path)};
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
function tfm_clear_window(element) {
//  clear_window(element)
  tfm_command_set_odb("reset_output")
}

//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
//function tfm_load_parameters() {
//  const table     = document.getElementById('cmd_params');
//  table.innerHTML = '';
//  odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Tfm`,0);
//}
//
//-----------------------------------------------------------------------------
// function tfm_load_parameters_get_state() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/get_state',0);
// }
//       
//-----------------------------------------------------------------------------
// function tfm_load_parameters_generate_fcl() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/generate_fcl',0);
// }
//       
//-----------------------------------------------------------------------------
// function tfm_load_parameters_print_fcl() {
//   const table     = document.getElementById('cmd_params');
//   table.innerHTML = '';
//   odb_browser('cmd_params','/Mu2e/Commands/DAQ/Tfm/print_fcl',0);
// }
//       

// ${ip.toString().padStart(2,'0')
//    emacs
//    Local Variables:
//    mode: web
//    End:
