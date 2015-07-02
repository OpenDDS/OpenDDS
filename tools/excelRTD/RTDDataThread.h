/*
 *
 * Derived from sample code included in Microsoft Knowledge Base Article 327215.
 * Additional material distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "IRTDServer.h"

DWORD WINAPI RealTimeDataThread( LPVOID CallbackObject);
WPARAM MessageLoop();
void ThreadOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

//Commands to the thread
#define WM_TERMINATE 100
#define WM_SILENTTERMINATE 101
