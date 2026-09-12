Unicode true
!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
Name "King Panel"
OutFile "KingPanel-Setup-0.9.4.exe"
InstallDir "$PROGRAMFILES64\King Panel"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
SetDatablockOptimize on
BrandingText "King Panel - King Alex Gilbert"
VIProductVersion "0.9.4.0"
VIAddVersionKey /LANG=1033 "ProductName" "King Panel Setup"
VIAddVersionKey /LANG=1033 "CompanyName" "King Alex Gilbert"
VIAddVersionKey /LANG=1033 "FileDescription" "King Panel Installer"
VIAddVersionKey /LANG=1033 "FileVersion" "0.9.4"
VIAddVersionKey /LANG=1033 "LegalCopyright" "King Alex Gilbert"
!define KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\KingPanel"
!define RUNKEY "Software\Microsoft\Windows\CurrentVersion\Run"
!define MUI_ICON "crown.ico"
!define MUI_UNICON "crown.ico"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TEXT "Install King Panel for all users of this computer.$\r$\n$\r$\nChoose whether to run at sign-in and create desktop and Start menu shortcuts.$\r$\n$\r$\nNo build tools or scripts are needed.$\r$\n$\r$\nIf you used an older AppData installation, uninstall it first from Windows Installed Apps."
!insertmacro MUI_PAGE_WELCOME
!define MUI_COMPONENTSPAGE_TEXT_TOP "Select your options. On an upgrade, these choices replace your previous startup and shortcut settings."
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "King Panel is installed. Close Setup, then launch it using your chosen shortcut or Program Files\King Panel\KingPanel.exe.$\r$\n$\r$\nThe app itself runs without administrator privileges."
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
Var SetupMutex

!macro Init PREFIX INITNAME
Function ${INITNAME}
 SetShellVarContext all
 SetRegView 64
 ${IfNot} ${RunningX64}
  MessageBox MB_OK|MB_ICONSTOP "King Panel requires 64-bit Windows."
  Abort
 ${EndIf}
 System::Call 'kernel32::CreateMutexW(p 0, i 0, w "Local\KingPanelSetup") p .r0 ?e'
 Pop $1
 StrCpy $SetupMutex $0
 ${If} $0 == 0
 ${OrIf} $1 == 183
  MessageBox MB_OK|MB_ICONSTOP "Another King Panel installer or uninstaller is running. Close it and try again."
  Abort
 ${EndIf}
FunctionEnd
Function ${PREFIX}CheckClosed
 retry:
 System::Call 'kernel32::OpenMutexW(i 0x100000, i 0, w "Local\DisplayTray-94BE521E") p .r0'
 ${If} $0 != 0
  System::Call 'kernel32::CloseHandle(p r0)'
  MessageBox MB_RETRYCANCEL|MB_ICONINFORMATION "King Panel is running. Right-click its crown icon and choose Exit, then click Retry.$\r$\n$\r$\nFinish any display confirmation first." IDRETRY retry
  Abort
 ${EndIf}
FunctionEnd
!macroend
!insertmacro Init "" ".onInit"
!insertmacro Init "un." "un.onInit"

Section "King Panel application (required)" SEC_APP
 SectionIn RO
 Call CheckClosed
 ; Fixed x64 machine-wide install target.
 StrCpy $INSTDIR "$PROGRAMFILES64\King Panel"
 SetOutPath "$INSTDIR"
 SetOverwrite on
 File "KingPanel.exe"
 WriteUninstaller "$INSTDIR\Uninstall.exe"
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONSTOP "Could not write the uninstaller. Please rerun Setup."
  Abort
 WriteRegStr HKLM "${KEY}" "DisplayName" "King Panel"
 WriteRegStr HKLM "${KEY}" "DisplayVersion" "0.9.4"
 WriteRegStr HKLM "${KEY}" "Publisher" "King Alex Gilbert"
 WriteRegStr HKLM "${KEY}" "DisplayIcon" "$INSTDIR\KingPanel.exe,0"
 WriteRegStr HKLM "${KEY}" "InstallLocation" "$INSTDIR"
 WriteRegStr HKLM "${KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
 WriteRegDWORD HKLM "${KEY}" "NoModify" 1
 WriteRegDWORD HKLM "${KEY}" "NoRepair" 1
 WriteRegDWORD HKLM "${KEY}" "EstimatedSize" 400
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONSTOP "Windows could not register King Panel. Rerun Setup to repair the installation."
  Abort
 ; Apply selected machine-wide startup and shortcut options.
 DeleteRegValue HKLM "${RUNKEY}" "King Panel"
 Delete "$DESKTOP\King Panel.lnk"
 Delete "$SMPROGRAMS\King Panel.lnk"
SectionEnd

Section /o "Run at Windows sign-in (all users)" SEC_STARTUP
 ClearErrors
 WriteRegStr HKLM "${RUNKEY}" "King Panel" '$\"$INSTDIR\KingPanel.exe$\"'
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "King Panel was installed, but Windows could not enable startup."
  SetErrorLevel 1
SectionEnd
Section /o "Create a desktop shortcut (all users)" SEC_DESKTOP
 ClearErrors
 CreateShortcut "$DESKTOP\King Panel.lnk" "$INSTDIR\KingPanel.exe" "" "$INSTDIR\KingPanel.exe" 0
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "King Panel was installed, but the desktop shortcut could not be created."
  SetErrorLevel 1
SectionEnd
Section "Create a Start menu shortcut (all users)" SEC_STARTMENU
 ClearErrors
 CreateShortcut "$SMPROGRAMS\King Panel.lnk" "$INSTDIR\KingPanel.exe" "" "$INSTDIR\KingPanel.exe" 0
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONEXCLAMATION "King Panel was installed, but the Start menu shortcut could not be created."
  SetErrorLevel 1
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
 !insertmacro MUI_DESCRIPTION_TEXT ${SEC_APP} "Install King Panel and its EXE uninstaller for all users of this computer."
 !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTUP} "Start in the notification area after each user signs in. Optional; off by default."
 !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP} "Add a crown shortcut to your desktop."
 !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU} "Add King Panel to your Start menu. You can uncheck this option."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"
 Call un.CheckClosed
 ReadRegStr $0 HKLM "${KEY}" "InstallLocation"
 ${If} $0 != $INSTDIR
  MessageBox MB_OK|MB_ICONSTOP "This uninstaller does not match the registered King Panel installation. Run it from the installed folder."
  Abort
 ${EndIf}
 ClearErrors
 Delete "$INSTDIR\KingPanel.exe"
 IfErrors 0 +3
  MessageBox MB_OK|MB_ICONSTOP "KingPanel.exe could not be removed. Close the app and try again."
  Abort
 DeleteRegValue HKLM "${RUNKEY}" "King Panel"
 Delete "$DESKTOP\King Panel.lnk"
 Delete "$SMPROGRAMS\King Panel.lnk"
 DeleteRegKey HKLM "${KEY}"
 Delete "$INSTDIR\Uninstall.exe"
 ; Never recursively remove a folder: preserve unrelated user files/build tools.
 RMDir "$INSTDIR"
SectionEnd
