#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

// if COND fails then log error and abort with -1.
#define TEST_CHECK(COND) \
  if (!( COND )) \
      ACE_ERROR((LM_ERROR,"(%P|%t) TEST_ASSERT(%C) FAILED at %N:%l%a\n",\
        #COND , -1));

// if COND fails then log error and throw.
#define TEST_ASSERT(COND) \
  if (!( COND )) \
      do { ACE_ERROR((LM_ERROR,"(%P|%t) TEST_ASSERT(%C) FAILED at %N:%l\n",\
        #COND)); \
        throw #COND; } \
      while (0);

#endif /* TEST_SUPPORT_H */
