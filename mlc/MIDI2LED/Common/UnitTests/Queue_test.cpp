/**
 * @file
 * @copyright (c) Daniel Schenk, 2017
 * This file is part of MLC: MIDI Led strip Controller.
 *
 * @date 5 jan 2017
 *
 * @brief Queue tests.
 */

#include <gtest/gtest.h>
#include "../Queue.h"

typedef int TestData_t;
#define TEST_DATA_SIZE sizeof(TestData_t)
#define MAX_NUM_ITEMS  1024

class QueueTest
    : public ::testing::Test
{
protected:
//    typedef int TestData_t;
//
    // Tried to use these instead of defines, but caused linker errors
//    static constexpr size_t c_TestDataSize = sizeof(TestData_t);
//    static constexpr unsigned int c_MaxNumItems = 1024;

    virtual void SetUp()
    {
        Queue_Initialize(&m_queue, m_storage, TEST_DATA_SIZE, MAX_NUM_ITEMS);
    }

    Queue_t m_queue;
    TestData_t m_storage[MAX_NUM_ITEMS];
};

TEST_F(QueueTest, EmptyOnInit)
{
    ASSERT_EQ(m_queue.count, 0);
}

TEST_F(QueueTest, StorageLocationInitialized)
{
    ASSERT_EQ(m_queue.pStorage, m_storage);
}

TEST_F(QueueTest, ItemSizeInitialized)
{
    ASSERT_EQ(m_queue.itemSize, TEST_DATA_SIZE);
}

TEST_F(QueueTest, NumItemsInitialized)
{
    ASSERT_EQ(m_queue.maxNumberOfItems, MAX_NUM_ITEMS);
}
