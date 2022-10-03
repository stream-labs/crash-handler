set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%BIN_DEPENDENCIES%.zip

if exist %BIN_DEPENDENCIES%.zip (curl -kLO %DepsURL% -f --retry 5 -z %BIN_DEPENDENCIES%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)

7z x %BIN_DEPENDENCIES%.zip -aoa -o%BIN_DEPENDENCIES%