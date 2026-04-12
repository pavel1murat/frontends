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


let g_log_dir = null;
//-----------------------------------------------------------------------------
// log file handling
// for the server to be generic, it needs to get the directory name from somewhere
//-----------------------------------------------------------------------------
app.get('/logs', (req, res) => {
  // 1. Get the stream from the URL
  const stream = req.query.stream;

//  console.log("Method :", req.method);
//  console.log("URL    :", req.url);
//  console.log("Headers:", req.headers);

  if (!stream) {
    return res.status(400).send({ error: 'stream is required' });
  }

  if (! g_log_dir) {
    // fetch that from the environment
    const exptab    = process.env.MIDAS_EXPTAB;
    const expt_name = process.env.MIDAS_EXPT_NAME;
    const lines     = fs.readFileSync(exptab, 'utf8').split('\n');
    const line      = lines.find(line => line.trim().startsWith(expt_name));
    const words     = line.trim().split(/\s+/);
    g_log_dir       = path.join(words[1],'logs');
    console.log('g_log_dir = '+g_log_dir);
  }

  const bn = path.basename(stream)+'.msg';
  const fn = path.join(g_log_dir,bn);

//  console.log(`reading file:${fn}`);
  
  fs.readFile(fn, "utf8", (err, data) => {
    if (err) {
      res.writeHead(500, { "Content-Type": "text/plain" });
      res.end("Error reading "+fn);
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
