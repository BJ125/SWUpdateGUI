/**
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024 IFM Ecomatic GmbH
 *
 * */

#include "rec-util-linked-list.h"

#include <gtest/gtest.h>

#include <string>

namespace test_rec_util_linked_list {

using namespace testing;

class UtilLinkedListTest : public Test {
 public:
  UtilLinkedListTest();
  ~UtilLinkedListTest();

  LinkedList* list;
};

UtilLinkedListTest::UtilLinkedListTest() : list(util_linked_list_create()) {}

UtilLinkedListTest::~UtilLinkedListTest() {
  util_linked_list_release(list);
}

TEST_F(UtilLinkedListTest,
       test_util_linked_list_get_first_element__empty_list) {
  auto it = util_linked_list_get_first_element(list);

  EXPECT_TRUE(NULL == it);
}

TEST_F(UtilLinkedListTest,
       test_util_linked_list_get_first_element__after_adding_elements) {
  std::string str = "some string";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);
  const int value = 5;
  util_linked_list_push_element(list, &value, sizeof(value));

  auto it = util_linked_list_get_first_element(list);

  EXPECT_TRUE(NULL != it);
}

TEST_F(UtilLinkedListTest, test_util_linked_list_get_size) {
  EXPECT_EQ(0, util_linked_list_get_size(list));

  std::string str = "some string";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);
  const int value = 5;
  util_linked_list_push_element(list, &value, sizeof(value));
  str = "123 abc";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);

  EXPECT_EQ(3, util_linked_list_get_size(list));
}

TEST_F(UtilLinkedListTest, test_util_linked_list_get_size_noListPassed) {
  EXPECT_EQ(util_linked_list_get_size(NULL), 0);
}

TEST_F(UtilLinkedListTest, test_iterate_list) {
  std::string str = "some string";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);
  const uint32_t value = 0xdeadbeef;
  util_linked_list_push_element(list, &value, sizeof(value));
  str = "123 abc";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);

  // element 1
  auto it = util_linked_list_get_first_element(list);

  const char* pData1 =
      reinterpret_cast<const char*>(util_linked_list_get_data(it));
  unsigned int dataSize1 = util_linked_list_get_data_size(it);
  EXPECT_STREQ(pData1, "some string");
  EXPECT_EQ(dataSize1, 12);

  // element 2
  it = util_linked_list_iterate_to_next_element(list, it);

  const unsigned int pData2 =
      *reinterpret_cast<const unsigned int*>(util_linked_list_get_data(it));
  const unsigned int dataSize2 = util_linked_list_get_data_size(it);
  EXPECT_EQ(pData2, 0xdeadbeef);
  EXPECT_EQ(dataSize2, 4);

  // element 3
  it = util_linked_list_iterate_to_next_element(list, it);

  const char* pData3 =
      reinterpret_cast<const char*>(util_linked_list_get_data(it));
  const unsigned int dataSize3 = util_linked_list_get_data_size(it);
  EXPECT_STREQ(pData3, "123 abc");
  EXPECT_EQ(dataSize3, 8);

  // element 4 and further elements are NULL
  it = util_linked_list_iterate_to_next_element(list, it);
  EXPECT_TRUE(NULL == it);

  it = util_linked_list_iterate_to_next_element(list, it);
  EXPECT_TRUE(NULL == it);
}

TEST_F(UtilLinkedListTest, test_util_linked_list_get_first_element_listIsNull) {
  const LinkedListNode* it = util_linked_list_get_first_element(NULL);
  EXPECT_TRUE(it == NULL);
}

TEST_F(UtilLinkedListTest,
       test_util_linked_list_iterate_to_next_element_listIsNull) {
  // add elements
  std::string str = "some string";
  util_linked_list_push_element(list, str.c_str(), str.length() + 1);
  const uint32_t value = 0xdeadbeef;
  util_linked_list_push_element(list, &value, sizeof(value));

  // it points to the 1st element
  const LinkedListNode* it = util_linked_list_get_first_element(NULL);

  // list is NULL
  it = util_linked_list_iterate_to_next_element(NULL, it);
  EXPECT_TRUE(it == NULL);

  // node is null
  it = util_linked_list_iterate_to_next_element(list, NULL);
  EXPECT_TRUE(it == NULL);

  // it points to the 1st element
  it = util_linked_list_get_first_element(NULL);
  it = util_linked_list_iterate_to_next_element(list, NULL);
  // it points to the 2nd element
  it = util_linked_list_iterate_to_next_element(list, NULL);
  // there is no element after the last and that is why is is NULL
  EXPECT_TRUE(it == NULL);
}

}  // namespace test_rec_util_linked_list
