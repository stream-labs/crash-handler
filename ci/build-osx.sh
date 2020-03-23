mkdir build
cd build

mkdir crash-handler

cmake .. -DCMAKE_INSTALL_PREFIX=${PWD}/build/distribute/crash-handler -DNODEJS_NAME=${RuntimeName} -DNODEJS_URL=${RuntimeURL} -DNODEJS_VERSION=${RuntimeVersion} -DCMAKE_BUILD_TYPE=RelWithDebInfo

cd ..

cmake --build build --target install --config RelWithDebInfo