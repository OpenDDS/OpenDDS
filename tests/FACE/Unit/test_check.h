#define TEST_CHECK(COND) \
  ++assertions; \
  if (!( COND )) {\
    ++failed; \
    ACE_DEBUG((LM_ERROR,"TEST_CHECK(%C) FAILED at %N:%l\n",\
        #COND )); \
    return; \
  }

#define TEST_CHECK_RETURN(COND, val) \
  ++assertions; \
  if (!( COND )) {\
    ++failed; \
    ACE_DEBUG((LM_ERROR,"TEST_CHECK(%C) FAILED at %N:%l\n",\
        #COND )); \
    return val; \
  }
