/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for file operations
 *
 */

#include "rec-util-files.h"

#include "rec-util-config.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>

static bool util_files_isCurrentDir(struct dirent *Ent)
{
	return (0 == strcmp(".", Ent->d_name));
}

static bool util_files_isDir(struct dirent *Ent)
{
	return (DT_DIR == Ent->d_type);
}

static bool util_files_isUpdateFile(struct dirent *Ent)
{
	const char *Extension = util_files_getExtension(Ent->d_name);

	if ((DT_REG == Ent->d_type) && (0 == strcmp(Extension, "swu"))) {
		return true;
	}

	return false;
}

static void util_files_setFileInfo(struct FileEntry *CurrentFile,
				   const char *SourceDir, struct dirent *Ent)
{
	char FilePath[FILENAME_MAX];

	strncpy(CurrentFile->FileInfo.Name, Ent->d_name,
		sizeof(CurrentFile->FileInfo.Name) - 1);
	CurrentFile->FileInfo.Name[FNAME_MAX - 1] = '\0';

	snprintf(FilePath, sizeof(FilePath), "%s/%s", SourceDir,
		 CurrentFile->FileInfo.Name);
	FilePath[sizeof(FilePath) - 1] = '\0';

	CurrentFile->FileInfo.Size = util_files_getFileSize(FilePath);
}

static void util_files_setDirInfo(struct DirEntry *CurrentDir,
				  struct dirent *Entry)
{
	strncpy(CurrentDir->Name, Entry->d_name, sizeof(CurrentDir->Name) - 1);
	CurrentDir->Name[FNAME_MAX - 1] = '\0';
}

static void util_files_allocateDirInfo(struct DirEntry **CurrentDirPtr)
{
	*CurrentDirPtr = malloc(sizeof(struct DirEntry));
	if (NULL == *CurrentDirPtr) {
		printf("Unable to allocate memory!\n Critical error!\n");
		exit(-1);
	}

	memset(*CurrentDirPtr, 0, sizeof(struct DirEntry));
}

static void util_files_allocateFileInfo(struct FileEntry **CurrentFilePtr)
{
	*CurrentFilePtr = malloc(sizeof(struct FileEntry));
	if (NULL == *CurrentFilePtr) {
		printf("Unable to allocate memory!\n Critical error!\n");
		exit(-1);
	}

	memset(*CurrentFilePtr, 0, sizeof(struct FileEntry));
}

static void util_files_pushFileInfo(struct DirInfo *DirInfo,
				    struct FileEntry *FileEntry)
{
	FileEntry->Next = DirInfo->SwuFiles;
	DirInfo->SwuFiles = FileEntry;
}

static void util_files_pushDirInfo(struct DirInfo *DirInfo,
				   struct DirEntry *DirEntry)
{
	DirEntry->Next = DirInfo->Dirs;
	DirInfo->Dirs = DirEntry;
}

static void util_files_pushSortedFiles(struct DirInfo *SortedDirInfo,
				       struct FileEntry *FileEntry)
{
	if ((NULL == SortedDirInfo->SwuFiles) ||
	    (0 <= strcmp(SortedDirInfo->SwuFiles->FileInfo.Name,
			 FileEntry->FileInfo.Name))) {
		FileEntry->Next = SortedDirInfo->SwuFiles;
		SortedDirInfo->SwuFiles = FileEntry;
	} else {
		struct FileEntry *Current = SortedDirInfo->SwuFiles;

		while ((NULL != Current->Next) &&
		       (0 > strcmp(Current->Next->FileInfo.Name,
				   FileEntry->FileInfo.Name))) {
			Current = Current->Next;
		}
		FileEntry->Next = Current->Next;
		Current->Next = FileEntry;
	}
}

static void util_files_pushSortedDirs(struct DirInfo *SortedDirInfo,
				      struct DirEntry *DirEntry)
{
	if ((NULL == SortedDirInfo->Dirs) ||
	    (0 <= strcmp(SortedDirInfo->Dirs->Name, DirEntry->Name))) {
		DirEntry->Next = SortedDirInfo->Dirs;
		SortedDirInfo->Dirs = DirEntry;
	} else {
		struct DirEntry *Current = SortedDirInfo->Dirs;

		while ((NULL != Current->Next) &&
		       (0 > strcmp(Current->Next->Name, DirEntry->Name))) {
			Current = Current->Next;
		}
		DirEntry->Next = Current->Next;
		Current->Next = DirEntry;
	}
}

static void util_files_sortFileList(struct DirInfo *DirInfo,
				    struct DirInfo *SortedDirInfo)
{
	struct FileEntry *Current = DirInfo->SwuFiles;

	while (NULL != Current) {
		struct FileEntry *Next = Current->Next;

		util_files_pushSortedFiles(SortedDirInfo, Current);

		SortedDirInfo->SwuCount++;

		Current = Next;
	}
}

static void util_files_sortDirList(struct DirInfo *DirInfo,
				   struct DirInfo *SortedDirInfo)
{
	struct DirEntry *Current = DirInfo->Dirs;

	while (NULL != Current) {
		struct DirEntry *Next = Current->Next;

		util_files_pushSortedDirs(SortedDirInfo, Current);

		SortedDirInfo->DirCount++;

		Current = Next;
	}
}

static void util_files_deallocateFileEntry(struct DirInfo *DirInfo)
{
	struct FileEntry *FileEntry = DirInfo->SwuFiles;
	while (NULL != FileEntry) {
		struct FileEntry *Tmp = FileEntry;

		FileEntry = FileEntry->Next;

		free(Tmp);
	}

	DirInfo->SwuFiles = NULL;
	DirInfo->SwuCount = 0;
}

static void util_files_deallocateDirInfo(struct DirInfo *DirInfo)
{
	struct DirEntry *DirEntry = DirInfo->Dirs;
	while (NULL != DirEntry) {
		struct DirEntry *Tmp = DirEntry;

		DirEntry = DirEntry->Next;

		free(Tmp);
	}

	DirInfo->Dirs = NULL;
	DirInfo->DirCount = 0;
}

/**
 * Get filename extension for valid-filenames
 *
 * @param[in] Filename File path
 *
 * @return extension
 */
const char *util_files_getExtension(const char *Filename)
{
	const char *Dot = strrchr(Filename, '.');
	if ((!Dot) || (Dot == Filename)) {
		return "";
	}
	return (Dot + 1);
}

/**
 * Get size of a file
 *
 * @param[in] Filepath Full path to a file
 *
 * @return size of file or -ve errno
 */
int util_files_getFileSize(const char *Filepath)
{
	struct stat St;
	if (0 != stat(Filepath, &St)) {
		fprintf(stderr,
			"Errorcode: %d\nError getting file-size for %s\n",
			errno, Filepath);
		return -errno;
	}

	return St.st_size;
}

/**
 * List all "swu" files & sub-directories in a directory
 * Parent dir entry("..") shall be present in the list
 * but not current dir(".")
 * Both dir & file lists are sorted alphabetically.
 *
 * @param[in] SourceDir Directory to read
 * @param[out] DirInfo Pointer to structure containing
 *                     directory-list & file-list
 *
 * Note: DirInfo must be deallocated after usage by using util_files_Dealloc()
 */
void util_files_listAllSwuFiles(const char *SourceDir, struct DirInfo *DirInfo)
{
	struct dirent *Entry;

	DirInfo->DirCount = 0;
	DirInfo->SwuCount = 0;
	DirInfo->Dirs = NULL;
	DirInfo->SwuFiles = NULL;

	DIR *Dir = opendir(SourceDir);
	if (NULL == Dir) {
		return;
	}

	Entry = readdir(Dir);
	while (NULL != Entry) {
		if (util_files_isUpdateFile(Entry)) {
			struct FileEntry *CurrentFile;
			util_files_allocateFileInfo(&CurrentFile);

			util_files_setFileInfo(CurrentFile, SourceDir, Entry);

			util_files_pushFileInfo(DirInfo, CurrentFile);

			DirInfo->SwuCount++;
		} else if (util_files_isDir(Entry)) {
			if (!util_files_isCurrentDir(Entry)) {
				struct DirEntry *CurrentDir;
				util_files_allocateDirInfo(&CurrentDir);

				util_files_setDirInfo(CurrentDir, Entry);

				util_files_pushDirInfo(DirInfo, CurrentDir);

				DirInfo->DirCount++;
			}
		}
		Entry = readdir(Dir);
	}

	closedir(Dir);

	util_files_sortList(DirInfo);
}

/**
 * Sort lists of dirs and files alphabetically
 *
 *
 * @param[in|out] DirInfo
 */
void util_files_sortList(struct DirInfo *DirInfo)
{
	struct DirInfo SortedDirInfo;

	memset(&SortedDirInfo, 0, sizeof(struct DirInfo));

	util_files_sortDirList(DirInfo, &SortedDirInfo);

	util_files_sortFileList(DirInfo, &SortedDirInfo);

	*DirInfo = SortedDirInfo;
}

/**
 * Free memory allocated to DirInfo
 *
 * @param[in] DirInfo Pointer to structure containing
 *                    list of dirs, files and filesizes
 */
void util_files_deallocate(struct DirInfo *DirInfo)
{
	util_files_deallocateFileEntry(DirInfo);

	util_files_deallocateDirInfo(DirInfo);
}

/**
 * Find last instance of '/'
 *
 * @param[in] Filepath File path to search in
 *
 * @return index of '/' or -1 if not found
 */
int util_files_findLastSlash(const char *Filepath)
{
	int Start = 0;
	int End = strlen(Filepath) - 1;

	while (Start <= End) {
		if ('/' == Filepath[End]) {
			return End;
		}
		End--;
	}

	return -1;
}

/**
 * Remove ".." from directory-list when current directory is top directory
 *
 * @param[in|out] DirInfo Structure containing list of sub-directories & SWU files
 *                        in CurrentDirPath
 * @param[in] TopDirPath Top directory path till which browsing is allowed
 * @param[in] CurrentDirPath Current directory path which user is about to browse
 * @param[out] CountOfFiles Number of files in dir-info
 */
void util_files_removeParentEntry(struct DirInfo *DirInfo,
				  const char *TopDirPath,
				  const char *CurrentDirPath,
				  unsigned int *CountOfFiles)
{
	if ((0 == strcmp("..", DirInfo->Dirs->Name)) &&
	    (0 == strcmp(TopDirPath, CurrentDirPath))) {
		struct DirEntry *Current = DirInfo->Dirs;

		DirInfo->Dirs = DirInfo->Dirs->Next;

		--(*CountOfFiles);

		free(Current);
	}
}

/**
 * If selected dir is "..", move one level up, else move one level down in CurrentDirPath
 *
 * @param[in|out] CurrentDirPath Path to current directory being browsed
 * @param[in] MaxLen Maximum length of currentDirPath
 * @param[in] SelectedDirName Selected dir entry, either ".." or one of subdirectories
 *
 * @return state of operation
 */
bool util_files_updateCurrentDir(char *CurrentDirPath, unsigned int MaxLen,
				 const char *SelectedDirName)
{
	if (0 == strcmp("..", SelectedDirName)) {
		int IndexOfSlash = util_files_findLastSlash(CurrentDirPath);
		if ((-1 == IndexOfSlash) || (0 == IndexOfSlash)) {
			printf("Invalid top dir: %s\n", CurrentDirPath);
			return false;
		}

		CurrentDirPath[IndexOfSlash] = '\0';
	} else {
		if (FNAME_MAX <
		    (strlen(CurrentDirPath) + strlen(SelectedDirName) + 2)) {
			return false;
		}

		strncat(CurrentDirPath, "/", 2);
		strncat(CurrentDirPath, SelectedDirName, FNAME_MAX);
		CurrentDirPath[MaxLen - 1] = '\0';
	}
	return true;
}

/**
 * Create label-text for file-entry in file-browser
 *
 * @param[out] LabelText Text for file-entry as filename (filesize B)
 * @param[in] FileName Name of file
 * @param[in] FileSize Size of file in bytes
 *
 * @return true for success, false for failure to create label
 */
bool util_files_createFileEntryLabel(char *LabelText, const char *FileName,
				     const unsigned int FileSize)
{
	int Ret = snprintf(LabelText, FILEPATH_MAX, "%s (%d B)", FileName,
			   FileSize);

	if ((0 < Ret) && (FILEPATH_MAX > Ret)) {
		return true;
	}
	return false;
}

/**
 * Check for directory-name length
 *
 * @param[in] DirName Name of directory
 *
 * @return true for valid, false for too-long dirname
 */
bool util_files_isValidDirNameLength(const char *DirName)
{
	if (FILEPATH_MAX <= strlen(DirName)) {
		return false;
	}
	return true;
}
