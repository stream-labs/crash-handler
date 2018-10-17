module.exports = require('./crash_handler.node');

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

function registerProcess(isCritial = false) {
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

function startCrashHandler() {
    const { spawn } = require('child_process');
    const subprocess = spawn('./crash-handler-process.exe', {
      detached: true,
      stdio: 'ignore'
    });
    subprocess.unref();
}

exports.startCrashHandler = startCrashHandler;
exports.registerProcess = registerProcess;
exports.unregisterProcess = unregisterProcess;
exports.terminateCrashHandler = terminateCrashHandler;