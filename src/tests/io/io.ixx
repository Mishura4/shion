module;

export module shion.tests:io;

import :suite;

namespace shion::tests
{
	
bool serializer_helper_fundamental(test& t);
bool serializer_helper_tuples(test& t);
bool serializer_helper_contiguous_ranges(test& t);
bool serializer_helper_list_ranges(test& t);

}
