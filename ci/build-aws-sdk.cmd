:: run before cmake of main project to build aws cpp sdk locally 
:: it expect AWS_SDK_VERSION env variable to be set

mkdir aws-sdk
cd aws-sdk

git clone --branch %AWS_SDK_VERSION% --depth 1 --recurse-submodules https://github.com/aws/aws-sdk-cpp.git

mkdir build
cd build

cmake -G "Visual Studio 17 2022" -A x64 ^
-DBUILD_ONLY="s3;sts" ^
-DENABLE_TESTING=OFF ^
-DBUILD_SHARED_LIBS=OFF ^
-DSTATIC_CRT=ON ^
-DFORCE_SHARED_CRT=OFF ^
-DENABLE_UNITY_BUILD=ON ^
-DSTATIC_LINKING=1 ^
-DCUSTOM_MEMORY_MANAGEMENT=OFF ^
-DCPP_STANDARD=17 ^
-DCMAKE_BUILD_TYPE=Release ^
-DCMAKE_INSTALL_PREFIX=%cd%../../awsi/ ^
../aws-sdk-cpp

cmake --build . --target install --config Release

cd .. 
cd .. 