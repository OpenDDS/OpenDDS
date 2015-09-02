/*
 *
 * Derived from sample code included in Microsoft Knowledge Base Article 327215.
 * Additional material distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include <ace/config-lite.h>
#include "RTDServerDLL.h"
#include "RTDServer.h"

namespace {
#ifdef RTD_DLL_NAME
  const ACE_TCHAR* DLL_NAME = ACE_TEXT_CHAR_TO_TCHAR(RTD_DLL_NAME);
#elif _DEBUG
  const ACE_TCHAR* DLL_NAME = ACE_TEXT("OpenDDS_ExcelRTDd.dll");
#else
  const ACE_TCHAR* DLL_NAME = ACE_TEXT("OpenDDS_ExcelRTD.dll");
#endif

  const char* PROG_ID = "OpenDDS.ExcelRTD";
  const char* PROG_ID_CLSID_KEY = "OpenDDS.ExcelRTD\\CLSID";

  // 6361D863-60AA-4582-A730-6C719FC324B7
  const CLSID CLSID_RTDServer = {0x6361D863,0x60AA,0x4582,{0xA7,0x30,0x6C,0x71,
  0x9F,0xC3,0x24,0xB7}};
}

RTDServerClassFactory::RTDServerClassFactory()
{
   m_refCount = 0;
}

RTDServerClassFactory::~RTDServerClassFactory()
{}

/******************************************************************************
*   IUnknown Interfaces -- All COM objects must implement, either directly or
*   indirectly, the IUnknown interface.
******************************************************************************/

/******************************************************************************
*   QueryInterface -- Determines if this component supports the requested
*   interface, places a pointer to that interface in ppvObj if it's available,
*   and returns S_OK.  If not, sets ppvObj to NULL and returns E_NOINTERFACE.
******************************************************************************/
STDMETHODIMP RTDServerClassFactory::QueryInterface(REFIID riid, void ** ppvObj)
{
   if (riid == IID_IUnknown){
      *ppvObj = static_cast<IClassFactory*>(this);
   }

   else if (riid == IID_IClassFactory){
      *ppvObj = static_cast<IClassFactory*>(this);
   }

   else{
      *ppvObj = NULL;
      return E_NOINTERFACE;
   }

   static_cast<IUnknown*>(*ppvObj)->AddRef();
   return S_OK;
}

/******************************************************************************
*   AddRef() -- In order to allow an object to delete itself when it is no
*   longer needed, it is necessary to maintain a count of all references to
*   this object.  When a new reference is created, this function increments
*   the count.
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServerClassFactory::AddRef()
{
   return ++m_refCount;
}

/******************************************************************************
*   Release() -- When a reference to this object is removed, this function
*   decrements the reference count.  If the reference count is 0, then this
*   function deletes this object and returns 0;
******************************************************************************/
STDMETHODIMP_(ULONG) RTDServerClassFactory::Release()
{
   if (--m_refCount == 0)
   {
      delete this;
      return 0;
   }
   return m_refCount;
}


/******* IClassFactory Methods *******/
/******************************************************************************
*   CreateInstance() -- This method attempts to create an instance of RTDServer
*   and returns it to the caller.  It maintains a count of the number of
*   created objects.
******************************************************************************/
STDMETHODIMP RTDServerClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
                                                      REFIID riid,
                                                      LPVOID *ppvObj)
{
   HRESULT hr;
   RTDServer* pObj;

   *ppvObj = NULL;
   hr = ResultFromScode(E_OUTOFMEMORY);

   //It's illegal to ask for anything but IUnknown when aggregating
   if ((pUnkOuter != NULL) && (riid != IID_IUnknown))
      return E_INVALIDARG;

   //Create a new instance of RTDServer
   pObj = new RTDServer(pUnkOuter);

   if (pObj == NULL)
      return hr;

   //Return the resulting object
   hr = pObj->NonDelegatingQueryInterface(riid, ppvObj);

   if (FAILED(hr))
      delete pObj;

   return hr;
}

/******************************************************************************
*   LockServer() -- This method maintains a count of the current locks on this
*   DLL.  The count is used to determine if the DLL can be unloaded, or if
*   clients are still using it.
******************************************************************************/
STDMETHODIMP RTDServerClassFactory::LockServer(BOOL fLock)
{
   if (fLock)
      InterlockedIncrement( &g_cLock );
   else
      InterlockedDecrement( &g_cLock );
   return NOERROR;
}

/******* Exported DLL functions *******/
/******************************************************************************
*  g_RegTable -- This N*3 array contains the keys, value names, and values that
*  are associated with this dll in the registry.
******************************************************************************/
const char *g_RegTable[][3] = {
   //format is {key, value name, value }
   {PROG_ID, 0, PROG_ID},
   {PROG_ID_CLSID_KEY, 0, "{6361D863-60AA-4582-A730-6C719FC324B7}"},

   {"CLSID\\{6361D863-60AA-4582-A730-6C719FC324B7}", 0, PROG_ID},
   {"CLSID\\{6361D863-60AA-4582-A730-6C719FC324B7}\\InprocServer32", 0,
      (const char*)-1},
   {"CLSID\\{6361D863-60AA-4582-A730-6C719FC324B7}\\ProgId", 0, PROG_ID},
   {"CLSID\\{6361D863-60AA-4582-A730-6C719FC324B7}\\TypeLib", 0,
      "{217C2068-49F7-4207-B14D-B36FB7F9BB17}"},

   //copied this from Kruglinski with uuids and names changed.
   //Just marks where the typelib is
   {"TypeLib\\{217C2068-49F7-4207-B14D-B36FB7F9BB17}", 0, PROG_ID},
   {"TypeLib\\{217C2068-49F7-4207-B14D-B36FB7F9BB17}\\1.0", 0, PROG_ID},
   {"TypeLib\\{217C2068-49F7-4207-B14D-B36FB7F9BB17}\\1.0\\0", 0, "win32"},
   {"TypeLib\\{217C2068-49F7-4207-B14D-B36FB7F9BB17}\\1.0\\0\\Win32", 0,
      (const char*)-1},
   {"TypeLib\\{217C2068-49F7-4207-B14D-B36FB7F9BB17}\\1.0\\FLAGS", 0, "0"},
};

/******************************************************************************
*  DLLRegisterServer -- This method is the exported method that is used by
*  COM to self-register this component.  It removes the need for a .reg file.
*  ( Taken from Don Box's _Essential COM_ pg. 110-112)
******************************************************************************/
STDAPI DllRegisterServer(void)
{
   HRESULT hr = S_OK;

   //look up server's file name
   ACE_TCHAR szFileName[255];
   HMODULE dllModule = GetModuleHandle(DLL_NAME);
   GetModuleFileName(dllModule, szFileName, 255);

   //register entries from the table
   int nEntries = sizeof(g_RegTable)/sizeof(*g_RegTable);
   for (int i = 0; SUCCEEDED(hr) && i < nEntries; i++)
   {
      const char *pszName = g_RegTable[i][0];
      const char *pszValueName = g_RegTable[i][1];
      const char *pszValue = g_RegTable[i][2];

      //Map rogue values to module file name
      if (pszValue == (const char*)-1)
         pszValue = ACE_TEXT_ALWAYS_CHAR(szFileName);

      //Create the key
      HKEY hkey;
      long err = RegCreateKeyA( HKEY_CLASSES_ROOT, pszName, &hkey);

      //Set the value
      if (err == ERROR_SUCCESS){
         err = RegSetValueExA( hkey, pszValueName, 0, REG_SZ,
            (const BYTE*)pszValue, static_cast<DWORD>(strlen(pszValue) + 1));
         RegCloseKey(hkey);
      }

      //if cannot add key or value, back out and fail
      if (err != ERROR_SUCCESS){
         DllUnregisterServer();
         hr = SELFREG_E_CLASS;
      }
   }
   return hr;
}

/******************************************************************************
*  DllUnregisterServer -- This method is the exported method that is used by
*  COM to remove the keys added to the registry by DllRegisterServer.  It
*  is essentially for housekeeping.
*  (Taken from Don Box, _Essential COM_ pg 112)
******************************************************************************/
STDAPI DllUnregisterServer(void)
{
   HRESULT hr = S_OK;

   int nEntries = sizeof(g_RegTable)/sizeof(*g_RegTable);

   for (int i = nEntries - 1; i >= 0; i--){
      const char * pszKeyName = g_RegTable[i][0];

      long err = RegDeleteKeyA(HKEY_CLASSES_ROOT, pszKeyName);
      if (err != ERROR_SUCCESS)
         hr = S_FALSE;
   }
   return hr;
}

/******************************************************************************
*   DllGetClassObject() -- This method is the exported method that clients use
*   to create objects in the DLL.  It uses class factories to generate the
*   desired object and returns it to the caller.  The caller must call Release()
*   on the object when they're through with it.
******************************************************************************/
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR * ppvObj)
{
   //Make sure the requested class is supported by this server
   if (!IsEqualCLSID(rclsid, CLSID_RTDServer))
      return ResultFromScode(E_FAIL);

   //Make sure the requested interface is supported
   if ((!IsEqualCLSID(riid, IID_IUnknown)) && (!IsEqualCLSID(riid,
      IID_IClassFactory)))
      return ResultFromScode(E_NOINTERFACE);

   //Create the class factory
   *ppvObj = (LPVOID) new RTDServerClassFactory();

   //error checking
   if (*ppvObj == NULL)
      return ResultFromScode(E_OUTOFMEMORY);

   //Addref the Class Factory
   ((LPUNKNOWN)*ppvObj)->AddRef();

   return NOERROR;
}

/******************************************************************************
*   DllCanUnloadNow() -- This method checks to see if it's alright to unload
*   the dll by determining if there are currently any locks on the dll.
******************************************************************************/
STDAPI DllCanUnloadNow()
{
   if ((g_cLock == 0) && (g_cOb == 0))
      return S_OK;
   else
      return S_FALSE;
}
