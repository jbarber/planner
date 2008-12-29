;;
;;  Language strings for the Windows Planner NSIS installer.
;;  Windows Code page: 1252
;;

; Make sure to update the PLANNER_MACRO_LANGUAGEFILE_END macro in
; langmacros.nsh when adding new strings to this file

!insertmacro PLANNER_MACRO_ADD_LANGUAGE "English"

; Startup Checks
!insertmacro PLANNER_MACRO_DEFINE_STRING INSTALLER_IS_RUNNING			"The installer is already running."
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_IS_RUNNING			"An instance of Planner is currently running. Exit Planner and then try again."
!insertmacro PLANNER_MACRO_DEFINE_STRING GTK_INSTALLER_NEEDED			"The GTK+ runtime environment is either missing or needs to be upgraded.$\rPlease install v${GTK_VERSION} or higher of the GTK+ runtime"

; License Page
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_LICENSE_BUTTON			"Accept >"
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_LICENSE_BOTTOM_TEXT		"$(^Name) is released under the GNU General Public License (GPL). The license is provided here for information purposes only. $_CLICK"

; Components Page
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_SECTION_TITLE			"Planner (required)"
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_TRANSLATIONS_SECTION_TITLE	"Translations"
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_SECTION_DESCRIPTION		"Core Planner files and dlls"
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_TRANSLATIONS_SECTION_DESCRIPTION	"Additional languages"

; Installer Finish Page
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_FINISH_VISIT_WEB_SITE		"Visit the Windows Planner Web Page"

; Planner Section Prompts and Texts
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_UNINSTALL_DESC			"Planner (remove only)"
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_PROMPT_WIPEOUT			"Your old Planner directory is about to be deleted. Would you like to continue?$\r$\rNote: Any non-standard plugins that you may have installed will be deleted.$\rPlanner user settings will not be affected."
!insertmacro PLANNER_MACRO_DEFINE_STRING PLANNER_PROMPT_DIR_EXISTS		"The installation directory you specified already exists. Any contents$\rwill be deleted. Would you like to continue?"

; Uninstall Section Prompts
!insertmacro PLANNER_MACRO_DEFINE_STRING un.PLANNER_UNINSTALL_ERROR_1		"The uninstaller could not find registry entries for Planner.$\rIt is likely that another user installed this application."
!insertmacro PLANNER_MACRO_DEFINE_STRING un.PLANNER_UNINSTALL_ERROR_2		"You do not have permission to uninstall this application."
