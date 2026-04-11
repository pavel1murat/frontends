//-----------------------------------------------------------------------------
// node-js server : for now, provides access to the log files to "custom" javascript
//                  interacting with mhttpd
//-----------------------------------------------------------------------------
const http    = require("http");
const fs      = require("fs");
const path    = require("path");

const express = require('express');
const cors    = require('cors');         // 1. Import cors
const app     = express();
app.use(cors());                         // 2. Enable cors for all routes


const g_log_dir = null;
//-----------------------------------------------------------------------------
// log file handling
// for the server to be generic, it needs to get the directory name from somewhere
//-----------------------------------------------------------------------------
app.get('/logs', (req, res) => {
  // 1. Get the filename from the URL
  const filename = req.query.name;

//  console.log("Method :", req.method);
//  console.log("URL    :", req.url);
//  console.log("Headers:", req.headers);

  if (!filename) {
    return res.status(400).send({ error: 'Filename is required' });
  }

  console.log(process.env.MIDAS_EXPTAB);

  if (! g_log_dir) {
    // fetch that from the environment
  }

  const bn = path.basename(filename);
  //  const filePath = path.join(__dirname, 'logs', safeName);
  
  const filePath = path.join('/home/mu2etrk/test_stand/experiments/test_025/', 'logs', bn);

  console.log(`reading file:${filePath}`);
  
  fs.readFile(filePath, "utf8", (err, data) => {
    if (err) {
      res.writeHead(500, { "Content-Type": "text/plain" });
      res.end("Error reading file");
      return;
    }
//    console.log(data);
    
    res.writeHead(200, { "Content-Type": "text/plain" });
    res.end(data);
  });

});


app.listen(3226, () => {
  console.log("Server running at http://localhost:3226");
});
