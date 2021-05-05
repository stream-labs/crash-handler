"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const crash_handler = require('./crash-handler.node');
const net = require('net');
const fs = require('fs');

let socket_name = '';

function tryConnect(buffer, attempt = 5, waitMs = 100) {
    try {
      const fd = fs.openSync(socket_name,
        fs.constants.O_WRONLY | fs.constants.O_DSYNC);

        const socket = new net.Socket({ fd });
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
    console.log('[crash-handler] Register process ' + pid);
    const buffer = new Buffer.alloc(6);
    let offset = 0;
    buffer.writeUInt8(0, offset++);
    buffer.writeUInt8(isCritial, offset++);
    buffer.writeUInt32LE(pid, offset++);

    tryConnect(buffer);
}

function unregisterProcess(pid) {
    console.log('[crash-handler] Unregister process' + pid);
    const buffer = new Buffer.alloc(5);
    let offset = 0;
    buffer.writeUInt8(1, offset++);
    buffer.writeUInt32LE(pid, offset++);
    
    tryConnect(buffer);
}

async function terminateCrashHandler(pid) {
    console.log('[crash-handler] terminateCrashHandler');
    const buffer = new Buffer.alloc(5);
    let offset = 0;
    buffer.writeUInt8(3, offset++);
    buffer.writeUInt32LE(pid, offset++);
    tryConnect(buffer);
}

function startCrashHandler(workingDirectory, version, isDevEnv, cachePath = "", socket_prefix = "") {
    console.log('[crash-handler] Spawn crash handler');
    const { spawn } = require('child_process');

    if (socket_prefix.length > 0 ) {
      socket_name = process.platform === "win32" ?
      '\\\\.\\pipe\\'+socket_prefix+'-crash-handler' :
      '/tmp/'+socket_prefix+'-crash-handler'
    } else {
      socket_name = process.platform === "win32" ?
      '\\\\.\\pipe\\slobs-crash-handler' :
      '/tmp/slobs-crash-handler'
    }

    try {
      fs.unlinkSync(socket_name);
    } catch (error) {}

    const processPath = workingDirectory.replace('app.asar', 'app.asar.unpacked') +
    '/node_modules/crash-handler';

    let spawnArguments = [workingDirectory, version, isDevEnv];

    if ( cachePath.length > 0 ) 
      spawnArguments.push( cachePath );

    spawnArguments.push( socket_name );

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
