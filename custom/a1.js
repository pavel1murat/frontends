(function(){function r(e,n,t){function o(i,f){if(!n[i]){if(!e[i]){var c="function"==typeof require&&require;if(!f&&c)return c(i,!0);if(u)return u(i,!0);var a=new Error("Cannot find module '"+i+"'");throw a.code="MODULE_NOT_FOUND",a}var p=n[i]={exports:{}};e[i][0].call(p.exports,function(r){var n=e[i][1][r];return o(n||r)},p,p.exports,r,e,n,t)}return n[i].exports}for(var u="function"==typeof require&&require,i=0;i<t.length;i++)o(t[i]);return o}return r})()({1:[function(require,module,exports){

},{}],2:[function(require,module,exports){
//  import { execSync } from 'node:child_process';  // replace ^ if using ES modules

function FunctionX() {

    const execSync = require('child_process').execSync;
    
    const output = execSync('ls', { encoding: 'utf-8' });  // the default is 'buffer'
  
    document.getElementById("demo4").innerHTML = "output: " + output;
}

},{"child_process":1}]},{},[2]);
