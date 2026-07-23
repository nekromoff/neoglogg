# NSIS script creating the Windows installer for neoglogg

# Is passed to the script using -DVERSION=$(git describe) on the command line
!ifndef VERSION
    !define VERSION 'anonymous-build'
!endif

# Headers
!include "MUI2.nsh"
!include "FileAssociation.nsh"

# General
OutFile "neoglogg-${VERSION}-setup.exe"

XpStyle on

SetCompressor /SOLID lzma

; Registry key to keep track of the directory we are installed in
!ifdef ARCH32
  InstallDir "$PROGRAMFILES\neoglogg"
!else
  InstallDir "$PROGRAMFILES64\neoglogg"
!endif
InstallDirRegKey HKLM Software\neoglogg ""

; neoglogg icon
; !define MUI_ICON neoglogg.ico

RequestExecutionLevel admin

Name "neoglogg"
Caption "neoglogg ${VERSION} Setup"

# Pages
!define MUI_WELCOMEPAGE_TITLE "Welcome to the neoglogg ${VERSION} Setup Wizard"
!define MUI_WELCOMEPAGE_TEXT "This wizard will guide you through the installation of neoglogg\
, a fast, advanced log explorer.$\r$\n$\r$\n\
neoglogg and the Qt libraries are released under the GPL, see \
the COPYING file.$\r$\n$\r$\n$_CLICK"
; MUI_FINISHPAGE_LINK_LOCATION "http://nsis.sf.net/"

!insertmacro MUI_PAGE_WELCOME
;!insertmacro MUI_PAGE_LICENSE "COPYING"
# !ifdef VER_MAJOR & VER_MINOR & VER_REVISION & VER_BUILD...
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

# Languages
!insertmacro MUI_LANGUAGE "English"

# Installer sections
Section "neoglogg" neoglogg
    ; Prevent this section from being unselected
    SectionIn RO

    SetOutPath $INSTDIR
    File release\neoglogg.exe
    File COPYING
    File README.md

    ; Create the 'sendto' link
    CreateShortCut "$SENDTO\neoglogg.lnk" "$INSTDIR\neoglogg,exe" "" "$INSTDIR\neoglogg.exe" 0

    ; Register as an otion (but not main handler) for some files (.txt, .Log, .cap)
    WriteRegStr HKCR "Applications\neoglogg.exe" "" ""
    WriteRegStr HKCR "Applications\neoglogg.exe\shell" "" "open"
    WriteRegStr HKCR "Applications\neoglogg.exe\shell\open" "FriendlyAppName" "neoglogg"
    WriteRegStr HKCR "Applications\neoglogg.exe\shell\open\command" "" '"$INSTDIR\neoglogg.exe" "%1"'
    WriteRegStr HKCR "*\OpenWithList\neoglogg.exe" "" ""
    WriteRegStr HKCR ".txt\OpenWithList\neoglogg.exe" "" ""
    WriteRegStr HKCR ".Log\OpenWithList\neoglogg.exe" "" ""
    WriteRegStr HKCR ".cap\OpenWithList\neoglogg.exe" "" ""

    ; Register uninstaller
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg"\
"UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegExpandStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg"\
"InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg" "DisplayName" "neoglogg"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg" "NoModify" "1"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg" "NoRepair" "1"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Qt5 Runtime libraries" qtlibs
    SetOutPath $INSTDIR
    File release\Qt5Core.dll
    File release\Qt5Gui.dll
    File release\Qt5Network.dll
    File release\Qt5Widgets.dll
    File release\libwinpthread-1.dll
    SetOutPath $INSTDIR\platforms
    File release\qwindows.dll
SectionEnd

Section "Create Start menu shortcut" shortcut
    SetShellVarContext all
    CreateShortCut "$SMPROGRAMS\neoglogg.lnk" "$INSTDIR\neoglogg.exe" "" "$INSTDIR\neoglogg.exe" 0
SectionEnd

Section /o "Associate with .log files" associate
    ${registerExtension} "$INSTDIR\neoglogg.exe" ".log" "Log file"
SectionEnd

# Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${neoglogg} "The core files required to use neoglogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${qtlibs} "Needed by neoglogg, you have to install these unless \
you already have the Qt5 development kit installed."
    !insertmacro MUI_DESCRIPTION_TEXT ${shortcut} "Create a shortcut in the Start menu for neoglogg."
    !insertmacro MUI_DESCRIPTION_TEXT ${associate} "Make neoglogg the default viewer for .log files."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

# Uninstaller
Section "Uninstall"
    Delete "$INSTDIR\Uninstall.exe"

    Delete "$INSTDIR\neoglogg.exe"
    Delete "$INSTDIR\README"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\mingwm10.dll"
    Delete "$INSTDIR\libgcc_s_dw2-1.dll"
    Delete "$INSTDIR\QtCore4.dll"
    Delete "$INSTDIR\QtGui4.dll"
    Delete "$INSTDIR\QtNetwork4.dll"
    Delete "$INSTDIR\Qt5Core.dll"
    Delete "$INSTDIR\Qt5Gui.dll"
    Delete "$INSTDIR\Qt5Network.dll"
    Delete "$INSTDIR\Qt5Widgets.dll"
    Delete "$INSTDIR\libwinpthread-1.dll"
    Delete "$INSTDIR\platforms\qwindows.dll"
    RMDir "$INSTDIR"

    ; Remove settings in %appdata%
    Delete "$APPDATA\neoglogg\neoglogg.ini"
    RMDir "$APPDATA\neoglogg"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\neoglogg"

    ; Remove settings in the registry (from neoglogg < 0.9)
    DeleteRegKey HKCU "Software\neoglogg"

    ; Remove the file associations
    ${unregisterExtension} ".log" "Log file"

    DeleteRegKey HKCR "*\OpenWithList\neoglogg.exe"
    DeleteRegKey HKCR ".txt\OpenWithList\neoglogg.exe"
    DeleteRegKey HKCR ".Log\OpenWithList\neoglogg.exe"
    DeleteRegKey HKCR ".cap\OpenWithList\neoglogg.exe"
    DeleteRegKey HKCR "Applications\neoglogg.exe\shell\open\command"
    DeleteRegKey HKCR "Applications\neoglogg.exe\shell\open"
    DeleteRegKey HKCR "Applications\neoglogg.exe\shell"
    DeleteRegKey HKCR "Applications\neoglogg.exe"

    ; Remove the shortcut, if any
    SetShellVarContext all
    Delete "$SMPROGRAMS\neoglogg.lnk"
SectionEnd
