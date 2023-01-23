/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 * */

#include "rec-util-files.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace test_rec_util_files {

namespace {

void CreateTmpFileWithContents(std::string fileName, std::string text) {
  std::ofstream myfile;
  myfile.open(fileName);
  myfile << text;

  sync();
}

void CreateLocalTmpLinks(std::vector<std::string>& fileList,
                         std::vector<std::string>& linkList) {
  for (std::string filename : fileList) {
    std::string linkFile = filename + ".link.swu";
    int status = symlink(filename.c_str(), linkFile.c_str());
    if (0 != status) {
      throw std::string("Error creating symlink error-code: " +
                        std::to_string(errno));
    }
    linkList.push_back(linkFile);
  }
  sync();
}

std::string GetListOfDirs(DirInfo* dirInfo) {
  std::string output = "";
  DirEntry* current = dirInfo->Dirs;

  while (NULL != current) {
    output.append(current->Name).append("\n");

    current = current->Next;
  }
  return output;
}

std::string GetStringOfLength(const unsigned int length) {
  std::string string(
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  while (string.length() < length) {
    string += string;
  }
  return string.substr(0, length);
}

}  // namespace

using namespace testing;

class UtilFilesTest : public Test {
 public:
  UtilFilesTest();
  ~UtilFilesTest();

 protected:
  std::vector<std::string> CreateMultipleLocalTmpFiles(unsigned int count);
  std::vector<std::string> CreateLocalTmpDir(unsigned int count);

  std::string tmpDirName;
};

UtilFilesTest::UtilFilesTest() : tmpDirName("") {
  char dirTemplate[25] = "/tmp/fileXXXXXX";
  char* dirName = mkdtemp(dirTemplate);

  if (NULL == dirName) {
    throw std::string("Error creating local-temp folder errcode: " +
                      std::to_string(errno));
  }
  tmpDirName = dirName;
}

UtilFilesTest::~UtilFilesTest() {
  std::filesystem::remove_all(tmpDirName);
}

std::vector<std::string> UtilFilesTest::CreateMultipleLocalTmpFiles(
    unsigned int count) {
  std::vector<std::string> fileList;
  for (unsigned int i = 0; i < count; i++) {
    std::stringstream ss;
    ss << tmpDirName << "/test.tmp" << std::setw(2) << std::setfill('0') << i
       << ".swu";
    std::string filename = ss.str();

    fileList.push_back(filename);
    CreateTmpFileWithContents(filename, "p");
  }

  sync();

  return fileList;
}

std::vector<std::string> UtilFilesTest::CreateLocalTmpDir(unsigned int count) {
  std::vector<std::string> dirList;
  for (unsigned int i = 0; i < count; i++) {
    std::stringstream ss;
    ss << tmpDirName << "/tmpdir_" << std::setw(2) << std::setfill('0') << i
       << ".swu";
    std::string dirname = ss.str();

    bool status = std::filesystem::create_directory(dirname);
    if (!status) {
      throw std::string("Error creating folder " + dirname);
    }
    dirList.push_back(dirname);
  }
  sync();

  return dirList;
}

TEST_F(UtilFilesTest, test_util_files_GetFileSize_pass) {
  const std::string tmpFileName = tmpDirName + "/abcfile_.tmp";

  CreateTmpFileWithContents(tmpFileName, "abcd\n");

  int size = util_files_getFileSize(tmpFileName.c_str());

  EXPECT_EQ(size, 5);
}

TEST_F(UtilFilesTest, test_util_files_GetFileSize_no_file) {
  int size = util_files_getFileSize("/tmp/xy9085768.tmp");

  EXPECT_LT(size, 0);
}

TEST_F(UtilFilesTest, test_util_files_ListAllSwuFiles_50_plus) {
  unsigned int fileCount = 50;
  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);

  // Issue possibly with qemu where dirent thinks /tmp is cmake_sysroot/tmp
  // while all other functions consider /tmp as system /tmp.
  // So using current folder for this test works as against /tmp.
  DirInfo dirInfo;

  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  EXPECT_EQ(dirInfo.SwuCount, fileCount);

  struct FileEntry* currentFile = dirInfo.SwuFiles;
  for (unsigned int i = 0; i < fileCount; i++) {
    std::string fullpath =
        tmpDirName + std::string("/") + currentFile->FileInfo.Name;

    currentFile = currentFile->Next;

    ASSERT_EQ(fullpath, fileList[i]) << fullpath;
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_ListAllSwuFiles_10_files) {
  const unsigned fileCount = 10;
  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);

  DirInfo dirInfo;
  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  ASSERT_EQ(dirInfo.SwuCount, fileCount);

  struct FileEntry* currentFile = dirInfo.SwuFiles;
  for (unsigned int i = 0; i < fileCount; i++) {
    std::string fullpath = tmpDirName + "/";
    fullpath += currentFile->FileInfo.Name;
    currentFile = currentFile->Next;

    ASSERT_EQ(fileList[i], fullpath) << fullpath;
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_ListAllSwuFiles_2_files_and_2_symlinks) {
  const unsigned fileCount = 2;
  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);

  std::vector<std::string> linkList;
  CreateLocalTmpLinks(fileList, linkList);

  ASSERT_EQ(fileCount, linkList.size());

  DirInfo dirInfo;
  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  ASSERT_EQ(dirInfo.SwuCount, fileCount);

  struct FileEntry* currentFile = dirInfo.SwuFiles;
  for (unsigned int i = 0; i < fileCount; i++) {
    std::string fullpath = tmpDirName + "/";
    fullpath += currentFile->FileInfo.Name;
    currentFile = currentFile->Next;

    ASSERT_EQ(fileList[i], fullpath) << fullpath;
  }

  ASSERT_EQ(currentFile, nullptr);

  currentFile = dirInfo.SwuFiles;
  for (unsigned int i = 0; i < fileCount; i++) {
    std::string fullpath = tmpDirName + "/";
    fullpath += currentFile->FileInfo.Name;
    currentFile = currentFile->Next;

    ASSERT_TRUE(linkList.end() ==
                std::find(linkList.begin(), linkList.end(), fullpath))
        << fullpath;
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_ListAllSwuFiles_2_files_and_dir) {
  const unsigned fileCount = 2;
  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);
  std::vector<std::string> dirList = CreateLocalTmpDir(fileCount);

  ASSERT_EQ(fileCount, dirList.size());

  DirInfo dirInfo;
  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  EXPECT_EQ(dirInfo.SwuCount, fileCount);
  EXPECT_EQ(dirInfo.DirCount, dirList.size() + 1)
      << GetListOfDirs(&dirInfo);  // Include ..

  struct FileEntry* currentFile = dirInfo.SwuFiles;
  for (unsigned int i = 0; i < fileCount; i++) {
    std::string fullpath = tmpDirName + "/";
    fullpath += currentFile->FileInfo.Name;
    currentFile = currentFile->Next;

    ASSERT_EQ(fileList[i], fullpath) << fullpath;
  }

  struct DirEntry* currentDir = dirInfo.Dirs;
  for (unsigned int i = 0; i < fileCount; i++) {
    // Skip ..
    if (0 != strcmp("..", currentDir->Name)) {
      std::string fullpath = tmpDirName + "/";
      fullpath += currentDir->Name;
      currentDir = currentDir->Next;

      ASSERT_EQ(dirList[i], fullpath) << fullpath;
    }
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_ListAllSwuFiles_invalid_dir) {
  DirInfo dirInfo;

  util_files_listAllSwuFiles("/pqr6758s9dd", &dirInfo);
  EXPECT_EQ(dirInfo.SwuCount, 0);

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_SortList_sort_empty_list) {
  DirInfo dirInfo;

  memset(&dirInfo, 0, sizeof(DirInfo));

  util_files_sortList(&dirInfo);

  ASSERT_EQ(0, dirInfo.DirCount);
  ASSERT_EQ(0, dirInfo.SwuCount);
}

TEST_F(UtilFilesTest, test_util_files_SortList_sort_only_files) {
  DirInfo dirInfo;

  memset(&dirInfo, 0, sizeof(DirInfo));

  for (unsigned int i = 0; i < 5; i++) {
    struct FileEntry* newFile =
        (struct FileEntry*)malloc(sizeof(struct FileEntry));
    memset(newFile, 0, sizeof(struct FileEntry));

    std::string file = "tmpabx" + std::to_string(10 - i) + ".swu";
    strncpy(newFile->FileInfo.Name, file.c_str(),
            sizeof(newFile->FileInfo.Name) - 1);

    newFile->Next = dirInfo.SwuFiles;
    dirInfo.SwuFiles = newFile;
  }

  util_files_sortList(&dirInfo);

  ASSERT_EQ(5, dirInfo.SwuCount);
  ASSERT_EQ(0, dirInfo.DirCount);

  struct FileEntry* current = dirInfo.SwuFiles;

  ASSERT_EQ(std::string("tmpabx10.swu"), current->FileInfo.Name);

  current = current->Next;

  for (unsigned int i = 0; i < 4; i++) {
    std::string file = "tmpabx" + std::to_string(i + 6) + ".swu";

    ASSERT_EQ(file, current->FileInfo.Name);

    current = current->Next;
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_SortList_sort_only_dirs) {
  DirInfo dirInfo;

  memset(&dirInfo, 0, sizeof(DirInfo));

  for (unsigned int i = 0; i < 5; i++) {
    struct DirEntry* newDir = (struct DirEntry*)malloc(sizeof(struct DirEntry));
    memset(newDir, 0, sizeof(struct DirEntry));

    std::string dir = "dirAp3_" + std::to_string(10 - i);
    strncpy(newDir->Name, dir.c_str(), sizeof(newDir->Name) - 1);

    newDir->Next = dirInfo.Dirs;
    dirInfo.Dirs = newDir;
  }

  util_files_sortList(&dirInfo);

  ASSERT_EQ(5, dirInfo.DirCount);
  ASSERT_EQ(0, dirInfo.SwuCount);

  struct DirEntry* current = dirInfo.Dirs;

  ASSERT_EQ(std::string("dirAp3_10"), current->Name) << GetListOfDirs(&dirInfo);

  current = current->Next;

  for (unsigned int i = 0; i < 4; i++) {
    std::string dir = "dirAp3_" + std::to_string(i + 6);

    ASSERT_EQ(dir, current->Name) << GetListOfDirs(&dirInfo);

    current = current->Next;
  }

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_ext_get_extension) {
  EXPECT_STREQ("txt", util_files_getExtension("./abc.txt"));
  EXPECT_STREQ("txt", util_files_getExtension("./abc.swu.txt"));
  EXPECT_STREQ("swu", util_files_getExtension("./abc.swu"));
  EXPECT_STREQ("swu", util_files_getExtension("./abc.txt.swu"));
  EXPECT_STREQ("", util_files_getExtension("."));
  EXPECT_STREQ("", util_files_getExtension(".."));
  EXPECT_STREQ("/", util_files_getExtension(".././"));
  EXPECT_STREQ("  ", util_files_getExtension("../.  "));
  EXPECT_STREQ("", util_files_getExtension(".swu"));
}

TEST_F(UtilFilesTest, test_util_files_FindLastSlash_get_index_of_last_slash) {
  EXPECT_EQ(-1, util_files_findLastSlash(""));
  EXPECT_EQ(-1, util_files_findLastSlash("___"));
  EXPECT_EQ(0, util_files_findLastSlash("/home"));
  EXPECT_EQ(5, util_files_findLastSlash("/home/root"));
  EXPECT_EQ(1, util_files_findLastSlash("//"));
  EXPECT_EQ(9, util_files_findLastSlash("/abc/pqr//"));
  EXPECT_EQ(22, util_files_findLastSlash("/home/root/test1/abc.m/intd"));
  EXPECT_EQ(5, util_files_findLastSlash("/home/this is a dir name"));
}

TEST_F(
    UtilFilesTest,
    test_util_files_RemoveParentEntryIfTopDir_remove_parent_entry_when_topdir) {
  const unsigned fileCount = 2;
  unsigned int numberOfFiles = fileCount;

  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);
  std::vector<std::string> dirList = CreateLocalTmpDir(fileCount);

  ASSERT_EQ(fileCount, dirList.size());

  DirInfo dirInfo;
  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  ASSERT_EQ(std::string(".."), dirInfo.Dirs->Name);

  util_files_removeParentEntry(&dirInfo, tmpDirName.c_str(), tmpDirName.c_str(),
                               &numberOfFiles);

  ASSERT_NE(std::string(".."), dirInfo.Dirs->Name);
  ASSERT_EQ(numberOfFiles, 1);

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest,
       test_util_files_RemoveParentEntryIfTopDir_do_not_remove_parent_entry) {
  const unsigned fileCount = 2;
  unsigned int numberOfFiles = fileCount;

  std::vector<std::string> fileList = CreateMultipleLocalTmpFiles(fileCount);
  std::vector<std::string> dirList = CreateLocalTmpDir(fileCount);

  ASSERT_EQ(fileCount, dirList.size());

  DirInfo dirInfo;
  util_files_listAllSwuFiles(tmpDirName.c_str(), &dirInfo);

  ASSERT_EQ(std::string(".."), dirInfo.Dirs->Name);

  util_files_removeParentEntry(&dirInfo, "/", tmpDirName.c_str(),
                               &numberOfFiles);

  ASSERT_EQ(std::string(".."), dirInfo.Dirs->Name);
  ASSERT_EQ(numberOfFiles, 2);

  util_files_deallocate(&dirInfo);
}

TEST_F(UtilFilesTest, test_util_files_UpdateCurrentDir_go_to_parent_dir) {
  char currentDir[FILEPATH_MAX];
  strcpy(currentDir, "/media/usb/sda");

  bool status = util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "..");

  ASSERT_STREQ(currentDir, "/media/usb") << currentDir;
  ASSERT_TRUE(status);
}

TEST_F(UtilFilesTest, test_util_files_UpdateCurrentDir_cant_go_above_media) {
  char currentDir[FILEPATH_MAX];
  strcpy(currentDir, "/media/usb");

  bool status = util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "..");

  ASSERT_STREQ(currentDir, "/media") << currentDir;
  ASSERT_TRUE(status);

  status = util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "..");

  ASSERT_STREQ(currentDir, "/media") << currentDir;
  ASSERT_FALSE(status);
}

TEST_F(UtilFilesTest, test_util_files_UpdateCurrentDir_go_to_sub_dir_single) {
  char currentDir[FILEPATH_MAX];
  strcpy(currentDir, "/media");
  const bool status =
      util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "user1");

  ASSERT_STREQ(currentDir, "/media/user1") << currentDir;
  ASSERT_TRUE(status);
}

TEST_F(UtilFilesTest, test_util_files_UpdateCurrentDir_go_to_sub_dir_multiple) {
  char currentDir[FILEPATH_MAX];
  strcpy(currentDir, "/media");
  bool status = util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "user1");

  ASSERT_STREQ(currentDir, "/media/user1") << currentDir;
  ASSERT_TRUE(status);

  status = util_files_updateCurrentDir(currentDir, FILEPATH_MAX, "usb_disk");

  ASSERT_STREQ(currentDir, "/media/user1/usb_disk") << currentDir;
  ASSERT_TRUE(status);
}

TEST_F(UtilFilesTest, test_util_files_CreateFileEntryLabel_success) {
  char text[FILEPATH_MAX];
  const bool success = util_files_createFileEntryLabel(text, "ABCD", 14);

  ASSERT_TRUE(success);
  ASSERT_STREQ(text, "ABCD (14 B)");
}

TEST_F(UtilFilesTest, test_util_files_CreateFileEntryLabel_failure) {
  char text[FILEPATH_MAX * 2];
  const std::string filename = GetStringOfLength(FILEPATH_MAX - 1);

  const bool success =
      util_files_createFileEntryLabel(text, filename.c_str(), 14);

  ASSERT_FALSE(success);
}

TEST_F(UtilFilesTest, test_util_files_IsValidDirNameLength_success) {
  const std::string filename = GetStringOfLength(10);

  const bool success = util_files_isValidDirNameLength(filename.c_str());

  ASSERT_TRUE(success);
}

TEST_F(UtilFilesTest, test_util_files_IsValidDirNameLength_boundary_case) {
  const std::string filename = GetStringOfLength(FILEPATH_MAX - 1);

  const bool success = util_files_isValidDirNameLength(filename.c_str());

  ASSERT_TRUE(success);
}

TEST_F(UtilFilesTest, test_util_files_IsValidDirNameLength_failure) {
  const std::string filename = GetStringOfLength(FILEPATH_MAX + 1);
  bool success = util_files_isValidDirNameLength(filename.c_str());
  ASSERT_FALSE(success);
}

}  // namespace test_rec_util_files
