mkdir aws-sdk
cd aws-sdk

git clone --branch %AWS_SDK_VERSION% --depth 1 --recurse-submodules https://github.com/aws/aws-sdk-cpp.git

mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64  -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;sts" -DENABLE_TESTING=OFF -DCUSTOM_MEMORY_MANAGEMENT=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%cd%../../awsi/ ../aws-sdk-cpp

cmake --build . --target install --config Release

