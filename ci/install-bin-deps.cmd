mkdir build 
cd build
mkdir deps
cd deps

REM downlaoad obs dependencies to use zlib 
set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%BIN_DEPENDENCIES%.zip
if exist %BIN_DEPENDENCIES%\ ( 
    echo "Using cached dependencies"
) else (
    if exist %BIN_DEPENDENCIES%.zip (curl -kLO %DepsURL% -f --retry 5 -z %BIN_DEPENDENCIES%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)
    7z x %BIN_DEPENDENCIES%.zip -aoa -o%BIN_DEPENDENCIES%
)

REM boost lib download
set BOOST_DIST_NAME=boost-vc143-1_79_0-bin
set DEPS_DIST_URI=https://s3-us-west-2.amazonaws.com/streamlabs-obs-updater-deps

if exist boost\ ( 
    echo "Using cached boost"
) else (
    if exist %BOOST_DIST_NAME%.7z (curl -kLO "%DEPS_DIST_URI%/%BOOST_DIST_NAME%.7z" -f --retry 5 -z %BOOST_DIST_NAME%.7z) else (curl -kLO "%DEPS_DIST_URI%/%BOOST_DIST_NAME%.7z" -f --retry 5 -C -)
    7z x "%BOOST_DIST_NAME%.7z" -aoa -oboost
)

cd .. && cd ..