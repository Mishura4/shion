module;

export module shion.tests:async;

import :suite;

namespace shion::tests
{

bool async_void(test& t);
bool async_value(test &t);
bool async_reference(test &t);
bool async_thread(test &t);
bool async_await(test &t);
bool async_move(test& t);
bool async_destruction(test& t);

}
