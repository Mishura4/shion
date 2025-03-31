#ifndef SHION_TESTS_H_
#define SHION_TESTS_H_

#define TEST_ASSERT(test, a) if (!(a)) do { \
		if (!std::is_constant_evaluated()) (test).fail(#a); \
		return false; \
	} while(false) \

#endif
