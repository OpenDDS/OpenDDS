/*
 *
 * Derived from sample code included in Microsoft Knowledge Base Article 327215.
 * Additional material distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RTDServer_H
#define RTDServer_H

#include "comdef.h"
#include "RTDDataThread.h"
#include <string>
#include <map>
#include <vector>

extern LONG g_cOb; //global count of the number of objects created.

struct INonDelegatingUnknown
{
   /***** INonDelegatingUnknown Methods *****/
   virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,
      void ** ppvObj) = 0;
   virtual STDMETHODIMP_(ULONG) NonDelegatingAddRef() = 0;
   virtual STDMETHODIMP_(ULONG) NonDelegatingRelease() = 0;
};

namespace Monitor {
  class RTDMonitor;
}

class RTDServer : public INonDelegatingUnknown,
public IRtdServer
{
   /// Data structures to hold topic parameters, topics, and monitors.
   typedef std::vector<std::string> Parameters;
   struct TopicData {
     std::string name;
     std::string monitorId;
     Parameters parameters;
   };
   typedef std::map<long, TopicData> TopicList;
   typedef std::map<std::string, Monitor::RTDMonitor*> Monitors;

private:
   int m_refCount;
   IUnknown* m_pOuterUnknown;
   ITypeInfo* m_pTypeInfoInterface;
   DWORD m_dwDataThread;

   TopicList m_pTopicList;
   Monitors monitors;

   Monitor::RTDMonitor* getMonitor(const RTDServer::TopicData& topicData) const;

public:
   RTDServer(IUnknown* pUnkOuter);
   ~RTDServer();

   STDMETHODIMP LoadTypeInfo(ITypeInfo** pptinfo, REFCLSID clsid,
      LCID lcid);

   /***** IUnknown Methods *****/
   STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
   STDMETHODIMP_(ULONG) AddRef();
   STDMETHODIMP_(ULONG) Release();

   /***** INonDelegatingUnknown Methods *****/
   STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,
      void ** ppvObj);
   STDMETHODIMP_(ULONG) NonDelegatingAddRef();
   STDMETHODIMP_(ULONG) NonDelegatingRelease();

   /***** IDispatch Methods *****/
   STDMETHODIMP GetTypeInfoCount(UINT *iTInfo);
   STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid,
      ITypeInfo **ppTInfo);
   STDMETHODIMP GetIDsOfNames(REFIID riid,
      OLECHAR **rgszNames,
      UINT cNames,  LCID lcid,
      DISPID *rgDispId);
   STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
      WORD wFlags, DISPPARAMS* pDispParams,
      VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
      UINT* puArgErr);

   /***** IRTDServer Methods *****/
   STDMETHODIMP ServerStart(
      IRTDUpdateEvent *CallbackObject,
      long *pfRes);

   STDMETHODIMP ConnectData(
      long TopicID,
      SAFEARRAY * *Strings,
      VARIANT_BOOL *GetNewValues,
      VARIANT *pvarOut);

   STDMETHODIMP RefreshData(
      long *TopicCount,
      SAFEARRAY * *parrayOut);

   STDMETHODIMP DisconnectData(
      long TopicID);

   STDMETHODIMP Heartbeat(
      long *pfRes);

   STDMETHODIMP ServerTerminate( void);
};
#endif /*RTDServer_H*/
