#ifndef ENTRYEXIT_H
#define ENTRYEXIT_H

#include "TransportDebug.h"
#include  "ace/Log_Msg.h"
#include  "ace/OS_NS_unistd.h"

#define DBG_ENTRY(CNAME,MNAME)
#define DBG_SUB_ENTRY(CNAME,MNAME,INUM)
#define DBG_ENTRY_LVL(CNAME,MNAME,DBG_LVL)

#if DDS_BLD_DEBUG_LEVEL > 0

#undef DBG_ENTRY
#undef DBG_SUB_ENTRY
#undef DBG_ENTRY_LVL

// deprecated
#define DBG_ENTRY(CNAME,MNAME) \
EntryExit dbg_0(CNAME,MNAME)

#define DBG_SUB_ENTRY(CNAME,MNAME,INUM) \
EntryExit dbg_##INUM (CNAME,MNAME,INUM)

#define DBG_ENTRY0(CNAME,MNAME)
#define DBG_ENTRY1(CNAME,MNAME)
#define DBG_ENTRY2(CNAME,MNAME)
#define DBG_ENTRY3(CNAME,MNAME)
#define DBG_ENTRY4(CNAME,MNAME)
#define DBG_ENTRY5(CNAME,MNAME)

#if DDS_BLD_DEBUG_LEVEL >=1
#undef DBG_ENTRY1
#define DBG_ENTRY1(CNAME,MNAME) \
DBG_ENTRY(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=2
#undef DBG_ENTRY2
#define DBG_ENTRY2(CNAME,MNAME) \
DBG_ENTRY(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=3
#undef DBG_ENTRY3
#define DBG_ENTRY3(CNAME,MNAME) \
DBG_ENTRY(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=4
#undef DBG_ENTRY4
#define DBG_ENTRY4(CNAME,MNAME) \
DBG_ENTRY(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=5
#undef DBG_ENTRY5
#define DBG_ENTRY5(CNAME,MNAME) \
DBG_ENTRY(CNAME,MNAME)
#endif

#define DBG_ENTRY_LVL(CNAME,MNAME,DBG_LVL) \
DBG_ENTRY##DBG_LVL(CNAME,MNAME)

#endif // #if DDS_BLD_DEBUG_LEVEL > 0

class EntryExit
{
  public:

    EntryExit(const char* className, const char* methodName, unsigned num = 0)
      : num_ (num)
    {
      // No processing unless debugging turned on.
      if (::TAO::DCPS::Transport_debug_level == 1)
  {
    class_[25] = method_[25] = 0;

    ACE_OS::strncpy (this->class_, className, 25);
    ACE_OS::strncpy (this->method_, methodName, 25);

    if (this->num_ == 0) {
      ACE_DEBUG ((LM_DEBUG, "(%P|%t) DBG: ENTRY: [%s::%s()]\n"
      , this->class_, this->method_));
      //VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: ENTRY: [%s::%s()]\n"
      //, this->class_, this->method_), 1);
    }
    else {
      VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: ENTRY: [%s::%s():%d]\n"
          , this->class_, this->method_, this->num_), 1);
    }
  }
    };

    ~EntryExit()
    {
       if (::TAO::DCPS::Transport_debug_level == 1)
  {
    if (this->num_ == 0) {
      ACE_DEBUG ((LM_DEBUG, "(%P|%t) DBG: EXIT : [%s::%s()]\n"
      , this->class_, this->method_));
      //VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: EXIT : [%s::%s()]\n"
      //, this->class_, this->method_), 1);
    }
    else {
      VDBG_LVL((LM_DEBUG, "(%P|%t) DBG: EXIT : [%s::%s():%d]\n"
          , this->class_, this->method_, this->num_), 1);
    }
  }
    };

  private:

    char class_[26];
    char method_[26];
    unsigned    num_;
};

#endif  /* ENTRYEXIT_H */
