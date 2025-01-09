// Utility function to fetch and style status values
function loadAndStyleValues(paths, statusCell) {
    mjsonrpc_db_get_values(paths)
        .then(function (rpc) {
            let [enabled, status] = rpc.result.data;

            if (enabled === 0) {
                statusCell.style.backgroundColor = "gray";
                statusCell.style.color = "white";
                statusCell.textContent = "Disabled";
            } else if (enabled === 1) {
                if (status === 1) {
                    statusCell.style.backgroundColor = "green";
                    statusCell.style.color = "white";
                    statusCell.textContent = "Enabled";
                } else if (status === 0) {
                    statusCell.style.backgroundColor = "red";
                    statusCell.style.color = "white";
                    statusCell.textContent = "Error";
                } else if (status === -1) {
                    statusCell.style.backgroundColor = "yellow";
                    statusCell.style.color = "black";
                    statusCell.textContent = "Warning";
                } else if (status == null){
                    statusCell.style.backgroundColor = "white";
                }

            } else {
                statusCell.style.backgroundColor = "white";
                statusCell.style.color = "black";
                statusCell.textContent = "Unknown";
            }
        })
        .catch(function (error) {
            console.error("Error fetching values:", error);
            statusCell.textContent = "Fetch Error";
        });
}

// Function to redirect to another page with parameters
function redirectToPage(url, params = {}) {
    if (!url) {
        console.error("Invalid URL provided for redirection");
        return;
    }
    const queryString = new URLSearchParams(params).toString();
    window.location.href = queryString ? `${url}?${queryString}` : url;
}

// Table Creation Logic
function createTable(containerSelector, headers, rowsCallback) {
    const container = document.querySelector(containerSelector);
    if (!container) {
        console.error(`Container ${containerSelector} not found.`);
        return;
    }

    // Create table structure
    container.innerHTML = `
        <table class="mtable">
            <thead>
                <tr>${headers.map(header => `<th>${header}</th>`).join("")}</tr>
            </thead>
            <tbody></tbody>
        </table>
    `;

    const tbody = container.querySelector("tbody");

    // Populate rows using the callback
    rowsCallback(tbody);
}
