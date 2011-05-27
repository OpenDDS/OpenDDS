#ifndef ENTRYEXIT_H
#define ENTRYEXIT_H

#include "TransportDebug.h"
#include  "ace/Log_Msg.h"
#include  <string>


#ifndef DCPS_TRANS_VERBOSE_DEBUG

#define DBG_ENTRY(CNAME,MNAME)
#define DBG_SUB_ENTRY(CNAME,MNAME,INUM)

#else

#define DBG_ENTRY(CNAME,MNAME) \
EntryExit dbg_0(CNAME,MNAME)

#define DBG_SUB_ENTRY(CNAME,MNAME,INUM) \
EntryExit dbg_##INUM (CNAME,MNAME,INUM)

class EntryExit
{
  public:

    EntryExit(const char* className, const char* methodName, unsigned num = 0)
      : class_(className), method_(methodName), num_(num)
    {
      if (this->num_ == 0)
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) DBG: ENTRY: [%s::%s()]\n",
                     this->class_.c_str(), this->method_.c_str()));
        }
      else
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) DBG: ENTRY: [%s::%s():%d]\n",
                     this->class_.c_str(), this->method_.c_str(), this->num_));
        }
    }

    ~EntryExit()
    {
      if (this->num_ == 0)
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) DBG: EXIT : [%s::%s()]\n",
                     this->class_.c_str(), this->method_.c_str()));
        }
      else
        {
          VDBG((LM_DEBUG,
                     "(%P|%t) DBG: EXIT : [%s::%s():%d]\n",
                     this->class_.c_str(), this->method_.c_str(), this->num_));
        }
    }


  private:

    std::string class_;
    std::string method_;
    unsigned    num_;
};

#endif  /* VERBOSE_DEBUG */

#endif  /* ENTRYEXIT_H */
