#include <string>

#include <gtest/gtest.h>

#include <mgrech/ovector.hpp>

using mgrech::ovector;

namespace global
{
	int dtor_count = 0;
}

struct dtor_counted
{
	~dtor_counted()
	{
		++global::dtor_count;
	}
};

TEST(ovector, default_ctor)
{
	ovector<char> v;
	ASSERT_EQ(v.data(), nullptr);
	ASSERT_EQ(v.max_size(), 0);
	ASSERT_EQ(v.size(), 0);
}

TEST(ovector, with_max_size_or_null)
{
	auto v = ovector<float>::with_max_size_or_null(1234);
	ASSERT_NE(v.data(), nullptr);
	ASSERT_EQ(v.max_size(), 1234);
	ASSERT_EQ(v.size(), 0);
}

TEST(ovector, no_overflow_in_allocation)
{
	auto v = ovector<char>::with_max_size_or_null(~0ull);
	ASSERT_EQ(v.data(), nullptr);
	ASSERT_EQ(v.max_size(), 0);
	ASSERT_EQ(v.size(), 0);
}

TEST(ovector, push_back)
{
	auto v = ovector<int>::with_max_size_or_null(2);
	v.push_back(1);

	int i = 2;
	v.push_back(i);

	ASSERT_EQ(v.size(), 2);
	ASSERT_EQ(v[0], 1);
	ASSERT_EQ(v[1], 2);
}

TEST(ovector, emplace_back)
{
	auto v = ovector<std::string>::with_max_size_or_null(2);
	v.emplace_back("foo");

	std::string barbar = "barbar";
	v.emplace_back(barbar, 3, 3);

	ASSERT_EQ(v.size(), 2);
	ASSERT_EQ(v[0], "foo");
	ASSERT_EQ(v[1], "bar");
}

TEST(ovector, pop_back)
{
	auto v = ovector<dtor_counted>::with_max_size_or_null(1);
	v.emplace_back();

	global::dtor_count = 0;
	v.pop_back();

	ASSERT_EQ(v.max_size(), 1);
	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(global::dtor_count, 1);
}

TEST(ovector, clear_nontrivial)
{
	auto v = ovector<dtor_counted>::with_max_size_or_null(3);
	v.emplace_back();
	v.emplace_back();
	v.emplace_back();

	global::dtor_count = 0;
	v.clear();

	ASSERT_NE(v.data(), nullptr);
	ASSERT_EQ(v.max_size(), 3);
	ASSERT_EQ(v.size(), 0);
	ASSERT_EQ(global::dtor_count, 3);
}

TEST(ovector, clear_trivial)
{
	auto v = ovector<int>::with_max_size_or_null(1234);
	v.push_back(123);
	v.push_back(234);
	v.push_back(345);

	v.clear();

	ASSERT_NE(v.data(), nullptr);
	ASSERT_EQ(v.max_size(), 1234);
	ASSERT_EQ(v.size(), 0);
}

TEST(ovector, swap)
{
	auto v1 = ovector<int>::with_max_size_or_null(123);
	auto v2 = ovector<int>();
	v1.push_back(1234);

	swap(v1, v2);

	ASSERT_EQ(v1.max_size(), 0);
	ASSERT_EQ(v2.max_size(), 123);
	ASSERT_EQ(v1.size(), 0);
	ASSERT_EQ(v2.size(), 1);
	ASSERT_EQ(v2[0], 1234);
}

TEST(ovector, op_eq_size_equal)
{
	auto v1 = ovector<int>::with_max_size_or_null(1234);
	auto v2 = ovector<int>::with_max_size_or_null(2345);
	v1.push_back(123);
	v2.push_back(123);

	ASSERT_EQ(v1, v2);
}

TEST(ovector, op_eq_size_not_equal)
{
	auto v1 = ovector<int>::with_max_size_or_null(1234);
	auto v2 = ovector<int>::with_max_size_or_null(2345);
	v1.push_back(123);

	ASSERT_NE(v1, v2);
}

TEST(ovector, guard_page_set_up_correctly)
{
	auto v = ovector<char>::with_max_size_or_null(1);
	v.push_back('a');

	ASSERT_DEATH(v.push_back('b'), "");
}
