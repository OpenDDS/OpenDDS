/*
 *
 * Derived from sample code included in Microsoft Knowledge Base Article 327215.
 * Additional material distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RTDServerDLL_H
#define RTDServerDLL_H

#include "comdef.h"
#include "initguid.h"

class RTDServerClassFactory : public IClassFactory
{
protected:
  ULONG m_refCount; //reference count

public:
  RTDServerClassFactory();
  ~RTDServerClassFactory();

  /******* IUnknown Methods *******/
  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  /******* IClassFactory Methods *******/
  STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
  STDMETHODIMP LockServer(BOOL);
};

LONG g_cLock = 0; //global count of the locks on this DLL
typedef RTDServerClassFactory FAR *LPRTDServerClassFactory;

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR * ppvObj);
STDAPI DllCanUnloadNow();

#endif /*RTDServerDLL_H*/
