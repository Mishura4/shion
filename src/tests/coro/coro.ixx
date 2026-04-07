module;

export module shion.tests:coro;

import :suite;

namespace shion::tests
{
	
bool state_machine_generator(test& t);
bool state_machine_coroutine(test& t);
bool state_machine_continuation(test& t);

bool enumerator_finite(test& t);
bool enumerator_infinite(test& t);
bool enumerator_exceptions(test& t);
bool enumerator_destructions(test& t);
bool enumerator_references(test& t);

bool event_hook_simple(test& t) { t.skip(); return true; }
bool event_hook_order(test& t) { t.skip(); return true; }
bool event_hook_stress_push(test& t) { t.skip(); return true; }

}
