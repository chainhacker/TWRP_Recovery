#ifndef _TWRPFUNCTIONS_HPP
#define _TWRPFUNCTIONS_HPP

#include <string>

using namespace std;

typedef enum
{
    rb_current = 0,
    rb_system,
    rb_recovery,
    rb_poweroff,
    rb_bootloader,     // May also be fastboot
    rb_download,
} RebootCommand;

// Partition class
class TWFunc
{
public:
	static int Check_MD5(string File);
	static string Get_Root_Path(string Path);                                   // Trims any trailing folders or filenames from the path, also adds a leading / if not present
	static string Get_Path(string Path);                                        // Trims everything after the last / in the string
	static string Get_Filename(string Path);                                    // Trims the path off of a filename

	static void install_htc_dumlock(void);                                      // Installs HTC Dumlock
	static void htc_dumlock_restore_original_boot(void);                        // Restores the backup of boot from HTC Dumlock
	static void htc_dumlock_reflash_recovery_to_boot(void);                     // Reflashes the current recovery to boot
	static int Recursive_Mkdir(string Path);                                    // Recursively makes the entire path
	static unsigned long long Get_Folder_Size(string Path, bool Display_Error); // Gets the size of a folder and all of its subfolders using dirent and stat
	static bool Path_Exists(string Path);                                       // Returns true if the path exists
	static void GUI_Operation_Text(string Read_Value, string Default_Text);     // Updates text for display in the GUI, e.g. Backing up %partition name%
	static void GUI_Operation_Text(string Read_Value, string Partition_Name, string Default_Text); // Same as above but includes partition name
	static unsigned long Get_File_Size(string Path);                            // Returns the size of a file
	static void twfinish_recovery(const char *send_intent);                     // Writes the log to last_log
	static int tw_reboot(RebootCommand command);                                // Prepares the device for rebooting
	static void check_and_run_script(const char* script_file, const char* display_name); // checks for the existence of a script, chmods it to 755, then runs it

private:
	static void check_and_fclose(FILE *fp, const char *name);
	static void copy_log_file(const char* source, const char* destination, int append);

};

#endif // _TWRPFUNCTIONS_HPP
