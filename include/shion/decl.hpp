#ifndef SHION_DECL_H_
#define SHION_DECL_H_

#ifndef SHION_BUILDING_LIBRARY
#  define SHION_DECL
#  define SHION_DECL_INLINE inline
#else
#  ifdef SHION_BUILDING_MODULES
#    define SHION_DECL export
#    define SHION_DECL_INLINE export inline
#  else
#    define SHION_DECL
#    define SHION_DECL_INLINE inline
#  endif
#endif

#endif /* SHION_DECL_H_ */
