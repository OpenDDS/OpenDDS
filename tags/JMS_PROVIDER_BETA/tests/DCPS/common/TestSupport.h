#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

// if COND fails then log error and abort with -1.
#define TEST_CHECK(COND) \
  if (!( COND )) \
      ACE_ERROR((LM_ERROR,"(%N:%l) FAILED on TEST_CHECK(%s)%a\n",\
        #COND , -1));

#endif /* TEST_SUPPORT_H */
