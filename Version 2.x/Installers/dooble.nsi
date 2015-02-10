# Define installer name.

Name "Dooble Web Browser Installer"
!define APPNAME "Dooble Web Browser"
outFile "Dooble-Installer.exe"

# Install directory.

InstallDir "C:\Dooble"

DirText "Please choose an installation directory for the Dooble Web Browser."

# Default section start.

section

# Define output path.

setOutPath $INSTDIR

# This a beautiful spot to specify the important files.

file /r ..\release\Icons
file /r ..\release\Images
file /r ..\release\Plugins
file /r ..\release\Translations
file ..\Resources\qt.conf
file ..\release\*.dll
file ..\release\Dooble.exe

# This is a comfortable spot to specify the plugin files.

file ..\release\gpg.exe
file ..\release\bdboot.txt
file ..\release\gpgme-w32spawn.exe

# Define menu link.

CreateDirectory "$SMPROGRAMS\${APPNAME}"
CreateShortCut  "$SMPROGRAMS\${APPNAME}\Start ${APPNAME}.lnk" "$INSTDIR\Dooble.exe" "" "$INSTDIR\Dooble.exe" 0
CreateShortCut  "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" "$INSTDIR\Dooble-Uninstaller.exe" "" "$INSTDIR\Dooble-Uninstaller.exe" 0
CreateShortCut  "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\dooble.exe" "" "$INSTDIR\dooble.exe" 0
CreateShortCut  "$QUICKLAUNCH\${APPNAME}.lnk" "$INSTDIR\dooble.exe" "" "$INSTDIR\dooble.exe" 0
CreateShortCut  "$QUICKLAUNCH\User Pinned\TaskBar\${APPNAME}.lnk" "$INSTDIR\dooble.exe" "" "$INSTDIR\dooble.exe" 0
CreateShortCut  "$QUICKLAUNCH\User Pinned\StartMenu\${APPNAME}.lnk" "$INSTDIR\dooble.exe" "" "$INSTDIR\dooble.exe" 0

# Define uninstaller name.

writeUninstaller $INSTDIR\Dooble-Uninstaller.exe

# Default section end.

sectionEnd

# Create a section to define what the uninstaller does.
# The section will always be named "Uninstall".

section "Uninstall"

Delete "$SMPROGRAMS\${APPNAME}\Start ${APPNAME}.lnk"
RMDir  "$SMPROGRAMS\${APPNAME}\Start ${APPNAME}.lnk"
Delete "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk"
RMDir  "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk"
Delete  "$SMPROGRAMS\${APPNAME}"
RMDir   "$SMPROGRAMS\${APPNAME}"
Delete  "$DESKTOP\${APPNAME}.lnk"
Delete  "$QUICKLAUNCH\${APPNAME}.lnk"
Delete  "$QUICKLAUNCH\User Pinned\TaskBar\${APPNAME}.lnk"
Delete  "$QUICKLAUNCH\User Pinned\StartMenu\${APPNAME}.lnk"
RMDir /r $INSTDIR

sectionEnd
