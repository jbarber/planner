; Installer script for win32 Planner
; Herman Bloggs <hermanator12002@yahoo.com>
; Maurice van der Pot <griffon26@kfk4ever.com>

; NOTE: this .NSI script is intended for NSIS 2.0 (finale release).
;

;--------------------------------
;Global Variables
Var name
Var LANG_IS_SET
Var STARTUP_RUN_KEY

var oldgtk_mode	; "public", "private" or "none" (used by uninstall of old version)
var gtk_mode	; "public", "private" or "none"
var gtk_tmp	; temporary variable
var gtk_translations	; whether or not to install gtk translations (copied from user selection of Planner translations)

;--------------------------------
;Configuration

;The name var is set in .onInit
Name $name

OutFile "planner-${PLANNER_VERSION}.exe"

SetCompressor lzma
ShowInstDetails show
ShowUninstDetails show
SetDateSave on

; $name and $INSTDIR are set in .onInit function..

!include "MUI.nsh"
!include "Sections.nsh"
!include "WordFunc.nsh"
!insertmacro VersionCompare

;--------------------------------
;Defines

!define PLANNER_REG_KEY				"SOFTWARE\planner"
!define PLANNER_UNINSTALL_KEY			"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Planner"
!define HKLM_APP_PATHS_KEY			"SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\planner.exe"
!define PLANNER_STARTUP_RUN_KEY			"SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
!define PLANNER_UNINST_EXE			"planner-uninst.exe"
!define PLANNER_REG_LANG			"Installer Language"
!define GTK_INSTALLER_EXE			"gtk2-runtime-2.22.0-2010-10-01-ash.exe"

;--------------------------------
;Modern UI Configuration

  !define MUI_ICON				".\win-install.ico"
  !define MUI_UNICON				".\win-uninstall.ico"
;  !define MUI_WELCOMEFINISHPAGE_BITMAP 		".\src\win32\nsis\planner-intro.bmp"
;  !define MUI_HEADERIMAGE
;  !define MUI_HEADERIMAGE_BITMAP		".\src\win32\nsis\planner-header.bmp"

  ; Alter License section
  !define MUI_LICENSEPAGE_BUTTON		$(PLANNER_LICENSE_BUTTON)
  !define MUI_LICENSEPAGE_TEXT_BOTTOM		$(PLANNER_LICENSE_BOTTOM_TEXT)

  !define MUI_COMPONENTSPAGE_SMALLDESC
  !define MUI_ABORTWARNING

  ;Finish Page config
  !define MUI_FINISHPAGE_RUN			"$INSTDIR\bin\planner.exe"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_LINK			$(PLANNER_FINISH_VISIT_WEB_SITE)
  !define MUI_FINISHPAGE_LINK_LOCATION          "http://live.gnome.org/Planner/"

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE			"${STAGING_DIR}\COPYING"
  !define MUI_PAGE_CUSTOMFUNCTION_LEAVE on_components_page_leave
  !insertmacro MUI_PAGE_COMPONENTS

  ; Planner install dir page
  !insertmacro MUI_PAGE_DIRECTORY

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages & Translations

  ;; English goes first because its the default. The rest are
  ;; in alphabetical order (at least the strings actually displayed
  ;; will be).

  !define PLANNER_DEFAULT_LANGFILE "languages\english.nsh"

  !include ".\langmacros.nsh"

  !insertmacro PLANNER_MACRO_INCLUDE_LANGFILE "ENGLISH"		"languages\english.nsh"
  !insertmacro PLANNER_MACRO_INCLUDE_LANGFILE "DUTCH"		"languages\dutch.nsh"

;--------------------------------
;Reserve Files
  ; Only need this if using bzip2 compression

  !insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
  !insertmacro MUI_RESERVEFILE_LANGDLL
  ReserveFile "${NSISDIR}\Plugins\UserInfo.dll"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Start Install Sections ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------------
;Uninstall any old version of Planner

Section -SecUninstallOldPlanner
  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  ;If planner is currently set to run on startup,
  ;  save the section of the Registry where the setting is before uninstalling,
  ;  so we can put it back after installing the new version
  ClearErrors
  ReadRegStr $STARTUP_RUN_KEY HKCU "${PLANNER_STARTUP_RUN_KEY}" "Planner"
  IfErrors +3
  StrCpy $STARTUP_RUN_KEY "HKCU"
  Goto +4
  ReadRegStr $STARTUP_RUN_KEY HKLM "${PLANNER_STARTUP_RUN_KEY}" "Planner"
  IfErrors +2
  StrCpy $STARTUP_RUN_KEY "HKLM"

  StrCmp $R0 "HKLM" planner_hklm
  StrCmp $R0 "HKCU" planner_hkcu done

  planner_hkcu:
      ReadRegStr $oldgtk_mode HKCU ${PLANNER_REG_KEY} "GtkInstalledMode"
      ReadRegStr $R1 HKCU ${PLANNER_REG_KEY} ""
      ReadRegStr $R2 HKCU ${PLANNER_REG_KEY} "Version"
      ReadRegStr $R3 HKCU "${PLANNER_UNINSTALL_KEY}" "UninstallString"
      Goto try_uninstall

  planner_hklm:
      ReadRegStr $oldgtk_mode HKLM ${PLANNER_REG_KEY} "GtkInstalledMode"
      ReadRegStr $R1 HKLM ${PLANNER_REG_KEY} ""
      ReadRegStr $R2 HKLM ${PLANNER_REG_KEY} "Version"
      ReadRegStr $R3 HKLM "${PLANNER_UNINSTALL_KEY}" "UninstallString"

  ; If previous version exists .. remove
  try_uninstall:
    StrCmp $oldgtk_mode "private" "" skip_gtk_remove
      ; remove private GTK+, specify the same custom options are during installation
      ExecWait "$INSTDIR\gtk2_runtime_uninst.exe /remove_config=yes /sideeffects=no /translations=yes /compatdlls=no /S"
      ; _?=$INSTDIR
      ; Delete "$INSTDIR\gtk2_runtime_uninst.exe"  ; If using _? flag, it won't get deleted automatically, do it manually.

  skip_gtk_remove:
    StrCmp $R1 "" done
      ; Version key started with 0.60a3. Prior versions can't be
      ; automaticlly uninstalled.
      StrCmp $R2 "" uninstall_problem
        ; Check if we have uninstall string..
        IfFileExists $R3 0 uninstall_problem
          ; Have uninstall string.. go ahead and uninstall.
          SetOverwrite on
          ; Need to copy uninstaller outside of the install dir
          ClearErrors
          CopyFiles /SILENT $R3 "$TEMP\${PLANNER_UNINST_EXE}"
          SetOverwrite off
          IfErrors uninstall_problem
            ; Ready to uninstall..
            ClearErrors
	    ExecWait '"$TEMP\${PLANNER_UNINST_EXE}" /S _?=$R1'
	    IfErrors exec_error
              Delete "$TEMP\${PLANNER_UNINST_EXE}"
	      Goto done

	    exec_error:
              Delete "$TEMP\${PLANNER_UNINST_EXE}"
              Goto uninstall_problem

        uninstall_problem:
	  ; In this case just wipe out previous Planner install dir..
	  ; We get here because versions 0.60a1 and 0.60a2 don't have versions set in the registry
	  ; and versions 0.60 and lower did not correctly set the uninstall reg string
	  ; (the string was set in quotes)
          IfSilent do_wipeout
          MessageBox MB_YESNO $(PLANNER_PROMPT_WIPEOUT) IDYES do_wipeout IDNO cancel_install
          cancel_install:
            Quit

          do_wipeout:
            StrCmp $R0 "HKLM" planner_del_lm_reg planner_del_cu_reg
            planner_del_cu_reg:
              DeleteRegKey HKCU ${PLANNER_REG_KEY}
              Goto uninstall_prob_cont
            planner_del_lm_reg:
              DeleteRegKey HKLM ${PLANNER_REG_KEY}

            uninstall_prob_cont:
	      RMDir /r "$R1"

  done:
SectionEnd

;--------------------------------
;Planner Install Section

var gtk_dll_abs_path

Section $(PLANNER_SECTION_TITLE) SecPlanner
  SectionIn 1 RO

  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  StrCmp $R0 "NONE" planner_install_files
  StrCmp $R0 "HKLM" planner_hklm planner_hkcu

  planner_hklm:
    StrCmp $gtk_mode "private" skip_exe_PATH
    ; set a special path for this exe, as GTK may not be in a global path.
    ReadRegStr $gtk_dll_abs_path HKLM "SOFTWARE\GTK\2.0" "DllPath"
    WriteRegStr HKLM "${HKLM_APP_PATHS_KEY}" "Path" "$gtk_dll_abs_path"

  skip_exe_PATH:
    WriteRegStr HKLM "${HKLM_APP_PATHS_KEY}" "" "$INSTDIR\bin\planner.exe"
    WriteRegStr HKLM ${PLANNER_REG_KEY} "GtkInstalledMode" "$gtk_mode"
    WriteRegStr HKLM ${PLANNER_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKLM ${PLANNER_REG_KEY} "Version" "${PLANNER_VERSION}"
    WriteRegStr HKLM "${PLANNER_UNINSTALL_KEY}" "DisplayName" $(PLANNER_UNINSTALL_DESC)
    WriteRegStr HKLM "${PLANNER_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${PLANNER_UNINST_EXE}"
    ; Sets scope of the desktop and Start Menu entries for all users.
    SetShellVarContext "all"
    Goto planner_install_files

  planner_hkcu:
    WriteRegStr HKCU ${PLANNER_REG_KEY} "GtkInstalledMode" "$gtk_mode"
    WriteRegStr HKCU ${PLANNER_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKCU ${PLANNER_REG_KEY} "Version" "${PLANNER_VERSION}"
    WriteRegStr HKCU "${PLANNER_UNINSTALL_KEY}" "DisplayName" $(PLANNER_UNINSTALL_DESC)
    WriteRegStr HKCU "${PLANNER_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${PLANNER_UNINST_EXE}"
    Goto planner_install_files

  planner_install_files:
    ; Planner files
    SetOverwrite on
    SetOutPath "$INSTDIR"
    File /r /x locale ${STAGING_DIR}\*.*

    CreateDirectory "$SMPROGRAMS\Planner"
    CreateShortCut "$SMPROGRAMS\Planner\Planner.lnk" "$INSTDIR\bin\planner.exe"
    CreateShortCut "$DESKTOP\Planner.lnk" "$INSTDIR\bin\planner.exe"
    SetOutPath "$INSTDIR"

    ; If we don't have install rights.. we're done
    StrCmp $R0 "NONE" done
    CreateShortCut "$SMPROGRAMS\Planner\User Guide.lnk" "$INSTDIR\planner.chm"
    CreateShortCut "$SMPROGRAMS\Planner\Uninstall.lnk" "$INSTDIR\${PLANNER_UNINST_EXE}"
    SetOverwrite off

    ; Write out installer language
    WriteRegStr HKCU "${PLANNER_REG_KEY}" "${PLANNER_REG_LANG}" "$LANGUAGE"

    ; write out uninstaller
    SetOverwrite on
    WriteUninstaller "$INSTDIR\${PLANNER_UNINST_EXE}"
    SetOverwrite off

    ; If we previously had planner setup to run on startup, make it do so again
    StrCmp $STARTUP_RUN_KEY "HKCU" +1 +2
    WriteRegStr HKCU "${PLANNER_STARTUP_RUN_KEY}" "Planner" "$INSTDIR\bin\planner.exe"
    StrCmp $STARTUP_RUN_KEY "HKLM" +1 +2
    WriteRegStr HKLM "${PLANNER_STARTUP_RUN_KEY}" "Planner" "$INSTDIR\bin\planner.exe"

  done:
SectionEnd ; end of default Planner section

;--------------------------------
; GTK+ installation Sections

; The SecGtkPrivate and SecGtkPublic sections are mutually exclusive.

Section $(GTK_PRIVATE_SECTION_TITLE) SecGtkPrivate
	SectionIn 1
	SetShellVarContext all  ; use all user variables as opposed to current user
	AddSize 12200  ; ~ size of unpacked gtk
	SetOutPath "$INSTDIR"
	File "${GTK_INSTALLER_EXE}"
	; TODO: in the future, when we have translations for this program,
	; make the GTK+ translations installation dependent on their installation status.
	ExecWait '"${GTK_INSTALLER_EXE}" /sideeffects=no /translations=$gtk_translations /compatdlls=no /S /D=$INSTDIR'
	Delete "$INSTDIR\${GTK_INSTALLER_EXE}"
SectionEnd

; disabled by default
Section /o  $(GTK_PUBLIC_SECTION_TITLE) SecGtkPublic
	SectionIn 1
	SetShellVarContext all  ; use all user variables as opposed to current user
	AddSize 12200  ; ~ size of unpacked gtk
	SetOutPath "$INSTDIR"
	File "${GTK_INSTALLER_EXE}"
	ExecWait '"${GTK_INSTALLER_EXE}"'
	Delete "$INSTDIR\${GTK_INSTALLER_EXE}"
SectionEnd

;--------------------------------
; Translations section

Section /o $(PLANNER_TRANSLATIONS_SECTION_TITLE) SecTranslations
  SetOverwrite on
  File /r ${STAGING_DIR}\*.mo
  SetOverwrite off
SectionEnd

;--------------------------------
;Uninstaller Section


Section Uninstall
  Call un.CheckUserInstallRights
  Pop $R0
  StrCmp $R0 "NONE" no_rights
  StrCmp $R0 "HKCU" try_hkcu try_hklm

  try_hkcu:
    ReadRegStr $R0 HKCU ${PLANNER_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 cant_uninstall
      ; HKCU install path matches our INSTDIR.. so uninstall
      ReadRegStr $gtk_mode HKCU ${PLANNER_REG_KEY} "GtkInstalledMode"
      DeleteRegKey HKCU ${PLANNER_REG_KEY}
      DeleteRegKey HKCU "${PLANNER_UNINSTALL_KEY}"
      Goto cont_uninstall

  try_hklm:
    ReadRegStr $R0 HKLM ${PLANNER_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 try_hkcu
      ; HKLM install path matches our INSTDIR.. so uninstall
      ReadRegStr $gtk_mode HKLM ${PLANNER_REG_KEY} "GtkInstalledMode"
      DeleteRegKey HKLM ${PLANNER_REG_KEY}
      DeleteRegKey HKLM "${PLANNER_UNINSTALL_KEY}"
      DeleteRegKey HKLM "${HKLM_APP_PATHS_KEY}"
      ; Sets start menu and desktop scope to all users..
      SetShellVarContext "all"

  cont_uninstall:
    StrCmp $gtk_mode "private" "" skip_gtk_remove
      ; remove private GTK+, specify the same custom options are during installation
      ExecWait "$INSTDIR\gtk2_runtime_uninst.exe /remove_config=yes /sideeffects=no /translations=yes /compatdlls=no /S"
      ; _?=$INSTDIR
      ; Delete "$INSTDIR\gtk2_runtime_uninst.exe"  ; If using _? flag, it won't get deleted automatically, do it manually.

  skip_gtk_remove:
    ; The WinPrefs plugin may have left this behind..
    DeleteRegValue HKCU "${PLANNER_STARTUP_RUN_KEY}" "Planner"
    DeleteRegValue HKLM "${PLANNER_STARTUP_RUN_KEY}" "Planner"
    ; Remove Language preference info
    DeleteRegKey HKCU ${PLANNER_REG_KEY} ;${MUI_LANGDLL_REGISTRY_ROOT} ${MUI_LANGDLL_REGISTRY_KEY}

    RMDir /r "$INSTDIR\bin"
    RMDir /r "$INSTDIR\etc"
    RMDir /r "$INSTDIR\lib"
    RMDir /r "$INSTDIR\share"
    ;RMDir /r "$INSTDIR\perlmod"
    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\planner.chm"
    Delete "$INSTDIR\${PLANNER_UNINST_EXE}"

    ;Try to remove Planner install dir .. if empty
    RMDir "$INSTDIR"

    ; Shortcuts..
    RMDir /r "$SMPROGRAMS\Planner"
    Delete "$DESKTOP\Planner.lnk"

    Goto done

  cant_uninstall:
    IfSilent skip_mb
    MessageBox MB_OK $(un.PLANNER_UNINSTALL_ERROR_1) IDOK
    skip_mb:
    Quit

  no_rights:
    IfSilent skip_mb1
    MessageBox MB_OK $(un.PLANNER_UNINSTALL_ERROR_2) IDOK
    skip_mb1:
    Quit

  done:
SectionEnd ; end of uninstall section

;--------------------------------
;Descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPlanner} \
	$(PLANNER_SECTION_DESCRIPTION)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecTranslations} \
	$(PLANNER_TRANSLATIONS_SECTION_DESCRIPTION)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecGtkPrivate} \
	$(GTK_PRIVATE_SECTION_DESCRIPTION)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecGtkPublic} \
	$(GTK_PUBLIC_SECTION_DESCRIPTION)

!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Functions

Function CheckUserInstallRights
	ClearErrors
	UserInfo::GetName
	IfErrors Win9x
	Pop $0
	UserInfo::GetAccountType
	Pop $1

	StrCmp $1 "Admin" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "Power" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "User" 0 +3
		StrCpy $1 "HKCU"
		Goto done
	StrCmp $1 "Guest" 0 +3
		StrCpy $1 "NONE"
		Goto done
	; Unknown error
	StrCpy $1 "NONE"
        Goto done

	Win9x:
		StrCpy $1 "HKLM"

	done:
        Push $1
FunctionEnd

Function un.CheckUserInstallRights
	ClearErrors
	UserInfo::GetName
	IfErrors Win9x
	Pop $0
	UserInfo::GetAccountType
	Pop $1

	StrCmp $1 "Admin" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "Power" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "User" 0 +3
		StrCpy $1 "HKCU"
		Goto done
	StrCmp $1 "Guest" 0 +3
		StrCpy $1 "NONE"
		Goto done
	; Unknown error
	StrCpy $1 "NONE"
        Goto done

	Win9x:
		StrCpy $1 "HKLM"

	done:
        Push $1
FunctionEnd

;
; Usage:
;   Push $0 ; Path string
;   Call VerifyDir
;   Pop $0 ; 0 - Bad path  1 - Good path
;
Function VerifyDir
  Pop $0
  Loop:
    IfFileExists $0 dir_exists
    StrCpy $1 $0 ; save last
    Push $0
    Call GetParent
    Pop $0
    StrLen $2 $0
    ; IfFileExists "C:" on xp returns true and on win2k returns false
    ; So we're done in such a case..
    IntCmp $2 2 loop_done
    ; GetParent of "C:" returns ""
    IntCmp $2 0 loop_done
    Goto Loop

  loop_done:
    StrCpy $1 "$0\PlannerFooB"
    ; Check if we can create dir on this drive..
    ClearErrors
    CreateDirectory $1
    IfErrors DirBad DirGood

  dir_exists:
    ClearErrors
    FileOpen $1 "$0\plannerfoo.bar" w
    IfErrors PathBad PathGood

    DirGood:
      RMDir $1
      Goto PathGood1

    DirBad:
      RMDir $1
      Goto PathBad1

    PathBad:
      FileClose $1
      Delete "$0\plannerfoo.bar"
      PathBad1:
      StrCpy $0 "0"
      Push $0
      Return

    PathGood:
      FileClose $1
      Delete "$0\plannerfoo.bar"
      PathGood1:
      StrCpy $0 "1"
      Push $0
FunctionEnd

Function .onVerifyInstDir
  Push $INSTDIR
  Call VerifyDir
  Pop $0
  StrCmp $0 "0" 0 dir_good
    Abort
  dir_good:
FunctionEnd

; GetParent
; input, top of stack  (e.g. C:\Program Files\Poop)
; output, top of stack (replaces, with e.g. C:\Program Files)
; modifies no other variables.
;
; Usage:
;   Push "C:\Program Files\Directory\Whatever"
;   Call GetParent
;   Pop $R0
;   ; at this point $R0 will equal "C:\Program Files\Directory"
Function GetParent
   Exch $0 ; old $0 is on top of stack
   Push $1
   Push $2
   StrCpy $1 -1
   loop:
     StrCpy $2 $0 1 $1
     StrCmp $2 "" exit
     StrCmp $2 "\" exit
     IntOp $1 $1 - 1
   Goto loop
   exit:
     StrCpy $0 $0 $1
     Pop $2
     Pop $1
     Exch $0 ; put $0 on top of stack, restore $0 to original value
FunctionEnd

Function RunCheck
  System::Call 'kernel32::OpenMutex(i 2031617, b 0, t "planner_is_running") i .R0'
  IntCmp $R0 0 done
  MessageBox MB_OK|MB_ICONEXCLAMATION $(PLANNER_IS_RUNNING) IDOK
    Abort
  done:
FunctionEnd

Function un.RunCheck
  System::Call 'kernel32::OpenMutex(i 2031617, b 0, t "planner_is_running") i .R0'
  IntCmp $R0 0 done
  MessageBox MB_OK|MB_ICONEXCLAMATION $(PLANNER_IS_RUNNING) IDOK
    Abort
  done:
FunctionEnd

Function .onInit
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "planner_installer_running") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONEXCLAMATION $(INSTALLER_IS_RUNNING)
    Abort
  Call RunCheck
  StrCpy $name "Planner ${PLANNER_VERSION}"
  Call ParseParameters

  ; Select Language
  IntCmp $LANG_IS_SET 1 skip_lang
    ; Display Language selection dialog
    !insertmacro MUI_LANGDLL_DISPLAY
  skip_lang:

  ; If install path was set on the command, use it.
  StrCmp $INSTDIR "" 0 instdir_done

  ;  If planner is currently installed, we should default to where it is currently installed
  ClearErrors
  ReadRegStr $INSTDIR HKCU "${PLANNER_REG_KEY}" ""
  IfErrors +2
  StrCmp $INSTDIR "" 0 instdir_done
  ReadRegStr $INSTDIR HKLM "${PLANNER_REG_KEY}" ""
  IfErrors +2
  StrCmp $INSTDIR "" 0 instdir_done

  Call CheckUserInstallRights
  Pop $0

  StrCmp $0 "HKLM" 0 user_dir
    StrCpy $INSTDIR "$PROGRAMFILES\Planner"
    Goto instdir_done
  user_dir:
    StrCpy $2 "$SMPROGRAMS"
    Push $2
    Call GetParent
    Call GetParent
    Pop $2
    StrCpy $INSTDIR "$2\Planner"

  instdir_done:

  StrCpy $gtk_mode "private" ; default
  StrCpy $gtk_translations "no" ; default

FunctionEnd

Function .onselchange
	; Remember which gtk section was selected.
	; Deselect the other section.

	; If it was private, we check if public is checked and uncheck private.

	StrCmp $gtk_mode "private" check_public  ; old selection
	StrCmp $gtk_mode "public" check_private  ; old selection
	goto check_exit

	check_public:
		SectionGetFlags ${SecGtkPublic} $gtk_tmp  ; see if it's checked
		IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
		IntCmp $gtk_tmp ${SF_SELECTED} "" check_exit check_exit
		SectionGetFlags ${SecGtkPrivate} $gtk_tmp  ; unselect the other one
		IntOp $gtk_tmp $gtk_tmp & ${SECTION_OFF}
		SectionSetFlags ${SecGtkPrivate} $gtk_tmp
		goto check_exit

	check_private:
		SectionGetFlags ${SecGtkPrivate} $gtk_tmp  ; see if it's checked
		IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
		IntCmp $gtk_tmp ${SF_SELECTED} "" check_exit check_exit
		SectionGetFlags ${SecGtkPublic} $gtk_tmp  ; unselect the other one
		IntOp $gtk_tmp $gtk_tmp & ${SECTION_OFF}
		SectionSetFlags ${SecGtkPublic} $gtk_tmp

	check_exit:


	; store the current mode
	StrCpy $gtk_mode "none"

	SectionGetFlags ${SecGtkPrivate} $gtk_tmp
	IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
	IntCmp $gtk_tmp ${SF_SELECTED} "" mode_end_private mode_end_private
	StrCpy $gtk_mode "private"
	mode_end_private:

	SectionGetFlags ${SecGtkPublic} $gtk_tmp
	IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
	IntCmp $gtk_tmp ${SF_SELECTED} "" mode_end_public mode_end_public
	StrCpy $gtk_mode "public"
	mode_end_public:

	; store whether translations were selected for Planner so we can reuse it during GTK+ installation
	StrCpy $gtk_translations "no"
	SectionGetFlags ${SecTranslations} $gtk_tmp
	IntOp $gtk_tmp $gtk_tmp & ${SF_SELECTED}
	IntCmp $gtk_tmp ${SF_SELECTED} "" mode_end_translations mode_end_translations
	StrCpy $gtk_translations "yes"
	mode_end_translations:

	;MessageBox MB_ICONINFORMATION|MB_OK "gtk_mode: $gtk_mode" /SD IDOK
FunctionEnd

Function on_components_page_leave
	StrCmp $gtk_mode "none" "" noabort
		Call AskForGtk
	noabort:
FunctionEnd

Function un.onInit
  Call un.RunCheck
  StrCpy $name "Planner ${PLANNER_VERSION}"

  ; Get stored language prefrence
  ReadRegStr $LANGUAGE HKCU ${PLANNER_REG_KEY} "${PLANNER_REG_LANG}"

FunctionEnd

; GetParameters
; input, none
; output, top of stack (replaces, with e.g. whatever)
; modifies no other variables.

Function GetParameters

   Push $R0
   Push $R1
   Push $R2
   Push $R3

   StrCpy $R2 1
   StrLen $R3 $CMDLINE

   ;Check for quote or space
   StrCpy $R0 $CMDLINE $R2
   StrCmp $R0 '"' 0 +3
     StrCpy $R1 '"'
     Goto loop
   StrCpy $R1 " "

   loop:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 $R1 get
     StrCmp $R2 $R3 get
     Goto loop

   get:
     IntOp $R2 $R2 + 1
     StrCpy $R0 $CMDLINE 1 $R2
     StrCmp $R0 " " get
     StrCpy $R0 $CMDLINE "" $R2

   Pop $R3
   Pop $R2
   Pop $R1
   Exch $R0

FunctionEnd

; StrStr
 ; input, top of stack = string to search for
 ;        top of stack-1 = string to search in
 ; output, top of stack (replaces with the portion of the string remaining)
 ; modifies no other variables.
 ;
 ; Usage:
 ;   Push "this is a long ass string"
 ;   Push "ass"
 ;   Call StrStr
 ;   Pop $R0
 ;  ($R0 at this point is "ass string")

 Function StrStr
 Exch $R1 ; st=haystack,old$R1, $R1=needle
   Exch    ; st=old$R1,haystack
   Exch $R2 ; st=old$R1,old$R2, $R2=haystack
   Push $R3
   Push $R4
   Push $R5
   StrLen $R3 $R1
   StrCpy $R4 0
   ; $R1=needle
   ; $R2=haystack
   ; $R3=len(needle)
   ; $R4=cnt
   ; $R5=tmp
   loop:
     StrCpy $R5 $R2 $R3 $R4
     StrCmp $R5 $R1 done
     StrCmp $R5 "" done
     IntOp $R4 $R4 + 1
     Goto loop
 done:
   StrCpy $R1 $R2 "" $R4
   Pop $R5
   Pop $R4
   Pop $R3
   Pop $R2
   Exch $R1
 FunctionEnd

;
; Parse the Command line
;
; Unattended install command line parameters
; /L=Language e.g.: /L=1033
;
Function ParseParameters
  IntOp $LANG_IS_SET 0 + 0
  Call GetParameters
  Pop $R0
  Push $R0
  Push "L="
  Call StrStr
  Pop $R1
  StrCmp $R1 "" next
    StrCpy $R1 $R1 4 2 ; Strip first 2 chars of string
    StrCpy $LANGUAGE $R1
    IntOp $LANG_IS_SET 0 + 1
  next:
FunctionEnd

; detect GTK installation (any of available versions)
Function AskForGtk
	SetShellVarContext all  ; use all user variables as opposed to current user
	push $R0

	ReadRegStr $R0 HKLM "SOFTWARE\GTK\2.0" "DllPath"
	StrCmp $R0 "" no_gtk have_gtk

	no_gtk:
		MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
		"GTK2-Runtime is not installed. This product needs it to function properly.$\n\
		Please install GTK2-Runtime from http://gtk-win.sf.net/ first.$\n$\n\
		Click 'Cancel' to abort the installation \
		or 'OK' to continue anyway." \
		/SD IDOK IDOK have_gtk
		;Abort  ; Abort has different meaning from onpage callbacks, so use Quit
		Quit
		goto end_gtk_check

	have_gtk:
		; do nothing

	end_gtk_check:

	pop $R0
FunctionEnd
