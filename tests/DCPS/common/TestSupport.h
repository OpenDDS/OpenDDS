#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

// if COND fails then log error and abort with -1.
#define TEST_CHECK(COND) \
  if (!( COND )) \
      ACE_ERROR((LM_ERROR,"(%N:%l) FAILED on TEST_CHECK(%C)%a\n",\
        #COND , -1));

// if COND fails then log error and throw.
#define TEST_ASSERT(COND) \
  if (!( COND )) \
      do { ACE_ERROR((LM_ERROR,"(%N:%l) FAILED on TEST_CHECK(%C).\n",\
        #COND)); \
        throw #COND; } \
      while (0);

#endif /* TEST_SUPPORT_H */
