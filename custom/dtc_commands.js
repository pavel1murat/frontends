//-----------------------------------------------------------------------------
// the hostname should be the same within the scope of this script
//-----------------------------------------------------------------------------
      let g_hostname = 'undefined';
      let g_pcie     = 0;
      let g_roc      = 0;
//-----------------------------------------------------------------------------
// CFO frontend : the name shoudl be fixed! 
//-----------------------------------------------------------------------------
      function cfo_command(cmd) {
        let msg = { "client_name":"cfo_emu_fe", "cmd":cmd, "args":'{}'};
        mjsonrpc_call("jrpc",msg).then(function(rpc1) {
          let s = rpc1.result.reply;
          const regex = /\n/gi
          let y = s.replaceAll(regex,'<br>');

          const el = document.getElementById("content");
          el.innerHTML += y;
          el.style.fontFamily = 'monospace';
          el.scrollIntoView({ behavior: 'smooth', block: 'end' });

        }).catch(function(error){
          mjsonrpc_error_alert(error);
        });
      }

//-----------------------------------------------------------------------------
      function dtc_command(cmd) {
        let msg = { "client_name":g_hostname, "cmd":cmd, "max_reply_length":10000,
                    "args":'{"pcie":'+g_pcie.toString()+',"roc":'+g_roc.toString()+'}'};
        mjsonrpc_call("jrpc",msg).then(function(rpc1) {
          let s = rpc1.result.reply
          console.log(s.length);
          let y = '<br>'+s.replaceAll(/\n/gi,'<br>').replace(/ /g, '&nbsp;');

          const el = document.getElementById("content");
          el.innerHTML += y;
          el.style.fontFamily = 'monospace';
          // el.scrollIntoView();
          const scrollToBottom = (id) => {
            el.scrollTop = el.scrollHeight;
          }

        }).catch(function(error){
          mjsonrpc_error_alert(error);
        });
      }
//-----------------------------------------------------------------------------
// load table with the DTC parameters
//-----------------------------------------------------------------------------
      function dtc_load_parameters() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params',`/Mu2e/ActiveRunConfiguration/DAQ/Nodes/${g_hostname}/DTC${g_pcie}`,0);
      }

//-----------------------------------------------------------------------------
      function dtc_load_parameters_control_roc_read() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_ROC_read',0);
      }
      
//-----------------------------------------------------------------------------
      function dtc_load_parameters_control_roc_digi_rw() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_ROC_digi_rw',0);
      }
      
//-----------------------------------------------------------------------------
      function dtc_load_parameters_control_roc_pulser_on() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_ROC_pulser_on',0);
      }
      
//-----------------------------------------------------------------------------
      function dtc_load_parameters_control_roc_rates() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_ROC_rates',0);
      }
      
//-----------------------------------------------------------------------------
      function dtc_load_parameters_control_roc_pulser_off() {
        const table     = document.getElementById('cmd_params');
        table.innerHTML = '';
        odb_browser('cmd_params','/Mu2e/Commands/Tracker/DTC/control_ROC_pulser_off',0);
      }
      
//-----------------------------------------------------------------------------      
      function choose_dtc_id(evt, dtc_id) {
        var i, tablinks;
        tablinks = document.getElementsByClassName("tablinks");
        for (i=0; i<tablinks.length; i++) {
          tablinks[i].className = tablinks[i].className.replace(" active", "");
        }
        document.getElementById(dtc_id).style.display = "block";
        evt.currentTarget.className += " active";

        if (dtc_id == 'dtc0') { g_pcie = 0; } else {g_pcie = 1;} ;
        console.log('g_pcie=',g_pcie);
      }
//-----------------------------------------------------------------------------      
      function choose_roc_id(evt, roc_id) {
        var i, roctabs;
        roctabs = document.getElementsByClassName("roctabs");
        for (i=0; i<roctabs.length; i++) {
          roctabs[i].className = roctabs[i].className.replace(" active", "");
        }
        document.getElementById(roc_id).style.display = "block";
        evt.currentTarget.className += " active";

        g_roc = Number(roc_id.charAt(3)); // forth character: 'rocX'
        // -1: all ROCs
        if (g_roc == 6) {g_roc = -1};
        console.log('g_roc=',g_roc);
      }
