#include "nanomatch/core/order.hpp"
#include "nanomatch/memory/pool_allocator.hpp"

#include <gtest/gtest.h>

using namespace nm;

TEST(PoolAllocatorTest, AllocateAndDeallocate) {
    PoolAllocator<int> pool(10);

    EXPECT_EQ(pool.size(), 0u);
    EXPECT_EQ(pool.capacity(), 10u);
    EXPECT_FALSE(pool.full());

    int* ptr = pool.allocate();
    ASSERT_NE(ptr, nullptr);
    *ptr = 42;
    EXPECT_EQ(*ptr, 42);
    EXPECT_EQ(pool.size(), 1u);

    pool.deallocate(ptr);
    EXPECT_EQ(pool.size(), 0u);
}

TEST(PoolAllocatorTest, FillPool) {
    PoolAllocator<int> pool(5);

    std::vector<int*> ptrs;
    for (int i = 0; i < 5; ++i) {
        int* ptr = pool.allocate();
        ASSERT_NE(ptr, nullptr);
        *ptr = i;
        ptrs.push_back(ptr);
    }

    EXPECT_TRUE(pool.full());
    EXPECT_EQ(pool.size(), 5u);

    // Pool exhausted
    int* null_ptr = pool.allocate();
    EXPECT_EQ(null_ptr, nullptr);

    // Free one and reallocate
    pool.deallocate(ptrs[0]);
    EXPECT_EQ(pool.size(), 4u);

    int* reused = pool.allocate();
    ASSERT_NE(reused, nullptr);
    EXPECT_EQ(pool.size(), 5u);
}

TEST(PoolAllocatorTest, OrderPool) {
    PoolAllocator<Order> pool(100);

    Order* o = pool.allocate();
    ASSERT_NE(o, nullptr);

    o->id = 42;
    o->side = Side::Buy;
    o->price = 10000;
    o->initial_quantity = 100;
    o->remaining_quantity = 100;

    EXPECT_EQ(o->id, 42u);
    EXPECT_EQ(o->price, 10000);

    pool.deallocate(o);
    EXPECT_EQ(pool.size(), 0u);
}

TEST(PoolAllocatorTest, DeallocateAndReuse) {
    PoolAllocator<int> pool(3);

    int* a = pool.allocate();
    int* b = pool.allocate();
    int* c = pool.allocate();

    *a = 1;
    *b = 2;
    *c = 3;

    // Free b
    pool.deallocate(b);
    EXPECT_EQ(pool.size(), 2u);

    // Next allocate reuses b's slot
    int* d = pool.allocate();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(pool.size(), 3u);

    pool.deallocate(a);
    pool.deallocate(c);
    pool.deallocate(d);
    EXPECT_EQ(pool.size(), 0u);
}
