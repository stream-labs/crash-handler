REM get tools 
call ci\localization_get_tools.cmd

REM dump strings need to be translated from source code 
%GETTEXT_BIN%xgettext.exe --keyword=translate:1,1t crash-handler-process\platforms\util-osx.mm  crash-handler-process\platforms\util-win.cpp crash-handler-process\platforms\upload-window-win.cpp -d messages -p crash-handler-process\locale
del crash-handler-process\\locale\\messages.pot
rename crash-handler-process\\locale\\messages.po messages.pot

REM make sure that the locale directory exists for each locale to be able to save files to it
for %%F in (%locales_list%) do if not exist "crash-handler-process\locale\%%~F\" mkdir "crash-handler-process\locale\%%~F\\LC_MESSAGES"

REM only generate new .po files if there is none. 
REM to prevernt loosing existing translations
for %%F in (%locales_list%) do if not exist "crash-handler-process\locale\%%~F\LC_MESSAGES\messages.po" %GETTEXT_BIN%msginit.exe -i crash-handler-process/locale/messages.pot --locale=%%~F -o crash-handler-process/locale/%%~F/LC_MESSAGES/messages.po

REM merge new lines to existing .po files
for %%F in (%locales_list%) do %GETTEXT_BIN%msgmerge.exe crash-handler-process/locale/%%~F/LC_MESSAGES/messages.po crash-handler-process/locale/messages.pot -o crash-handler-process/locale/%%~F/LC_MESSAGES/messages.po

REM generate diff file for each locale
REM %GETTEXT_BIN%msguniq.exe crash-handler-process/locale/messages.pot -d

del crash-handler-process\\locale\\messages.pot
