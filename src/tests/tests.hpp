#ifndef SHION_TESTS_H_
#define SHION_TESTS_H_

#define TEST_ASSERT(test, a) if (!(a)) { (test).fail(#a); return; }

#endif
