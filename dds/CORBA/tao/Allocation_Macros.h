#ifndef Allocation_Macros_h
#define Allocation_Macros_h

#include "ace/Malloc_Base.h"

# define ACE_ALLOCATOR_NEW(POINTER,CONSTRUCTOR,RET_VAL) \
  do { void* ptr = ACE_Allocator::instance ()->malloc (sizeof (CONSTRUCTOR)); \
     if (ptr == 0) { POINTER = 0; errno = ENOMEM; return RET_VAL;}        \
     else { POINTER = new (ptr) CONSTRUCTOR; } \
   } while (0)
# define ACE_ALLOCATOR_NEW_RETURN(POINTER,CONSTRUCTOR) \
  do { void* ptr = ACE_Allocator::instance ()->malloc (sizeof (CONSTRUCTOR));; \
     if (ptr == 0) { POINTER = 0; errno = ENOMEM; return;}                \
     else { POINTER = new (POINTER) CONSTRUCTOR; } \
  } while (0)
# define ACE_ALLOCATOR_NEW_NORETURN(POINTER,CONSTRUCTOR) \
  do { void* ptr = ACE_Allocator::instance ()->malloc (sizeof (CONSTRUCTOR));; \
     if (ptr == 0) { POINTER = 0; errno = ENOMEM;}                        \
     else { (POINTER = new (POINTER) CONSTRUCTOR; } \
  } while (0)

#endif /* Allocation_Macros_h */
