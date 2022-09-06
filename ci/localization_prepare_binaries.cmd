REM get tools 
call ci\localization_get_tools.cmd

REM merge new lines to existing .po files
for %%F in (%locales_list%) do %GETTEXT_BIN%msgfmt.exe crash-handler-process/locale/%%~F/LC_MESSAGES/messages.po -o crash-handler-process/locale/%%~F_messages.mo
