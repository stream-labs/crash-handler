# crash-handler
Crash handler 

# To build 
## On Windows 
```
yarn install 

set BIN_DEPENDENCIES="dependencies2019.0"
ci\install-zlib.cmd

set AWS_SDK_VERSION="1.8.186"
ci\build-aws-sdk.cmd

set INSTALL_PACKAGE_PATH:"../desktop/node_modules/crash_handler"
cmake  -B"build" -G"Visual Studio 17 2022" -A x64 -DDepsPath="${PWD}\build\deps\${BIN_DEPENDENCIES}\win64" -DBOOST_ROOT="${PWD}\build\deps\boost" -DCMAKE_INSTALL_PREFIX="${INSTALL_PACKAGE_PATH}"
cmake --build "build" --target install --config RelWithDebInfo
```

## On macOS
```
yarn install 
set 
./ci/build-osx.sh
```