#define TEST_CHECK(COND) \
  ++assertions; \
  if (!( COND )) \
      ACE_ERROR((LM_ERROR,"(%P|%t) TEST_CHECK(%C) FAILED at %N:%l %a\n",\
        #COND , -1));

