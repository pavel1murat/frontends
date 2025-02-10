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
// redirects browser to the DTC control page
//-----------------------------------------------------------------------------
function dtc_control(hostname,pcie) {
  window.location.href = `dtc_control.html?hostname=${hostname}&pcie=${pcie}`;
}

//-----------------------------------------------------------------------------
// redirects browser to the ARTDAQ process control page
//-----------------------------------------------------------------------------
function artdaq_process_status(hostname,process) {
  window.location.href = `artdaq_process_status.html?hostname=${hostname}&process=${process}`;
}

//-----------------------------------------------------------------------------
// redirects to the node page
//-----------------------------------------------------------------------------
function node_status(hostname) {
  window.location.href = `node_status.html?hostname=${hostname}`;
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


// emacs
// Local Variables:
// mode: js
// tab-width: 8
// c-basic-offset: 2
// js-indent-level: 0
// indent-tabs-mode: nil
// End:
