#ifndef SHION_META_MACROS_H_
#define SHION_META_MACROS_H_

#if defined(_MSC_VER)
#	define SHION_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
# 	define SHION_INTRINSIC [[msvc::intrinsic]]
#elif defined(__GNUC__)
#	define SHION_NO_UNIQUE_ADDRESS [[no_unique_address]]
# 	define SHION_INTRINSIC
#else
#	define SHION_NO_UNIQUE_ADDRESS
# 	define SHION_INTRINSIC
#endif

#ifdef __cpp_static_call_operator
#	define SHION_STATIC_OPERATOR_CALL static
#	define SHION_CONST_OPERATOR_CALL
#else
#	define SHION_STATIC_OPERATOR_CALL
#	define SHION_CONST_OPERATOR_CALL const
#endif

#endif /* SHION_META_MACROS_H_ */

