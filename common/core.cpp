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

// STL
#include <iostream>

// Win32
#ifdef _WIN32
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#endif

// Project
#include "win32_exception.h"
#include "string_ex.h"

namespace pv {

namespace /* anonymous */ {

#ifdef _WIN32

bool isUtf8Locale(const wchar_t *locale)
{
	if (!locale)
		return false;
	std::wstring_view lsv = locale;
	if (endsWith(lsv, L".UTF-8"sv)
	    || endsWith(lsv, L".UTF8"sv)
	    || endsWith(lsv, L".utf-8"sv)
	    || endsWith(lsv, L".utf8"sv))
		return true;
	return false;
}

UINT getCpFromLcid(LCID lcid)
{
	UINT cp;
	int sizeInChars = sizeof(cp) / sizeof(wchar_t);
	if (GetLocaleInfoW(lcid,
		LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
		reinterpret_cast<LPWSTR>(&cp),
		sizeInChars) != sizeInChars) {
		return CP_ACP;
	}
	return cp;
}

#endif

} /* anonymous namespace */

Core::Core(int argc, char *argv[])
{
#ifdef _WIN32
	// Check if all APIs are using UTF-8
	// Enforce it if possible
	bool isUtf8Clean = false;
	UINT acp = GetACP();
	if (acp == CP_UTF8)
	{
		// Update C/C++ locale
		const wchar_t *pbkp = _wsetlocale(LC_ALL, null);
		std::wstring bkp = pbkp ? pbkp : L"";
		const wchar_t *locale = _wsetlocale(LC_ALL, L"C.UTF-8");
		if (!isUtf8Locale(locale))
		{
			locale = _wsetlocale(LC_ALL, L"en_US.UTF-8");
			if (!isUtf8Locale(locale)) locale = _wsetlocale(LC_ALL, L".UTF-8");
		}
		if (isUtf8Locale(locale) || isUtf8Locale(_wsetlocale(LC_ALL, null)))
		{
			int prevStdOutMode = _setmode(_fileno(stdout), _O_TEXT);
			if (prevStdOutMode != -1)
			{
				SetConsoleOutputCP(CP_UTF8);
				isUtf8Clean = (GetConsoleOutputCP() == CP_UTF8);
			}
		}
		if (!isUtf8Clean && pbkp) // Attempt to revert changes on failure
			_wsetlocale(LC_ALL, bkp.c_str());
	}
	if (!isUtf8Clean) // This ensures we can output Unicode characters to the console when UTF-8 is not available
	{
		// _O_U8TEXT makes stdout only accept wchar_t but translates it to UTF-8
		// Same for _O_U16TEXT, but the output on stdout read by other processes will be UTF-16
		(void)_setmode(_fileno(stdout), _O_U8TEXT);
	}
	m_Utf8Clean = isUtf8Clean;

	// This allows us to get the legacy codepage used by non-Unicode applications
	LCID lcid = GetSystemDefaultLCID();
	UINT cpLegacy = getCpFromLcid(lcid);
	if (cpLegacy == CP_ACP)
		cpLegacy = acp;
	m_CpLegacy = cpLegacy;

	// https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw
	// Convert command line to UTF-8 argv
	// Always use UTF-16 for command line arguments
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
#ifdef _WIN32
	argv = null;
	m_ExecutableIcon = executableIcon;
	executableIcon = NULL;
#endif
}

Core::~Core()
{
#ifdef _WIN32
	DestroyIcon(m_ExecutableIcon);
	delete[] (char *)m_ArgV;
#endif
}

void Core::printImpl(std::string_view str)
{
	if (isUtf8Clean())
	{
		std::cout << str;
	}
	else
	{
#ifdef _WIN32
		// Convert from UTF-8 to wide
		wchar_t *tmp = (wchar_t *)_malloca((str.length() + 1) * 4);
		if (!tmp)
			throw std::bad_alloc();
		PV_FINALLY([&] { _freea(tmp); });
		int tmpLen = MultiByteToWideChar(CP_UTF8, 0,
		    str.data(), (int)str.length(),
		    tmp, (int)(str.length() * 2));
		if (!tmpLen)
			return;
		tmp[tmpLen] = L'\0';
		std::wcout << std::wstring_view(tmp, (size_t)tmpLen);
#endif
	}
}

void Core::print(std::string_view str)
{
	if (!str.length())
		return;
	std::unique_lock<std::mutex> lock(m_PrintMutex);
	printImpl(str);
}

void Core::printLf(std::string_view str)
{
	std::unique_lock<std::mutex> lock(m_PrintMutex);
	if (!str.length())
	{
		if (isUtf8Clean())
			std::cout << "\n";
		else
			std::wcout << L"\n";
		return;
	}
	if (isUtf8Clean())
	{
		std::cout << str << "\n";
	}
	else
	{
#ifdef _WIN32
		// Convert from UTF-8 to wide
		wchar_t *tmp = (wchar_t *)_malloca((str.length() + 2) * 4);
		if (!tmp)
			throw std::bad_alloc();
		PV_FINALLY([&] { _freea(tmp); });
		int tmpLen = MultiByteToWideChar(CP_UTF8, 0,
		    str.data(), (int)str.length(),
		    tmp, (int)(str.length() * 2));
		if (!tmpLen)
			return;
		tmp[tmpLen] = L'\n';
		tmp[tmpLen + 1] = L'\0';
		std::wcout << std::wstring_view(tmp, (size_t)tmpLen + 1);
#endif
	}
}

void Core::printLf()
{
	std::unique_lock<std::mutex> lock(m_PrintMutex);
	if (isUtf8Clean())
	{
		std::cout << "\n";
	}
	else
	{
#ifdef _WIN32
		std::wcout << L"\n";
#endif
	}
}

void Core::print(const char *str)
{
	if (isUtf8Clean())
	{
		std::unique_lock<std::mutex> lock(m_PrintMutex);
		std::cout << str;
	}
	else
	{
		print(std::string_view(str));
	}
}

void Core::printLf(const char *str)
{
	if (isUtf8Clean())
	{
		std::unique_lock<std::mutex> lock(m_PrintMutex);
		std::cout << str << "\n";
	}
	else
	{
		printLf(std::string_view(str));
	}
}

void PrintContainer::flush()
{
	int len = m_Length;
	if (m_Buffer[len - 1] & 0x80)
	{
		// Last character may be incomplete
		--len;
		while (!(m_Buffer[len] & 0x40))
			--len;
		// Check if it's complete
		int remain = m_Length - len;
		if (remain == 4)
		{
			if ((m_Buffer[len] & 0xF8) == 0xF0)
				len += 4;
		}
		else if (remain == 3)
		{
			if ((m_Buffer[len] & 0xF0) == 0xE0)
				len += 3;
		}
		else if (remain == 2)
		{
			if ((m_Buffer[len] & 0xE0) == 0xC0)
				len += 2;
		}
		else
		{
			// Invalid UTF-8
			len = m_Length;
		}
	}
	m_Core.printImpl(std::string_view(m_Buffer, len));
	int remain = m_Length - len;
	for (int i = 0; i < remain; ++i)
		m_Buffer[i] = m_Buffer[len + i];
	m_Length = remain;
}

} /* namespace pv */

/* end of file */
