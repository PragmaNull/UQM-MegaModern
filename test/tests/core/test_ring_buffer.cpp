#include <gtest/gtest.h>

#include "core/stl/vector.h"
#include "core/stl/stl.h"

#include "core/stl/ring_buffer.h"

namespace uqm
{

// Detection idiom for emplace_back
template <typename, typename...>
struct has_emplace_back : std::false_type
{
};

template <typename R, typename... Args>
auto test_emplace_back(int) -> decltype(std::declval<R>().emplace_back(std::declval<Args>()...), std::true_type {});

template <typename R, typename... Args>
std::false_type test_emplace_back(...);

template <typename R, typename... Args>
struct has_emplace_back<R(Args...)> : decltype(test_emplace_back<R, Args...>(0))
{
};

// Helper alias to query std::ring_buffer<T,N>::emplace_back(...) support
template <typename RB, typename... Args>
inline constexpr bool has_emplace_v = decltype(test_emplace_back<RB, Args...>(0))::value;

using IntPair = std::pair<int, int>;

class RingBufferTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Optional setup code
	}

	void TearDown() override
	{
		// Optional cleanup code
	}
};

TEST_F(RingBufferTest, BasicPushPopFrontBack)
{
	std::ring_buffer<int, 4> rb;
	EXPECT_EQ(rb.capacity(), 4u);
	EXPECT_TRUE(rb.empty());
	EXPECT_FALSE(rb.full());
	EXPECT_EQ(rb.size(), 0u);

	rb.push_back(1);
	EXPECT_FALSE(rb.empty());
	EXPECT_EQ(rb.size(), 1u);
	EXPECT_EQ(rb.front(), 1);
	EXPECT_EQ(rb.back(), 1);

	rb.push_back(2);
	rb.push_back(3);
	EXPECT_EQ(rb.size(), 3u);
	EXPECT_EQ(rb.front(), 1);
	EXPECT_EQ(rb.back(), 3);

	rb.pop_front();
	EXPECT_EQ(rb.size(), 2u);
	EXPECT_EQ(rb.front(), 2);
	EXPECT_EQ(rb.back(), 3);

	rb.pop_front();
	rb.pop_front();
	EXPECT_TRUE(rb.empty());
	EXPECT_EQ(rb.size(), 0u);
}

TEST_F(RingBufferTest, FillAndOverwriteWhenFull)
{
	std::ring_buffer<int, 4> rb;
	rb.push_back(1);
	rb.push_back(2);
	rb.push_back(3);
	rb.push_back(4);

	EXPECT_TRUE(rb.full());
	EXPECT_EQ(rb.size(), 4u);
	EXPECT_EQ(rb.front(), 1);
	EXPECT_EQ(rb.back(), 4);

	// Overwrite oldest element
	rb.push_back(5);
	EXPECT_TRUE(rb.full());
	EXPECT_EQ(rb.size(), 4u);
	EXPECT_EQ(rb.front(), 2);
	EXPECT_EQ(rb.back(), 5);


	// Now contents should be 2,3,4,5
	std::vector<int> contents;
	auto it = rb.begin();
	const auto itEnd = rb.end();
	for (; it != itEnd; ++it)
	{
		contents.push_back(*it);
	}
	const std::vector<int> expected = {2, 3, 4, 5};
	EXPECT_EQ(contents, expected);
	EXPECT_EQ(rb.front(), 2);
	EXPECT_EQ(rb.back(), 5);
}

TEST_F(RingBufferTest, WrapAroundBehavior)
{
	std::ring_buffer<int, 5> rb; // non-power-of-two size
	// Fill partially and pop some to advance head, then push to cause wrap
	for (int i = 1; i <= 5; ++i)
	{
		rb.push_back(i); // fills 1..5 (full)
	}
	EXPECT_TRUE(rb.full());

	// pop two => should remove 1 and 2
	rb.pop_front();
	rb.pop_front();
	EXPECT_EQ(rb.size(), 3u);
	// push more items to wrap tail around
	rb.push_back(6);
	rb.push_back(7);
	// Now contents should be 3,4,5,6,7
	std::vector<int> got;
	for (auto it = rb.begin(); it != rb.end(); ++it)
	{
		got.push_back(*it);
	}
	const std::vector<int> expect = {3, 4, 5, 6, 7};
	EXPECT_EQ(got, expect);
	EXPECT_EQ(rb.front(), 3);
	EXPECT_EQ(rb.back(), 7);
}

TEST_F(RingBufferTest, IteratorsAndPostIncrement)
{
	std::ring_buffer<int, 4> rb;
	rb.push_back(10);
	rb.push_back(20);
	rb.push_back(30);

	auto it = rb.begin();
	ASSERT_EQ(*it, 10);

	// Test post-increment returns previous value
	auto old = it++;
	EXPECT_EQ(*old, 10);
	EXPECT_EQ(*it, 20);

	++it;
	EXPECT_EQ(*it, 30);

	// end iterator equals begin + size (iterator internal index)
	auto itEnd = rb.end();
	// iterate through all elements with range-style loop via iterators
	std::vector<int> vals;
	for (auto val : rb)
	{
		vals.push_back(val);
	}
	const std::vector<int> expected = {10, 20, 30};
	EXPECT_EQ(vals, expected);
}

TEST_F(RingBufferTest, ConstIteratorAndConstAccess)
{
	std::ring_buffer<int, 3> rb;
	rb.push_back(7);
	rb.push_back(8);

	const std::ring_buffer<int, 3>& crb = rb;
	std::vector<int> vals;
	for (auto it = crb.cbegin(); it != crb.cend(); ++it)
	{
		vals.push_back(*it);
	}
	const std::vector<int> expected = {7, 8};
	EXPECT_EQ(vals, expected);

	// const front/back access
	EXPECT_EQ(crb.front(), 7);
	EXPECT_EQ(crb.back(), 8);
}

TEST_F(RingBufferTest, ClearResetsState)
{
	std::ring_buffer<int, 4> rb;
	rb.push_back(1);
	rb.push_back(2);
	rb.push_back(3);
	EXPECT_FALSE(rb.empty());
	EXPECT_EQ(rb.size(), 3u);

	rb.clear();
	EXPECT_TRUE(rb.empty());
	EXPECT_EQ(rb.size(), 0u);
	EXPECT_FALSE(rb.full());

	// after clear, access should throw
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.front();
	}(),
				 std::out_of_range);
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.back();
	}(),
				 std::out_of_range);
	EXPECT_THROW(rb.pop_front(), std::out_of_range);
}

TEST_F(RingBufferTest, PopFrontOnEmptyThrowsAndFrontBackThrow)
{
	std::ring_buffer<int, 2> rb;
	EXPECT_TRUE(rb.empty());
	EXPECT_THROW(rb.pop_front(), std::out_of_range);
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.front();
	}(),
				 std::out_of_range);
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.back();
	}(),
				 std::out_of_range);

	rb.push_back(1);
	EXPECT_NO_THROW(uqstl::ignore = rb.front());
	EXPECT_NO_THROW(uqstl::ignore = rb.back());
	rb.pop_front();
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.front();
	}(),
				 std::out_of_range);
	EXPECT_THROW([&]() {
		uqstl::ignore = rb.back();
	}(),
				 std::out_of_range);
}

TEST_F(RingBufferTest, MoveOnlyElementSupport)
{
	struct MoveOnly
	{
		int v = 0;
		MoveOnly() = default;
		explicit MoveOnly(int x)
			: v(x)
		{
		}
		MoveOnly(MoveOnly&& o) noexcept
			: v(o.v)
		{
			o.v = -1;
		}
		MoveOnly& operator=(MoveOnly&& o) noexcept
		{
			v = o.v;
			o.v = -1;
			return *this;
		}
		MoveOnly(const MoveOnly&) = delete;
		MoveOnly& operator=(const MoveOnly&) = delete;
		bool operator==(const MoveOnly& o) const { return v == o.v; }
	};

	// ensure MoveOnly is default constructible (array requires it)
	static_assert(std::is_default_constructible_v<MoveOnly>);
	static_assert(!std::is_copy_constructible_v<MoveOnly>);
	static_assert(std::is_move_constructible_v<MoveOnly>);

	std::ring_buffer<MoveOnly, 3> rb;
	rb.push_back(MoveOnly(5));
	rb.push_back(MoveOnly(6));
	EXPECT_EQ(rb.size(), 2u);
	EXPECT_EQ(rb.front().v, 5);
	EXPECT_EQ(rb.back().v, 6);

	// push to full and cause overwrite
	rb.push_back(MoveOnly(7));
	rb.push_back(MoveOnly(8)); // overwrites oldest (5)
	EXPECT_EQ(rb.size(), 3u);
	std::vector<int> got;
	for (auto it = rb.begin(); it != rb.end(); ++it)
	{
		got.push_back(it->v);
	}
	// should be 6,7,8
	const std::vector<int> expect = {6, 7, 8};
	EXPECT_EQ(got, expect);
}

TEST_F(RingBufferTest, SizeAndFullEmptyConsistency)
{
	std::ring_buffer<int, 3> rb;
	EXPECT_TRUE(rb.empty());
	EXPECT_FALSE(rb.full());
	EXPECT_EQ(rb.size(), 0u);

	rb.push_back(1);
	EXPECT_FALSE(rb.empty());
	EXPECT_FALSE(rb.full());
	EXPECT_EQ(rb.size(), 1u);

	rb.push_back(2);
	rb.push_back(3);
	EXPECT_TRUE(rb.full());
	EXPECT_EQ(rb.size(), 3u);

	// further pushes keep size constant and full stays true
	rb.push_back(4);
	EXPECT_TRUE(rb.full());
	EXPECT_EQ(rb.size(), 3u);

	// pop reduces size and full becomes false
	rb.pop_front();
	EXPECT_EQ(rb.size(), 2u);
	EXPECT_FALSE(rb.full());
}


struct EmplaceFriendly
{
	int a {};
	int b {};
	uqstl::unique_ptr<int> p;

	EmplaceFriendly() = default;
	EmplaceFriendly(int aa, int bb)
		: a {aa}
		, b {bb}
		, p {new int}
	{
	}

	EmplaceFriendly(const EmplaceFriendly& rhs)
		: a {rhs.a}
		, b {rhs.b}
		, p {new int}
	{
		*p = *rhs.p;
	}
	bool operator==(const EmplaceFriendly& o) const { return a == o.a && b == o.b; }
};

TEST_F(RingBufferTest, SupportedOrSkipped)
{
	// Check compile-time presence of emplace_back for a typical instantiation:
	using RBPair = std::ring_buffer<IntPair, 4>;
	if constexpr (!has_emplace_v<RBPair, int, int>)
	{
		GTEST_SKIP() << "RingBuffer::emplace_back(int,int) not available in this build.";
	}
	else
	{
		SUCCEED(); // presence confirmed; other tests exercise behavior
	}
}

TEST_F(RingBufferTest, EmplaceConstructsPairAndOverwrites)
{
	using RBPair = std::ring_buffer<IntPair, 4>;

	if constexpr (!has_emplace_v<RBPair, int, int>)
	{
		GTEST_SKIP() << "emplace_back(int,int) not present; skipping behavior test.";
	}
	else
	{
		RBPair rb;
		rb.emplace_back(1, 2);
		EXPECT_EQ(rb.size(), 1u);
		EXPECT_EQ(rb.front(), IntPair(1, 2));
		EXPECT_EQ(rb.back(), IntPair(1, 2));

		rb.emplace_back(3, 4);
		rb.emplace_back(5, 6);
		rb.emplace_back(7, 8);
		EXPECT_TRUE(rb.full());
		// Overwrite oldest (1,2) with new emplace
		rb.emplace_back(9, 10);
		EXPECT_TRUE(rb.full());
		// Expect contents to be (3,4),(5,6),(7,8),(9,10)
		std::vector<IntPair> got;
		for (auto it = rb.begin(); it != rb.end(); ++it)
		{
			got.push_back(*it);
		}
		const std::vector<IntPair> expected {
			{3, 4 },
			{5, 6 },
			{7, 8 },
			{9, 10}
		   };
		EXPECT_EQ(got, expected);
	}
}

TEST_F(RingBufferTest, EmplaceWithUserType)
{
	using RB = std::ring_buffer<EmplaceFriendly, 3>;
	static_assert(!std::is_trivially_destructible_v<EmplaceFriendly>); // need to test that it's not trivially destructable
	if constexpr (!has_emplace_v<RB, int, int>)
	{
		GTEST_SKIP() << "emplace_back for EmplaceFriendly not present; skipping.";
	}
	else
	{
		RB rb;
		rb.emplace_back(11, 22);
		rb.emplace_back(33, 44);
		EXPECT_EQ(rb.size(), 2u);
		EXPECT_EQ(rb.front(), EmplaceFriendly(11, 22));
		EXPECT_EQ(rb.back(), EmplaceFriendly(33, 44));

		// Fill and overwrite
		rb.emplace_back(55, 66); // now full
		rb.emplace_back(77, 88); // overwrites oldest (11,22)
		std::vector<EmplaceFriendly> got;
		for (auto it = rb.begin(); it != rb.end(); ++it)
		{
			got.push_back(*it);
		}
		const std::vector<EmplaceFriendly> expect {
			{33, 44},
			{55, 66},
			{77, 88}
		};
		EXPECT_EQ(got, expect);
	}
}

} // namespace uqm