#include <mgrech/ovector.hpp>

using mgrech::ovector;

int main()
{
	// hover over all of these expressions to verify that the documentation displays properly

	ovector<int> v = ovector<int>::with_max_size_or_null(123);
	ovector<int> const cv = ovector<int>::with_max_size_or_null(456);

	(void)v.data();
	(void)v.empty();
	(void)v.size();
	(void)v.begin();
	(void)cv.begin();
	(void)v.end();
	(void)cv.end();
	(void)v.cbegin();
	(void)cv.cbegin();
	(void)v.cend();
	(void)cv.cend();
	(void)v.front();
	(void)cv.front();
	(void)v.back();
	(void)cv.back();
	(void)v[0];
	(void)cv[0];

	v.clear();
	v.push_back(1);
	int i = 2; v.push_back(i);
	v.pop_back();
	v.emplace_back(3);
	v.uninitialized_grow_back_by(1234);
	v.uninitialized_shrink_back_by(5678);
}
