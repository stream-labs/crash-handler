# crash-handler
Crash handler 

# To build 
## On Windows 
```
yarn install

set BIN_DEPENDENCIES=dependencies2019.0
ci\install-bin-deps.cmd

set AWS_SDK_VERSION="1.9.248"
ci\build-aws-sdk.cmd

ci\localization_prepare_binaries.cmd

set INSTALL_PACKAGE_PATH:"../desktop/node_modules/crash_handler"
cmake  -B"build" -G"Visual Studio 17 2022" -A x64  -DDepsPath="%CD%\build\deps\%BIN_DEPENDENCIES%\win64" -DBOOST_ROOT="%CD%\build\deps\boost"  -DCMAKE_INSTALL_PREFIX="%INSTALL_PACKAGE_PATH%"
cmake --build "build" --target install --config RelWithDebInfo
```

## On macOS
```
yarn install

./ci/build-osx.sh
```

## Localization
Boost.locale lib with a gettext format used for a localization(on windows). 
mo files included in exe by windows resources. 
### Commands 

`ci\localization_prepare_binaries.cmd` - prepares mo files with current translation 

`ci\generate_new_translations.cmd` - update po files with current strings from source code 

### Add new language 

* Add new lang code into `ci\localization_get_tools.cmd` and run `ci\generate_new_translations.cmd`
* Translate lines inside `locale\NEW_LANG\LC_MESSAGES\messages.po`
* Add new mo file to `resources\slobs-updater.rc`
* Add it to `locales_resources` map inside `get_messages_callback()`
* Prepare binaries `ci\localization_prepare_binaries.cmd`
* Make a new build 
* Do not forget to commit `locale\NEW_LANG\LC_MESSAGES\messages.po`