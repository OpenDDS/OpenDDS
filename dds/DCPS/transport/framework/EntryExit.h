/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ENTRYEXIT_H
#define ENTRYEXIT_H

#include "TransportDebug.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_string.h"

#define DBG_ENTRY_CORE(CNAME,MNAME)   \
  EntryExit ee_dbg(CNAME,MNAME,this)

#define DBG_ENTRY0(CNAME,MNAME)
#define DBG_ENTRY1(CNAME,MNAME)
#define DBG_ENTRY2(CNAME,MNAME)
#define DBG_ENTRY3(CNAME,MNAME)
#define DBG_ENTRY4(CNAME,MNAME)
#define DBG_ENTRY5(CNAME,MNAME)
#define DBG_ENTRY6(CNAME,MNAME)

#if DDS_BLD_DEBUG_LEVEL >=0
#undef DBG_ENTRY0
#define DBG_ENTRY0(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=1
#undef DBG_ENTRY1
#define DBG_ENTRY1(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=2
#undef DBG_ENTRY2
#define DBG_ENTRY2(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=3
#undef DBG_ENTRY3
#define DBG_ENTRY3(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=4
#undef DBG_ENTRY4
#define DBG_ENTRY4(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=5
#undef DBG_ENTRY5
#define DBG_ENTRY5(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#if DDS_BLD_DEBUG_LEVEL >=6
#undef DBG_ENTRY6
#define DBG_ENTRY6(CNAME,MNAME)     \
  DBG_ENTRY_CORE(CNAME,MNAME)
#endif

#define DBG_ENTRY_LVL(CNAME,MNAME,DBG_LVL)  \
  DBG_ENTRY##DBG_LVL(CNAME,MNAME)

// deprecated
#define DBG_ENTRY(CNAME,MNAME)      \
  DBG_ENTRY_LVL(CNAME,MNAME,6)

class EntryExit {
public:
  EntryExit(const char* className, const char* methodName, const void* addr
            , unsigned recurse_level = 0)
  : recurse_level_(recurse_level)
  , addr_(addr)
  , addr_set_(true) {

    ACE_UNUSED_ARG(addr_set_);

    // No processing unless debugging turned on.
    if (OpenDDS::DCPS::Transport_debug_level > 0) {
      class_[25] = method_[25] = 0;

      ACE_OS::strncpy(this->class_, className, 25);
      ACE_OS::strncpy(this->method_, methodName, 25);

      if (this->recurse_level_ == 0) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG: ENTRY: [%s::%s() ::%@]\n"
                   , this->class_, this->method_, this->addr_));

      } else {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG: ENTRY: [%s::%s() ::%@ :%d]\n"
                   , this->class_, this->method_, this->addr_
                   , this->recurse_level_));
      }
    }
  };

  ~EntryExit() {
    if (OpenDDS::DCPS::Transport_debug_level > 0) {
      if (this->recurse_level_ == 0) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG: EXIT : [%s::%s() ::%@]\n"
                   , this->class_, this->method_, this->addr_));

      } else {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) DBG: EXIT : [%s::%s() ::%@:%d]\n"
                   , this->class_, this->method_, this->addr_
                   , this->recurse_level_));
      }
    }
  };

private:

  char class_[26];
  char method_[26];
  unsigned    recurse_level_;
  const void *addr_;
  bool addr_set_;
};

#endif  /* ENTRYEXIT_H */
