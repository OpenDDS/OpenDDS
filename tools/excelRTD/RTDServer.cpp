/*
 *
 * Derived from sample code included in Microsoft Knowledge Base Article 327215.
 * Additional material distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "RTDServer.h"
#include "RTDMonitor.h"

#include <sstream>

LONG g_cOb = 0; //global count of the number of objects created.

RTDServer::RTDServer(IUnknown* pUnkOuter)
{
  m_refCount = 0;
  m_pTypeInfoInterface = NULL;
  m_dwDataThread = -1;

  //Get the TypeInfo for this object
  LoadTypeInfo(&m_pTypeInfoInterface, IID_IRtdServer, 0x0);

  //Set up the aggregation
  if (pUnkOuter != NULL)
    m_pOuterUnknown = pUnkOuter;
  else
    m_pOuterUnknown = reinterpret_cast<IUnknown*>
    (static_cast<INonDelegatingUnknown*>(this));

  //Increment the object count so the server knows not to unload
  InterlockedIncrement( &g_cOb );
}

RTDServer::~RTDServer()
{
  //Clean up the type information
  if (m_pTypeInfoInterface != NULL){
    m_pTypeInfoInterface->Release();
    m_pTypeInfoInterface = NULL;
  }

  //Make sure we kill the data thread
  if (m_dwDataThread != -1){
    PostThreadMessage( m_dwDataThread, WM_COMMAND, WM_SILENTTERMINATE, 0 );
  }

  //Decrement the object count
  InterlockedDecrement( &g_cOb );

  for (Monitors::iterator iMonitor = this->monitors.begin(); iMonitor != this->monitors.end(); ++iMonitor) {
    delete iMonitor->second;
  }

  Monitor::RTDMonitor::shutdown();
}

/******************************************************************************
*   LoadTypeInfo -- Gets the type information of an object's interface from the
*   type library.  Returns S_OK if successful.
******************************************************************************/
STDMETHODIMP RTDServer::LoadTypeInfo(ITypeInfo** pptinfo, REFCLSID clsid,
                                        LCID lcid)
{
  HRESULT hr;
  LPTYPELIB ptlib = NULL;
  LPTYPEINFO ptinfo = NULL;
  *pptinfo = NULL;

  // First try to load the type info from a registered type library
  hr = LoadRegTypeLib(LIBID_RTDServerLib, 1, 0, lcid, &ptlib);
  if (FAILED(hr)) {
    //can't get the type information
    return hr;
  }

  // Get type information for interface of the object.
  hr = ptlib->GetTypeInfoOfGuid(clsid, &ptinfo);
  if (FAILED(hr))
  {
    ptlib->Release();
    return hr;
  }
  ptlib->Release();
  *pptinfo = ptinfo;
  return S_OK;
}

/******************************************************************************
*   IUnknown Interfaces -- All COM objects must implement, either
*  directly or indirectly, the IUnknown interface.
******************************************************************************/

/******************************************************************************
*  QueryInterface -- Determines if this component supports the
*  requested interface, places a pointer to that interface in ppvObj if it's
*  available, and returns S_OK.  If not, sets ppvObj to NULL and returns
*  E_NOINTERFACE.
******************************************************************************/
STDMETHODIMP RTDServer::QueryInterface(REFIID riid, void ** ppvObj)
{
  //defer to the outer unknown
  return m_pOuterUnknown->QueryInterface( riid, ppvObj );
}

/******************************************************************************
*  AddRef() -- In order to allow an object to delete itself when
*  it is no longer needed, it is necessary to maintain a count of all
*  references to this object.  When a new reference is created, this function
*  increments the count.
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServer::AddRef()
{
  //defer to the outer unknown
  return m_pOuterUnknown->AddRef();
}

/******************************************************************************
*  Release() -- When a reference to this object is removed, this
*  function decrements the reference count.  If the reference count is 0, then
*  this function deletes this object and returns 0.
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServer::Release()
{
  //defer to the outer unknown
  return m_pOuterUnknown->Release();
}

/******************************************************************************
*   INonDelegatingUnknown Interfaces -- All COM objects must implement, either
*  directly or indirectly, the IUnknown interface.
******************************************************************************/

/******************************************************************************
*  NonDelegatingQueryInterface -- Determines if this component supports the
*  requested interface, places a pointer to that interface in ppvObj if it's
*  available, and returns S_OK.  If not, sets ppvObj to NULL and returns
*  E_NOINTERFACE.
******************************************************************************/
STDMETHODIMP RTDServer::NonDelegatingQueryInterface(REFIID riid,
                                                     void ** ppvObj)
{
  if (riid == IID_IUnknown){
    *ppvObj = static_cast<INonDelegatingUnknown*>(this);
  }

  else if (riid == IID_IDispatch){
    *ppvObj = static_cast<IDispatch*>(this);
  }

  else if (riid == IID_IRtdServer){
    *ppvObj = static_cast<IRtdServer*>(this);
  }

  else{
    // Unsupported Interface
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }

  static_cast<IUnknown*>(*ppvObj)->AddRef();
  return S_OK;
}

/******************************************************************************
*  NonDelegatingAddRef() -- In order to allow an object to delete itself when
*  it is no longer needed, it is necessary to maintain a count of all
*  references to this object.  When a new reference is created, this function
*  increments the count.
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServer::NonDelegatingAddRef()
{
  return ++m_refCount;
}

/******************************************************************************
*  NonDelegatingRelease() -- When a reference to this object is removed, this
*  function decrements the reference count.  If the reference count is 0, then
*  this function deletes this object and returns 0.
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServer::NonDelegatingRelease()
{
  --m_refCount;

  if (m_refCount == 0)
  {
   delete this;
   return 0;
  }
  return m_refCount;
}

/******************************************************************************
*   IDispatch Interface -- This interface allows this class to be used as an
*   automation server, allowing its functions to be called by other COM
*   objects
******************************************************************************/

/******************************************************************************
*   GetTypeInfoCount -- This function determines if the class supports type
*   information interfaces or not.  It places 1 in iTInfo if the class supports
*   type information and 0 if it doesn't.
******************************************************************************/
STDMETHODIMP RTDServer::GetTypeInfoCount(UINT *iTInfo)
{
  *iTInfo = 0;
  return S_OK;
}

/******************************************************************************
*   GetTypeInfo -- Returns the type information for the class.  For classes
*   that don't support type information, this function returns E_NOTIMPL;
******************************************************************************/
STDMETHODIMP RTDServer::GetTypeInfo(UINT iTInfo, LCID lcid,
                                       ITypeInfo **ppTInfo)
{
  return E_NOTIMPL;
}

/******************************************************************************
*   GetIDsOfNames -- Takes an array of strings and returns an array of DISPID's
*   which corespond to the methods or properties indicated.  If the name is not
*   recognized, returns DISP_E_UNKNOWNNAME.
******************************************************************************/
STDMETHODIMP RTDServer::GetIDsOfNames(REFIID riid,
                                         OLECHAR **rgszNames,
                                         UINT cNames,  LCID lcid,
                                         DISPID *rgDispId)
{
  HRESULT hr = E_FAIL;

  //Validate arguments
  if (riid != IID_NULL)
    return E_INVALIDARG;

  //this API call gets the DISPID's from the type information
  if (m_pTypeInfoInterface != NULL)
    hr = m_pTypeInfoInterface->GetIDsOfNames(rgszNames, cNames, rgDispId);

  //DispGetIDsOfNames may have failed, so pass back its return value.
  return hr;
}

/******************************************************************************
*   Invoke -- Takes a dispid and uses it to call another of this class's
*   methods.  Returns S_OK if the call was successful.
******************************************************************************/
STDMETHODIMP RTDServer::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pDispParams,
                                  VARIANT* pVarResult, EXCEPINFO* pExcepInfo,
                                  UINT* puArgErr)
{
  HRESULT hr = DISP_E_PARAMNOTFOUND;

  //Validate arguments
  if ((riid != IID_NULL))
    return E_INVALIDARG;

  hr = m_pTypeInfoInterface->Invoke((IRtdServer*)this, dispIdMember, wFlags,
    pDispParams, pVarResult, pExcepInfo, puArgErr);

  return S_OK;
}

/******************************************************************************
*  ServerStart -- The ServerStart method is called immediately after a
*  real-time data server is instantiated.
*  Parameters: CallbackObject -- interface pointer the RTDServer uses to
*                                indicate new data is available.
*              pfRes -- set to positive value to indicate success.  0 or
*                       negative value indicates failure.
*  Returns: S_OK
*           E_POINTER
*           E_FAIL
******************************************************************************/
STDMETHODIMP RTDServer::ServerStart(IRTDUpdateEvent *CallbackObject,
                                    long *pfRes)
{
  HRESULT hr = S_OK;

  //Check the arguments first
  if ((CallbackObject == NULL) || (pfRes == NULL))
    hr = E_POINTER;

  //if the data thread has already been launched, return an error
  else if (m_dwDataThread != -1){
    hr = E_FAIL;
    *pfRes = -1;
  }

  //Try to launch the data thread
  else{
    //Marshal the interface to the new thread
    IStream* pMarshalStream = NULL;
    hr = CoMarshalInterThreadInterfaceInStream( IID_IRTDUpdateEvent,
       CallbackObject, &pMarshalStream );

    CreateThread( NULL, 0, RealTimeDataThread, (void*)pMarshalStream, 0,
       &m_dwDataThread );
    *pfRes = m_dwDataThread;
  }

  return hr;
}

namespace {
  const char* MONITOR_HEARTBEAT("MonitorHeartbeat");
  const char* MONITOR_NEW_MONITOR("MonitorNewMonitor");
  const char* MONITOR_REMOVE_MONITOR("MonitorRemoveMonitor");
  const char* MONITOR_ATTACH_INFOREPO("MonitorAttachRepo");
  const char* MONITOR_DETACH_INFOREPO("MonitorDetachRepo");
  const char* MONITOR_COLUMN_COUNT("MonitorColumnCount");
  const char* MONITOR_IS_NODE_VALID("MonitorIsNodeValid");
  const char* MONITOR_NODE_VALUE("MonitorNodeValue");
  const char* MONITOR_NODE_COLOR("MonitorNodeColor");
  const char* MONITOR_NODE_CHILDREN("MonitorNodeChildren");
}

namespace {
  std::string convertToString(const VARIANT& value) {
    std::stringstream os;
    os << _bstr_t(value.bstrVal, true);
    return os.str();
  }

  std::string getSafeArrayElementAsStr(SAFEARRAY **Strings, long idx) {
    VARIANT value;

    SafeArrayGetElement(*Strings, &idx, &value);
    return convertToString(value);
  }

  typedef std::list<std::string> StringList;

  std::string convertToDelimitedString(const StringList& list) {
    std::stringstream os;

    os << "{";

    for (StringList::const_iterator iList = list.begin(); iList != list.end();
         ++iList) {
      if (iList != list.begin()) {
        os << ", ";
      }

      os << "\"" << *iList << "\"";
    }
    os << "}";

    return os.str();
  }
}

/******************************************************************************
*  ConnectData -- Adds new topics from a real-time data server. The ConnectData
*  method is called when a file is opened that contains real-time data
*  functions or when a user types in a new formula which contains the RTD
*  function.
*  Parameters: TopicID -- value assigned by Excel to identify the topic
*              Strings -- safe array containing the strings identifying the
*                         data to be served.
*              GetNewValues -- BOOLEAN indicating whether to retrieve new
*                              values or not.
*              pvarOut -- initial value of the topic
*  Returns: S_OK
*           E_POINTER
*           E_FAIL
******************************************************************************/
STDMETHODIMP RTDServer::ConnectData(long TopicID,
                                    SAFEARRAY **Strings,
                                    VARIANT_BOOL *GetNewValues,
                                    VARIANT *pvarOut)
{
  //Check the arguments first
  if (pvarOut == NULL) return E_POINTER;

  HRESULT hr = S_OK;

  std::stringstream topicValue;
  bool addTopic = true;

  TopicData topicData;
  topicData.name = getSafeArrayElementAsStr(Strings, 0);

  if (topicData.name != MONITOR_HEARTBEAT) {
    topicData.monitorId = getSafeArrayElementAsStr(Strings, 1);

    Monitor::RTDMonitor* monitor = getMonitor(topicData);

    if (topicData.name == MONITOR_NEW_MONITOR) {
      if (monitor != NULL) {
        monitor->newRepo("");

        delete monitor;
        this->monitors.erase(topicData.monitorId);
      }

      monitor = new Monitor::RTDMonitor;
      this->monitors.insert(Monitors::value_type(topicData.monitorId, monitor));

      addTopic = false;

      topicValue << "True";
    } else if (topicData.name == MONITOR_REMOVE_MONITOR) {
      if (monitor != NULL) {
        monitor->newRepo("");

        delete monitor;
        this->monitors.erase(topicData.monitorId);
      }
      addTopic = false;
      topicValue << "True";
    } else if (topicData.name == MONITOR_ATTACH_INFOREPO) {
      if (monitor != NULL) {
        std::string ior = getSafeArrayElementAsStr(Strings, 2);
        bool success = monitor->newRepo(ior);

        topicValue << (success ? "True" : "False");
      } else {
        topicValue << "False";
      }
      addTopic = false;
    } else if (topicData.name == MONITOR_DETACH_INFOREPO) {
      if (monitor != NULL) {
        bool success = monitor->newRepo("");

        topicValue << (success ? "True" : "False");
      } else {
        topicValue << "True";
      }

      addTopic = false;
    } else if (topicData.name == MONITOR_COLUMN_COUNT) {
      topicValue << (monitor ? monitor->getColumnCount() : 0);

    } else if (topicData.name == MONITOR_IS_NODE_VALID) {
      std::string id = getSafeArrayElementAsStr(Strings, 2);
      topicData.parameters.push_back(id);

      bool isNodeValid = (monitor ? monitor->isNodeValid(id) : false);
      topicValue << (isNodeValid ? "True" : "False");

    } else if (topicData.name == MONITOR_NODE_VALUE) {
      std::string id = getSafeArrayElementAsStr(Strings, 2);
      topicData.parameters.push_back(id);

      std::string indexStr = getSafeArrayElementAsStr(Strings, 3);
      int index = atoi(indexStr.c_str());
      topicData.parameters.push_back(indexStr);

      if (monitor == NULL) {
        topicValue << "#Error: unable to find monitor with id: " << topicData.monitorId;
      } else {
        topicValue << monitor->getNodeValue(id, index);
      }

    } else if (topicData.name == MONITOR_NODE_COLOR) {
      std::string id = getSafeArrayElementAsStr(Strings, 2);
      topicData.parameters.push_back(id);

      std::string indexStr = getSafeArrayElementAsStr(Strings, 3);
      int index = atoi(indexStr.c_str());
      topicData.parameters.push_back(indexStr);

      if (monitor != NULL) {
        topicValue << monitor->getNodeColor(id, index);
      }
    } else if (topicData.name == MONITOR_NODE_CHILDREN) {
      std::string parentId = getSafeArrayElementAsStr(Strings, 2);

      topicData.parameters.push_back(parentId);

      if (monitor == NULL) {
        topicValue << "{}";
      } else {
        topicValue << convertToDelimitedString(monitor->getNodeChildren(parentId));
      }

    } else {
      topicValue << "<unsupported request>";

      hr = E_FAIL;
    }
  }

  if (addTopic) {
    m_pTopicList.insert(TopicList::value_type(TopicID, topicData));
  }

  VariantInit(pvarOut);
  pvarOut->vt = VT_BSTR;

  std::wstring ws;
  std::string s = topicValue.str();
  ws.assign(s.begin(), s.end());

  pvarOut->bstrVal = SysAllocString(ws.c_str());

  return hr;
}

Monitor::RTDMonitor*
RTDServer::getMonitor(const RTDServer::TopicData& topicData) const {
  Monitors::const_iterator iMonitor = this->monitors.find(topicData.monitorId);

  if (iMonitor == this->monitors.end()) return NULL;
  return iMonitor->second;
}

/******************************************************************************
*  RefreshData -- This method is called by Microsoft Excel to get new data.
*  This method call only takes place after being notified by the real-time
*  data server that there is new data.
*  Parameters: TopicCount -- filled with the count of topics in the safearray
*              parrayOut -- two-dimensional safearray.  First dimension
*                           contains the list of topic IDs.  Second dimension
*                           contains the values of those topics.
*  Returns: S_OK
*           E_POINTER
*           E_FAIL
******************************************************************************/
STDMETHODIMP RTDServer::RefreshData( long *TopicCount,
                                    SAFEARRAY **parrayOut)
{
  //Check the arguments first
  if ((TopicCount == NULL) || (parrayOut == NULL) || (*parrayOut != NULL)){
    // Bad pointer
    return E_POINTER;
  }

  HRESULT hr = S_OK;

  //Set the TopicCount
  *TopicCount = static_cast<long>(m_pTopicList.size());


  SAFEARRAYBOUND bounds[2];

  //Build the safe-array values we want to insert
  bounds[0].cElements = 2;
  bounds[0].lLbound = 0;
  bounds[1].cElements = *TopicCount;
  bounds[1].lLbound = 0;
  *parrayOut = SafeArrayCreate(VT_VARIANT, 2, bounds);

  int i = 0;
  for (TopicList::const_iterator iTopic = m_pTopicList.begin();
       iTopic != m_pTopicList.end(); ++iTopic, ++i) {
    std::stringstream topicValue;
    long index[2];
    index[0] = 0;
    index[1] = i;

    VARIANT value;
    VariantInit(&value);
    value.vt = VT_I4;
    value.lVal = iTopic->first;
    SafeArrayPutElement(*parrayOut, index, &value);

    index[0] = 1;
    index[1] = i;

    const std::string& topicName = iTopic->second.name;
    if (topicName != MONITOR_HEARTBEAT) {
      Monitor::RTDMonitor* monitor = getMonitor(iTopic->second);

      if (topicName == MONITOR_COLUMN_COUNT) {
        topicValue << (monitor ? monitor->getColumnCount() : 0);
      } else if (topicName == MONITOR_IS_NODE_VALID) {
        bool isNodeValid = (monitor ? monitor->isNodeValid(iTopic->second.parameters[0]) : false);
        topicValue << (isNodeValid ? "True" : "False");

      } else if (topicName == MONITOR_NODE_VALUE) {
        int index = atoi(iTopic->second.parameters[1].c_str());

        if (monitor == NULL) {
          topicValue << "#Error: unable to find monitor with id: " << iTopic->second.monitorId;
        } else {
          topicValue << monitor->getNodeValue(iTopic->second.parameters[0], index);
        }
      } else if (topicName == MONITOR_NODE_COLOR) {
        int index = atoi(iTopic->second.parameters[1].c_str());

        if (monitor != NULL) {
          topicValue << monitor->getNodeColor(iTopic->second.parameters[0], index);
        }

      } else if (topicName == MONITOR_NODE_CHILDREN) {
        if (monitor == NULL) {
          topicValue << "{}";
        } else {
          topicValue << convertToDelimitedString(monitor->getNodeChildren(iTopic->second.parameters[0]));
        }

      } else {
        topicValue << "<unsupported request>";

        hr = E_FAIL;
      }
    }

    VariantInit(&value);
    value.vt = VT_BSTR;

    std::wstring ws;
    std::string s = topicValue.str();
    ws.assign(s.begin(), s.end());
    value.bstrVal = SysAllocString(ws.c_str());

    SafeArrayPutElement(*parrayOut, index, &value);
    VariantClear(&value); // Release memory.
  }

  return hr;
}

/******************************************************************************
*  DisconnectData -- Notifies the RTD server application that a topic is no
*  longer in use.
*  Parameters: TopicID -- the topic that is no longer in use.
*  Returns:
******************************************************************************/
STDMETHODIMP RTDServer::DisconnectData( long TopicID)
{
  //Search for the topic id and remove it
  m_pTopicList.erase(TopicID);
  return S_OK;
}

/******************************************************************************
*  Heartbeat -- Determines if the real-time data server is still active.
*  Parameters: pfRes -- filled with zero or negative number to indicate
*                       failure; positive number indicates success.
*  Returns: S_OK
*           E_POINTER
*           E_FAIL
******************************************************************************/
STDMETHODIMP RTDServer::Heartbeat(long *pfRes)
{
  HRESULT hr = S_OK;

  //Let's reply with the ID of the data thread
  if (pfRes == NULL)
    hr = E_POINTER;
  else
    *pfRes = m_dwDataThread;

  return hr;
}

/******************************************************************************
*  ServerTerminate -- Terminates the connection to the real-time data server.
*  Parameters: none
*  Returns: S_OK
*           E_FAIL
******************************************************************************/
STDMETHODIMP RTDServer::ServerTerminate( void)
{
  HRESULT hr = S_OK;

  //Make sure we kill the data thread
  if (m_dwDataThread != -1){
    PostThreadMessage( m_dwDataThread, WM_COMMAND, WM_TERMINATE, 0 );
  }

  return hr;
}
