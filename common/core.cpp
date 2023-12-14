/*

Copyright (C) 2021-2023  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "core.h"
#include "win32_exception.h"

#include <shellapi.h>

namespace pv {

Core::Core(int argc, char *argv[])
{
#ifdef WIN32
	// https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw
	// Convert command line to UTF-8 argv
	LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
	PV_THROW_LAST_ERROR_IF(!argvw);
	PV_FINALLY([&]() -> void { LocalFree(argvw); });
	if (argc == 0) // This should never happen, always have at least the executable as argument
		PV_THROW(Exception("CommandLineToArgvW returned 0 arguments"sv, Literal));
	int len = sizeof(char *) * argc;
	for (int i = 0; i < argc; ++i)
	{
		int reqLen = WideCharToMultiByte(CP_UTF8, 0,
		    argvw[i], -1,
		    null, 0,
		    null, null);
		PV_THROW_LAST_ERROR_IF(!reqLen);
		len += reqLen;
	}
	argv = (char **)(new char[len]);
	char *argi = (char *)(&argv[argc]);
	PV_FINALLY([&]() -> void { delete[] (char *)argv; }); // Exception destructor
	for (int i = 0; i < argc; ++i)
	{
		argv[i] = argi;
		int resLen = WideCharToMultiByte(CP_UTF8, 0,
		    argvw[i], -1,
		    argi, len - (int)((ptrdiff_t)argi - (ptrdiff_t)argv),
		    null, null);
		PV_THROW_LAST_ERROR_IF(!resLen);
		argi += resLen;
	}

	// Get module handle
	m_ExecutableModule = (HINSTANCE)GetModuleHandleW(NULL);

	// https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-extracticonw
	// Use icon from the executable, if any
	// Don't throw exceptions if this fails, it's ok
	WCHAR szExePath[MAX_PATH];
	DWORD res = GetModuleFileNameW(NULL, szExePath, MAX_PATH);
	HICON executableIcon;
	if (!res || res == MAX_PATH)
	{
		executableIcon = NULL;
	}
	else
	{
		executableIcon = ExtractIconW(NULL, szExePath, 0);
		if (executableIcon == (HICON)1)
			executableIcon = NULL;
	}
	PV_FINALLY([&]() -> void { DestroyIcon(executableIcon); }); // Exception destructor
#endif

	// Commit
	m_ArgC = argc;
	m_ArgV = argv;
#ifdef WIN32
	argv = null;
	m_ExecutableIcon = executableIcon;
	executableIcon = NULL;
#endif
}

Core::~Core()
{
#ifdef WIN32
	DestroyIcon(m_ExecutableIcon);
	delete[] (char *)m_ArgV;
#endif
}

} /* namespace pv */

/* end of file */
