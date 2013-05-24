/* OpenRecoveryScript class for TWRP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * The code was written from scratch by Dees_Troy dees_troy at
 * yahoo
 *
 * Copyright (c) 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <vector>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#include "twrp-functions.hpp"
#include "partitions.hpp"
#include "common.h"
#include "openrecoveryscript.hpp"
#include "variables.h"
extern "C" {
#include "data.h"
#include "twinstall.h"
int TWinstall_zip(const char* path, int* wipe_cache);
}

static const char *SCRIPT_FILE_CACHE = "/cache/recovery/openrecoveryscript";
static const char *SCRIPT_FILE_TMP = "/tmp/openrecoveryscript";
#define SCRIPT_COMMAND_SIZE 512

int OpenRecoveryScript::check_for_script_file(void) {
	char exec[512];

	if (!PartitionManager.Mount_By_Path(SCRIPT_FILE_CACHE, false)) {
		LOGE("Unable to mount /cache for OpenRecoveryScript support.\n");
		return 0;
	}
	if (TWFunc::Path_Exists(SCRIPT_FILE_CACHE)) {
		LOGI("Script file found: '%s'\n", SCRIPT_FILE_CACHE);
		// Copy script file to /tmp
		strcpy(exec, "cp ");
		strcat(exec, SCRIPT_FILE_CACHE);
		strcat(exec, " ");
		strcat(exec, SCRIPT_FILE_TMP);
		system(exec);
		// Delete the file from /cache
		strcpy(exec, "rm ");
		strcat(exec, SCRIPT_FILE_CACHE);
		system(exec);
		return 1;
	}
	return 0;
}

int OpenRecoveryScript::run_script_file(void) {
	FILE *fp = fopen(SCRIPT_FILE_TMP, "r");
	int ret_val = 0, cindex, line_len, i, remove_nl, install_cmd = 0;
	char script_line[SCRIPT_COMMAND_SIZE], command[SCRIPT_COMMAND_SIZE],
		 value[SCRIPT_COMMAND_SIZE], mount[SCRIPT_COMMAND_SIZE],
		 value1[SCRIPT_COMMAND_SIZE], value2[SCRIPT_COMMAND_SIZE];
	char *val_start, *tok;

	if (fp != NULL) {
		DataManager_SetIntValue(TW_SIMULATE_ACTIONS, 0);
		while (fgets(script_line, SCRIPT_COMMAND_SIZE, fp) != NULL && ret_val == 0) {
			cindex = 0;
			line_len = strlen(script_line);
			if (line_len < 2)
				continue; // there's a blank line or line is too short to contain a command
			//ui_print("script line: '%s'\n", script_line);
			for (i=0; i<line_len; i++) {
				if ((int)script_line[i] == 32) {
					cindex = i;
					i = line_len;
				}
			}
			memset(command, 0, sizeof(command));
			memset(value, 0, sizeof(value));
			if ((int)script_line[line_len - 1] == 10)
					remove_nl = 2;
				else
					remove_nl = 1;
			if (cindex != 0) {
				strncpy(command, script_line, cindex);
				LOGI("command is: '%s' and ", command);
				val_start = script_line;
				val_start += cindex + 1;
				strncpy(value, val_start, line_len - cindex - remove_nl);
				LOGI("value is: '%s'\n", value);
			} else {
				strncpy(command, script_line, line_len - remove_nl + 1);
				ui_print("command is: '%s' and there is no value\n", command);
			}
			if (strcmp(command, "install") == 0) {
				// Install Zip
				ret_val = Install_Command(value);
				install_cmd = -1;
			} else if (strcmp(command, "wipe") == 0) {
				// Wipe
				if (strcmp(value, "cache") == 0 || strcmp(value, "/cache") == 0) {
					ui_print("-- Wiping Cache Partition...\n");
					PartitionManager.Wipe_By_Path("/cache");
					ui_print("-- Cache Partition Wipe Complete!\n");
				} else if (strcmp(value, "dalvik") == 0 || strcmp(value, "dalvick") == 0 || strcmp(value, "dalvikcache") == 0 || strcmp(value, "dalvickcache") == 0) {
					ui_print("-- Wiping Dalvik Cache...\n");
					PartitionManager.Wipe_Dalvik_Cache();
					ui_print("-- Dalvik Cache Wipe Complete!\n");
				} else if (strcmp(value, "data") == 0 || strcmp(value, "/data") == 0 || strcmp(value, "factory") == 0 || strcmp(value, "factoryreset") == 0) {
					ui_print("-- Wiping Data Partition...\n");
					PartitionManager.Factory_Reset();
					ui_print("-- Data Partition Wipe Complete!\n");
				} else {
					LOGE("Error with wipe command value: '%s'\n", value);
					ret_val = 1;
				}
			} else if (strcmp(command, "backup") == 0) {
				// Backup
				tok = strtok(value, " ");
				strcpy(value1, tok);
				tok = strtok(NULL, " ");
				if (tok != NULL) {
					memset(value2, 0, sizeof(value2));
					strcpy(value2, tok);
					line_len = strlen(tok);
					if ((int)value2[line_len - 1] == 10 || (int)value2[line_len - 1] == 13) {
						if ((int)value2[line_len - 1] == 10 || (int)value2[line_len - 1] == 13)
							remove_nl = 2;
						else
							remove_nl = 1;
					} else
						remove_nl = 0;
					strncpy(value2, tok, line_len - remove_nl);
					DataManager_SetStrValue(TW_BACKUP_NAME, value2);
					ui_print("Backup folder set to '%s'\n", value2);
					if (PartitionManager.Check_Backup_Name(true) != 0) {
						ret_val = 1;
						continue;
					}
				} else {
					char empt[50];
					strcpy(empt, "(Current Date)");
					DataManager_SetStrValue(TW_BACKUP_NAME, empt);
				}
				ret_val = Backup_Command(value1);
			} else if (strcmp(command, "restore") == 0) {
				// Restore
				PartitionManager.Mount_All_Storage();
				DataManager_SetIntValue(TW_SKIP_MD5_CHECK_VAR, 0);

				string val = value, restore_folder, restore_partitions;
				size_t pos = val.find_last_of(" ");
				if (pos == string::npos) {
					ui_print("Malformed restore parameter: '%s'\n", value1);
					ret_val = 1;
					continue;
				}
				restore_folder = val.substr(0, pos);
				char folder_path[512], partitions[512];
				strcpy(folder_path, restore_folder.c_str());
				restore_partitions = val.substr(pos + 1, val.size() - pos - 1);
				strcpy(partitions, restore_partitions.c_str());
				LOGI("Restore folder is: '%s' and partitions: '%s'\n", folder_path, partitions);
				ui_print("Restoring '%s'\n", folder_path);

				if (folder_path[0] != '/') {
					char backup_folder[512];
					sprintf(backup_folder, "%s/%s", DataManager_GetStrValue(TW_BACKUPS_FOLDER_VAR), folder_path);
					LOGI("Restoring relative path: '%s'\n", backup_folder);
					if (!TWFunc::Path_Exists(backup_folder)) {
						if (DataManager_GetIntValue(TW_HAS_DUAL_STORAGE)) {
							if (DataManager_GetIntValue(TW_USE_EXTERNAL_STORAGE)) {
								LOGI("Backup folder '%s' not found on external storage, trying internal...\n", folder_path);
								DataManager_SetIntValue(TW_USE_EXTERNAL_STORAGE, 0);
							} else {
								LOGI("Backup folder '%s' not found on internal storage, trying external...\n", folder_path);
								DataManager_SetIntValue(TW_USE_EXTERNAL_STORAGE, 1);
							}
							sprintf(backup_folder, "%s/%s", DataManager_GetStrValue(TW_BACKUPS_FOLDER_VAR), folder_path);
							LOGI("2Restoring relative path: '%s'\n", backup_folder);
						}
					}
					strcpy(folder_path, backup_folder);
				} else {
					if (folder_path[strlen(folder_path) - 1] == '/')
						strcat(folder_path, ".");
					else
						strcat(folder_path, "/.");
				}
				if (!TWFunc::Path_Exists(folder_path)) {
					ui_print("Unable to locate backup '%s'\n", folder_path);
					ret_val = 1;
					continue;
				}
				DataManager_SetStrValue("tw_restore", folder_path);

				PartitionManager.Set_Restore_Files(folder_path);
				if (strlen(partitions) != 0) {
					int tw_restore_system = 0;
					int tw_restore_data = 0;
					int tw_restore_cache = 0;
					int tw_restore_recovery = 0;
					int tw_restore_boot = 0;
					int tw_restore_andsec = 0;
					int tw_restore_sdext = 0;
					int tw_restore_sp1 = 0;
					int tw_restore_sp2 = 0;
					int tw_restore_sp3 = 0;

					memset(value2, 0, sizeof(value2));
					strcpy(value2, partitions);
					ui_print("Setting restore options: '%s':\n", value2);
					line_len = strlen(value2);
					for (i=0; i<line_len; i++) {
						if ((value2[i] == 'S' || value2[i] == 's') && DataManager_GetIntValue(TW_RESTORE_SYSTEM_VAR) > 0) {
							tw_restore_system = 1;
							ui_print("System\n");
						} else if ((value2[i] == 'D' || value2[i] == 'd') && DataManager_GetIntValue(TW_RESTORE_DATA_VAR) > 0) {
							tw_restore_data = 1;
							ui_print("Data\n");
						} else if ((value2[i] == 'C' || value2[i] == 'c') && DataManager_GetIntValue(TW_RESTORE_CACHE_VAR) > 0) {
							tw_restore_cache = 1;
							ui_print("Cache\n");
						} else if ((value2[i] == 'R' || value2[i] == 'r') && DataManager_GetIntValue(TW_RESTORE_RECOVERY_VAR) > 0) {
							tw_restore_recovery = 1;
							ui_print("Recovery\n");
						} else if (value2[i] == '1' && DataManager_GetIntValue(TW_RESTORE_SP1_VAR) > 0) {
							tw_restore_sp1 = 1;
							ui_print("%s\n", "Special1");
						} else if (value2[i] == '2' && DataManager_GetIntValue(TW_RESTORE_SP2_VAR) > 0) {
							tw_restore_sp2 = 1;
							ui_print("%s\n", "Special2");
						} else if (value2[i] == '3' && DataManager_GetIntValue(TW_RESTORE_SP3_VAR) > 0) {
							tw_restore_sp3 = 1;
							ui_print("%s\n", "Special3");
						} else if ((value2[i] == 'B' || value2[i] == 'b') && DataManager_GetIntValue(TW_RESTORE_BOOT_VAR) > 0) {
							tw_restore_boot = 1;
							ui_print("Boot\n");
						} else if ((value2[i] == 'A' || value2[i] == 'a') && DataManager_GetIntValue(TW_RESTORE_ANDSEC_VAR) > 0) {
							tw_restore_andsec = 1;
							ui_print("Android Secure\n");
						} else if ((value2[i] == 'E' || value2[i] == 'e') && DataManager_GetIntValue(TW_RESTORE_SDEXT_VAR) > 0) {
							tw_restore_sdext = 1;
							ui_print("SD-Ext\n");
						} else if (value2[i] == 'M' || value2[i] == 'm') {
							DataManager_SetIntValue(TW_SKIP_MD5_CHECK_VAR, 1);
							ui_print("MD5 check skip is on\n");
						}
					}

					if (DataManager_GetIntValue(TW_RESTORE_SYSTEM_VAR) && !tw_restore_system)
						DataManager_SetIntValue(TW_RESTORE_SYSTEM_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_DATA_VAR) && !tw_restore_data)
						DataManager_SetIntValue(TW_RESTORE_DATA_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_CACHE_VAR) && !tw_restore_cache)
						DataManager_SetIntValue(TW_RESTORE_CACHE_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_RECOVERY_VAR) && !tw_restore_recovery)
						DataManager_SetIntValue(TW_RESTORE_RECOVERY_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_BOOT_VAR) && !tw_restore_boot)
						DataManager_SetIntValue(TW_RESTORE_BOOT_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_ANDSEC_VAR) && !tw_restore_andsec)
						DataManager_SetIntValue(TW_RESTORE_ANDSEC_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_SDEXT_VAR) && !tw_restore_sdext)
						DataManager_SetIntValue(TW_RESTORE_SDEXT_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_SP1_VAR) && !tw_restore_sp1)
						DataManager_SetIntValue(TW_RESTORE_SP1_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_SP2_VAR) && !tw_restore_sp2)
						DataManager_SetIntValue(TW_RESTORE_SP2_VAR, 0);
					if (DataManager_GetIntValue(TW_RESTORE_SP3_VAR) && !tw_restore_sp3)
						DataManager_SetIntValue(TW_RESTORE_SP3_VAR, 0);
				} else {
					ui_print("No restore options set.\n");
					ret_val = 1;
					continue;
				}
				PartitionManager.Run_Restore(folder_path);
				ui_print("Restore complete!\n");
			} else if (strcmp(command, "mount") == 0) {
				// Mount
				if (value[0] != '/') {
					strcpy(mount, "/");
					strcat(mount, value);
				} else
					strcpy(mount, value);
				if (PartitionManager.Mount_By_Path(mount, true))
					ui_print("Mounted '%s'\n", mount);
			} else if (strcmp(command, "unmount") == 0 || strcmp(command, "umount") == 0) {
				// Unmount
				if (value[0] != '/') {
					strcpy(mount, "/");
					strcat(mount, value);
				} else
					strcpy(mount, value);
				if (PartitionManager.UnMount_By_Path(mount, true))
					ui_print("Unmounted '%s'\n", mount);
			} else if (strcmp(command, "set") == 0) {
				// Set value
				tok = strtok(value, " ");
				strcpy(value1, tok);
				tok = strtok(NULL, " ");
				strcpy(value2, tok);
				ui_print("Setting '%s' to '%s'\n", value1, value2);
				DataManager_SetStrValue(value1, value2);
			} else if (strcmp(command, "mkdir") == 0) {
				// Make directory (recursive)
				ui_print("Making directory (recursive): '%s'\n", value);
				if (TWFunc::Recursive_Mkdir(value)) {
					LOGE("Unable to create folder: '%s'\n", value);
					ret_val = 1;
				}
			} else if (strcmp(command, "reboot") == 0) {
				// Reboot
			} else if (strcmp(command, "cmd") == 0) {
				if (cindex != 0) {
					system(value);
				} else {
					LOGE("No value given for cmd\n");
				}
			} else if (strcmp(command, "print") == 0) {
				ui_print("%s\n", value);
			} else {
				LOGE("Unrecognized script command: '%s'\n", command);
				ret_val = 1;
			}
		}
		fclose(fp);
		ui_print("Done processing script file\n");
	} else {
		LOGE("Error opening script file '%s'\n", SCRIPT_FILE_TMP);
		return 1;
	}
	if (install_cmd && DataManager_GetIntValue(TW_HAS_INJECTTWRP) == 1 && DataManager_GetIntValue(TW_INJECT_AFTER_ZIP) == 1) {
		ui_print("Injecting TWRP into boot image...\n");
		TWPartition* Boot = PartitionManager.Find_Partition_By_Path("/boot");
		if (Boot == NULL || Boot->Current_File_System != "emmc")
			system("injecttwrp --dump /tmp/backup_recovery_ramdisk.img /tmp/injected_boot.img --flash");
		else {
			string injectcmd = "injecttwrp --dump /tmp/backup_recovery_ramdisk.img /tmp/injected_boot.img --flash bd=" + Boot->Actual_Block_Device;
			system(injectcmd.c_str());
		}
		ui_print("TWRP injection complete.\n");
	}
	return ret_val;
}

int OpenRecoveryScript::Install_Command(string Zip) {
	// Install zip
	string ret_string;
	int ret_val = 0, wipe_cache = 0;

	PartitionManager.Mount_All_Storage();
	if (Zip.substr(0, 1) != "/") {
		// Relative path given
		string Full_Path;

		Full_Path = DataManager_GetCurrentStoragePath();
		Full_Path += "/" + Zip;
		LOGI("Full zip path: '%s'\n", Full_Path.c_str());
		if (!TWFunc::Path_Exists(Full_Path)) {
			ret_string = Locate_Zip_File(Full_Path, DataManager_GetCurrentStoragePath());
			if (!ret_string.empty()) {
				Full_Path = ret_string;
			} else if (DataManager_GetIntValue(TW_HAS_DUAL_STORAGE)) {
				if (DataManager_GetIntValue(TW_USE_EXTERNAL_STORAGE)) {
					LOGI("Zip file not found on external storage, trying internal...\n");
					DataManager_SetIntValue(TW_USE_EXTERNAL_STORAGE, 0);
				} else {
					LOGI("Zip file not found on internal storage, trying external...\n");
					DataManager_SetIntValue(TW_USE_EXTERNAL_STORAGE, 1);
				}
				Full_Path = DataManager_GetCurrentStoragePath();
				Full_Path += "/" + Zip;
				LOGI("Full zip path: '%s'\n", Full_Path.c_str());
				ret_string = Locate_Zip_File(Full_Path, DataManager_GetCurrentStoragePath());
				if (!ret_string.empty())
					Full_Path = ret_string;
			}
		}
		Zip = Full_Path;
	} else {
		// Full path given
		if (!TWFunc::Path_Exists(Zip)) {
			ret_string = Locate_Zip_File(Zip, DataManager_GetCurrentStoragePath());
			if (!ret_string.empty())
				Zip = ret_string;
		}
	}

	if (!TWFunc::Path_Exists(Zip)) {
		// zip file doesn't exist
		ui_print("Unable to locate zip file '%s'.\n", Zip.c_str());
		ret_val = 1;
	} else {
		ui_print("Installing zip file '%s'\n", Zip.c_str());
		ret_val = TWinstall_zip(Zip.c_str(), &wipe_cache);
	}
	if (ret_val != 0) {
		LOGE("Error installing zip file '%s'\n", Zip.c_str());
		ret_val = 1;
	} else if (wipe_cache)
		PartitionManager.Wipe_By_Path("/cache");

	return ret_val;
}

string OpenRecoveryScript::Locate_Zip_File(string Zip, string Storage_Root) {
	string Path = TWFunc::Get_Path(Zip);
	string File = TWFunc::Get_Filename(Zip);
	string pathCpy = Path;
	string wholePath;
	size_t pos = Path.find("/", 1);

	while (pos != string::npos)
	{
		pathCpy = Path.substr(pos, Path.size() - pos);
		wholePath = pathCpy + "/" + File;
		if (TWFunc::Path_Exists(wholePath))
			return wholePath;
		wholePath = Storage_Root + "/" + wholePath;
		if (TWFunc::Path_Exists(wholePath))
			return wholePath;

		pos = Path.find("/", pos + 1);
	}
	return "";
}

int OpenRecoveryScript::Backup_Command(string Options) {
	char value1[SCRIPT_COMMAND_SIZE];
	int line_len, i;

	strcpy(value1, Options.c_str());

	DataManager_SetIntValue(TW_BACKUP_SYSTEM_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_DATA_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_CACHE_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_RECOVERY_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_SP1_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_SP2_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_SP3_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_BOOT_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_ANDSEC_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_SDEXT_VAR, 0);
	DataManager_SetIntValue(TW_BACKUP_SDEXT_VAR, 0);
	DataManager_SetIntValue(TW_USE_COMPRESSION_VAR, 0);
	DataManager_SetIntValue(TW_SKIP_MD5_GENERATE_VAR, 0);

	ui_print("Setting backup options:\n");
	line_len = Options.size();
	for (i=0; i<line_len; i++) {
		if (Options.substr(i, 1) == "S" || Options.substr(i, 1) == "s") {
			DataManager_SetIntValue(TW_BACKUP_SYSTEM_VAR, 1);
			ui_print("System\n");
		} else if (Options.substr(i, 1) == "D" || Options.substr(i, 1) == "d") {
			DataManager_SetIntValue(TW_BACKUP_DATA_VAR, 1);
			ui_print("Data\n");
		} else if (Options.substr(i, 1) == "C" || Options.substr(i, 1) == "c") {
			DataManager_SetIntValue(TW_BACKUP_CACHE_VAR, 1);
			ui_print("Cache\n");
		} else if (Options.substr(i, 1) == "R" || Options.substr(i, 1) == "r") {
			DataManager_SetIntValue(TW_BACKUP_RECOVERY_VAR, 1);
			ui_print("Recovery\n");
		} else if (Options.substr(i, 1) == "1") {
			DataManager_SetIntValue(TW_BACKUP_SP1_VAR, 1);
			ui_print("%s\n", "Special1");
		} else if (Options.substr(i, 1) == "2") {
			DataManager_SetIntValue(TW_BACKUP_SP2_VAR, 1);
			ui_print("%s\n", "Special2");
		} else if (Options.substr(i, 1) == "3") {
			DataManager_SetIntValue(TW_BACKUP_SP3_VAR, 1);
			ui_print("%s\n", "Special3");
		} else if (Options.substr(i, 1) == "B" || Options.substr(i, 1) == "b") {
			DataManager_SetIntValue(TW_BACKUP_BOOT_VAR, 1);
			ui_print("Boot\n");
		} else if (Options.substr(i, 1) == "A" || Options.substr(i, 1) == "a") {
			DataManager_SetIntValue(TW_BACKUP_ANDSEC_VAR, 1);
			ui_print("Android Secure\n");
		} else if (Options.substr(i, 1) == "E" || Options.substr(i, 1) == "e") {
			DataManager_SetIntValue(TW_BACKUP_SDEXT_VAR, 1);
			ui_print("SD-Ext\n");
		} else if (Options.substr(i, 1) == "O" || Options.substr(i, 1) == "o") {
			DataManager_SetIntValue(TW_USE_COMPRESSION_VAR, 1);
			ui_print("Compression is on\n");
		} else if (Options.substr(i, 1) == "M" || Options.substr(i, 1) == "m") {
			DataManager_SetIntValue(TW_SKIP_MD5_GENERATE_VAR, 1);
			ui_print("MD5 Generation is off\n");
		}
	}
	if (!PartitionManager.Run_Backup()) {
		LOGE("Backup failed!\n");
		return 1;
	}
	ui_print("Backup complete!\n");
	return 0;
}
