"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const crash_handler = require('./crash-handler.node');
const net = require('net');

function tryConnect(buffer, attempt = 5, waitMs = 100) {
    const socket = net.connect('\\\\.\\pipe\\slobs-crash-handler');
     socket.on('connect', () => {
      socket.write(buffer);
      socket.destroy();
      socket.unref();
    });
     socket.on('error', () => {
      socket.destroy();
      socket.unref();
      if (attempt > 0) {
        setTimeout(() => {
          tryConnect(buffer, attempt - 1, waitMs * 2);
        }, waitMs);
      }
    });
}

function registerProcess(pid, isCritial = false) {
    const buffer = new Buffer(512);
    let offset = 0;
    buffer.writeUInt32LE(0, offset++);
    buffer.writeUInt32LE(isCritial, offset++);
    buffer.writeUInt32LE(pid, offset++);
  
    tryConnect(buffer);
}

function unregisterProcess(pid) {
    const buffer = new Buffer(512);
    let offset = 0;
    buffer.writeUInt32LE(1, offset++);
    buffer.writeUInt32LE(pid, offset++);
    
    tryConnect(buffer);
}

function terminateCrashHandler(pid) {
    const buffer = new Buffer(512);
    let offset = 0;
    buffer.writeUInt32LE(2, offset++);
    buffer.writeUInt32LE(pid, offset++);

    tryConnect(buffer);
  }

function startCrashHandler(workingDirectory) {
    const { spawn } = require('child_process');

    const processPath = workingDirectory.replace('app.asar', 'app.asar.unpacked') +
    '\\node_modules\\crash-handler';

    const subprocess = spawn(processPath +
      '\\crash-handler-process.exe', [workingDirectory], {
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