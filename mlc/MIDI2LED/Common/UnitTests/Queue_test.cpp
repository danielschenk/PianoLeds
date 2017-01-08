/**
 * @file
 * @copyright (c) Daniel Schenk, 2017
 * This file is part of MLC: MIDI Led strip Controller.
 *
 * @date 5 jan 2017
 *
 * @brief Queue tests.
 */

#include <array>
#include <gtest/gtest.h>

#include "../Queue.h"

#define MAX_NUM_ITEMS  1024

template <typename T>
class QueueTest
    : public ::testing::Test
{
protected:
    // Tried to use these instead of defines, but caused linker errors
//    static constexpr unsigned int c_MaxNumItems = 1024;

    virtual void SetUp()
    {
        Queue_Initialize(&m_queue, m_storage, sizeof(T), MAX_NUM_ITEMS);
    }

    Queue_t m_queue;
    T m_storage[MAX_NUM_ITEMS];
};

// Required declarations for typed tests.
typedef ::testing::Types<char, int, unsigned int> MyTypes;
TYPED_TEST_CASE(QueueTest, MyTypes);


TYPED_TEST(QueueTest, EmptyOnInit)
{
    ASSERT_EQ(this->m_queue.count, 0);
}

TYPED_TEST(QueueTest, StorageLocationInitialized)
{
    ASSERT_EQ(this->m_queue.pStorage, (uint8_t *)this->m_storage);
}

TYPED_TEST(QueueTest, ItemSizeInitialized)
{
    ASSERT_EQ(this->m_queue.itemSize, sizeof(TypeParam));
}

TYPED_TEST(QueueTest, NumItemsInitialized)
{
    ASSERT_EQ(this->m_queue.maxNumberOfItems, MAX_NUM_ITEMS);
}

TYPED_TEST(QueueTest, PopFailsWhenEmpty)
{
    TypeParam pop = 42;
    ASSERT_EQ(Queue_Pop(&this->m_queue, &pop), false);
    ASSERT_EQ(pop, 42);
}

TYPED_TEST(QueueTest, PushAndPopOne)
{
    const TypeParam push = 42;

    ASSERT_EQ(Queue_Push(&this->m_queue, &push), true);
    ASSERT_EQ(this->m_queue.count, 1);

    TypeParam pop;
    ASSERT_EQ(Queue_Pop(&this->m_queue, &pop), true);
    ASSERT_EQ(push, pop);
    ASSERT_EQ(this->m_queue.count, 0);
}

TYPED_TEST(QueueTest, PushAndPopMax)
{
    std::array<TypeParam, MAX_NUM_ITEMS> values;
    TypeParam value = 0;

    for(int i = 0; i < MAX_NUM_ITEMS; ++i)
    {
        values[i] = value;
        ASSERT_EQ(Queue_Push(&this->m_queue, &value), true);
        ++value;
    }

    ASSERT_EQ(this->m_queue.count, MAX_NUM_ITEMS);

    for(int i = 0; i < MAX_NUM_ITEMS; ++i)
    {
        ASSERT_EQ(Queue_Pop(&this->m_queue, &value), true);
        ASSERT_EQ(value, values[i]);
    }

    ASSERT_EQ(this->m_queue.count, 0);
}

TYPED_TEST(QueueTest, PushFailsWhenFull)
{
    const TypeParam push = 42;

    for(int i = 0; i < MAX_NUM_ITEMS; ++i)
    {
        ASSERT_EQ(Queue_Push(&this->m_queue, &push), true);
    }
    ASSERT_EQ(Queue_Push(&this->m_queue, &push), false);
}


