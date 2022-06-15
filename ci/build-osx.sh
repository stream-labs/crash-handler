mkdir build
cd build

mkdir crash-handler

cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_INSTALL_PREFIX=${PWD}/distribute/crash-handler -DNODEJS_VERSION=${{env.ELECTRON_VERSION}} -DCMAKE_BUILD_TYPE=RelWithDebInfo

cd ..

cmake --build build --target install --config RelWithDebInfo