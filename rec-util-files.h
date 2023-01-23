/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 *  - Description: Contains utility functions for file operations
 *
 */

#ifndef REC_UTIL_FILES_H
#define REC_UTIL_FILES_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FNAME_MAX 256
#define FENTRIES_MAX 256
#define FILEPATH_MAX 512

/**
 * Structure containing one file information
 *
 */
struct FileInfo {
	char Name[FNAME_MAX];
	unsigned int Size;
};

/**
 * Linked list node for sub-directories in a dir
 *
 */
struct DirEntry {
	char Name[FNAME_MAX];
	struct DirEntry *Next;
};

/**
 * Linked list node for files in a dir
 *
 */
struct FileEntry {
	struct FileInfo FileInfo;
	struct FileEntry *Next;
};

/**
 * Structure containing list of DIRs and swu-files in selected dir
 *
 */
struct DirInfo {
	unsigned int DirCount;
	unsigned int SwuCount;
	struct DirEntry *Dirs;
	struct FileEntry *SwuFiles;
};

const char *util_files_getExtension(const char *Filename);

int util_files_getFileSize(const char *Filepath);

void util_files_listAllSwuFiles(const char *SourceDir, struct DirInfo *DirInfo);

void util_files_sortList(struct DirInfo *DirInfo);

void util_files_deallocate(struct DirInfo *dirInfo);

int util_files_findLastSlash(const char *Filepath);

void util_files_removeParentEntry(struct DirInfo *DirInfo,
				  const char *TopDirPath,
				  const char *CurrentDirPath,
				  unsigned int *FileCount);

bool util_files_updateCurrentDir(char *CurrentDirPath, unsigned int MaxLen,
				 const char *SelectedDirName);

bool util_files_createFileEntryLabel(char *LabelText, const char *Filename,
				     const unsigned int Filesize);

bool util_files_isValidDirNameLength(const char *DirName);

#ifdef __cplusplus
}
#endif

#endif /* REC_UTIL_FILES_H */
