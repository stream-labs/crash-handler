"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const crash_handler = "loading";//require('./crash-handler.node');
const net = require('net');

function tryConnect(buffer, attempt = 5, waitMs = 100) {
  const socket = new net.Socket();
  socket.on('connect', () => {
  });
  socket.on('ready', () => {
    socket.write(buffer);
  });
  socket.on('data', (in_data) => {
    socket.end();
    socket.destroy();
    socket.unref();
  });
  socket.on('error', (err) => {
    socket.destroy();
    socket.unref();
    if (attempt > 0) {
      setTimeout(() => {
        tryConnect(buffer, attempt - 1, waitMs * 2);
      }, waitMs);
    }
  });
  socket.connect('\\\\.\\pipe\\slobs-crash-handler');
}

async function registerProcess(pid, isCritial = false) {
    const buffer = new Buffer(6);
    let offset = 0;
    buffer.writeUInt8(0, offset++);
    buffer.writeUInt8(isCritial, offset++);
    buffer.writeUInt32LE(pid, offset++);
    tryConnect(buffer);
}

async function unregisterProcess(pid) {
    const buffer = new Buffer(5);
    let offset = 0;
    buffer.writeUInt8(1, offset++);
    buffer.writeUInt32LE(pid, offset++);
    
    tryConnect(buffer);
}

async function terminateCrashHandler(pid) {
    const buffer = new Buffer(5);
    let offset = 0;
    buffer.writeUInt8(2, offset++);
    buffer.writeUInt32LE(pid, offset++);
    tryConnect(buffer);
}

function startCrashHandler(workingDirectory, version, isDevEnv, cachePath = "") {
    const { spawn } = require('child_process');

    const processPath = workingDirectory.replace('app.asar', 'app.asar.unpacked') +
    '\\node_modules\\crash-handler';

    let spawnArguments = [workingDirectory, version, isDevEnv];

    if ( cachePath.length > 0 ) 
      spawnArguments.push( cachePath+'\\crash-handler.log' );

    const subprocess = spawn(processPath +
      '\\crash-handler-process.exe', spawnArguments, {
      detached: true,
      stdio: 'ignore'
    });

    subprocess.on('error', (error) => {
      console.log('Error spawning');
      console.log('Error : ' + error)
    });

    subprocess.unref();
}

exports.startCrashHandler = startCrashHandler;
exports.registerProcess = registerProcess;
exports.unregisterProcess = unregisterProcess;
exports.terminateCrashHandler = terminateCrashHandler;

exports = crash_handler;