set DEPS=dependencies2019.0
set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%DEPS%.zip

if exist %DEPS%.zip (curl -kLO %DepsURL% -f --retry 5 -z %DEPS%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)

7z x %DEPS%.zip -aoa -o%DEPS%

echo "%CD%\build\%DEPS%\win64"

setx DepsPath "%CD%\build\%DEPS%\win64" /M

echo %DepsPath%
