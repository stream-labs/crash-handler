mkdir ${SLBuildDirectory}
cd ${SLBuildDirectory}

cmake .. -DCMAKE_INSTALL_PREFIX=${FullDistributePath}/crash-handler -DNODEJS_NAME=${RuntimeName} -DNODEJS_URL=${RuntimeURL} -DNODEJS_VERSION=${RuntimeVersion} -DCMAKE_BUILD_TYPE=RelWithDebInfo

cd ..

cmake --build ${SLBuildDirectory} --target install --config RelWithDebInfo