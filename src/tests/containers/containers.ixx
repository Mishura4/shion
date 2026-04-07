module;

export module shion.tests:containers;

import :suite;

namespace shion::tests
{

bool hive_test_nontrivial(test& self);
bool hive_test_trivial(test& self);

bool concurrent_flush_simple(test& self);
bool concurrent_flush_order(test& self);
bool concurrent_flush_state_machine(test& self);

}
