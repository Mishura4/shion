module;

export module shion.tests:coro;

import :suite;

namespace shion::tests
{
	
bool state_machine_generator(test& t);
bool state_machine_coroutine(test& t);
bool state_machine_continuation(test& t);

}
