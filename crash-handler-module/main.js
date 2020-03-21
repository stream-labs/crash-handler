"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const crash_handler = require('./crash-handler.node');
const net = require('net');
const fs = require('fs');

function tryConnect(buffer, attempt = 5, waitMs = 100) {
    try {
      const fd = fs.openSync('/tmp/slobs-crash-handler',
        fs.constants.O_WRONLY | fs.constants.O_DSYNC);

        const socket = new net.Socket({ fd });
        console.log('Writing');
        socket.write(buffer);
    } catch (error) {
      if (attempt > 0) {
        setTimeout(() => {
          tryConnect(buffer, attempt - 1, waitMs * 2);
        }, waitMs);
      }
    }
}

function registerProcess(pid, isCritial = false) {
    console.log('register process');
    const buffer = new Buffer(6);
    let offset = 0;
    buffer.writeUInt8(0, offset++);
    buffer.writeUInt8(isCritial, offset++);
    buffer.writeUInt32LE(pid, offset++);

    tryConnect(buffer);
}

function unregisterProcess(pid) {
    console.log('unregister process');
    const buffer = new Buffer(5);
    let offset = 0;
    buffer.writeUInt8(1, offset++);
    buffer.writeUInt32LE(pid, offset++);
    
    tryConnect(buffer);
}

function terminateCrashHandler(pid) {
    const buffer = new Buffer(5);
    let offset = 0;
    buffer.writeUInt8(2, offset++);
    buffer.writeUInt32LE(pid, offset++);

    tryConnect(buffer);
  }

function startCrashHandler(workingDirectory, version, isDevEnv, cachePath = "") {
    const { spawn } = require('child_process');

    try {
      fs.unlinkSync('/tmp/slobs-crash-handler');
    } catch (error) {}

    const processPath = workingDirectory.replace('app.asar', 'app.asar.unpacked') +
    '/node_modules/crash-handler';

    let spawnArguments = [workingDirectory, version, isDevEnv];

    if ( cachePath.length > 0 ) 
      spawnArguments.push( cachePath+'/crash-handler.log' );

    const subprocess = spawn(processPath +
      '/crash-handler-process', spawnArguments, {
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