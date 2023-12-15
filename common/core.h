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

#pragma once
#ifndef PV_CORE_H
#define PV_CORE_H

#include "platform.h"

#include <mutex>

namespace pv {

// Core platform behaviour
// All strings are UTF-8 encoded
class Core
{
public:
	Core(int argc, char *argv[]);
	~Core();

	// Process arguments
	PV_FORCE_INLINE int argC() { return m_ArgC; }
	PV_FORCE_INLINE char **argV() { return m_ArgV; }
	PV_FORCE_INLINE char *argV(int i) { return m_ArgV[i]; }

#ifdef _WIN32
	PV_FORCE_INLINE HINSTANCE executableModule()
	{
		return m_ExecutableModule;
	}
	PV_FORCE_INLINE HICON executableIcon()
	{
		return m_ExecutableIcon;
	}
#endif

	// If the system is UTF-8 clean, all 8-bit strings are UTF-8 encoded
	// Otherwise, we must use UTF-16 APIs for system interaction instead
	PV_FORCE_INLINE bool isUtf8Clean()
	{
#ifdef _WIN32
		return m_Utf8Clean;
#else
		return true;
#endif
	}

	void print(std::string_view str);
	void printLf(std::string_view str);

	void print(const char *str);
	void printLf(const char *str);

	template <typename... TArgs>
	void printF(std::string_view format, TArgs... args)
	{
		std::format_to(std::back_insert_iterator(pv::PrintContainer(*this)), format, args...);
	}

	template <typename... TArgs>
	void printF(const char *format, TArgs... args)
	{
		std::format_to(std::back_insert_iterator(pv::PrintContainer(*this)), format, args...);
	}

	template <typename... TArgs>
	void printF(const std::string &format, TArgs... args)
	{
		std::format_to(std::back_insert_iterator(pv::PrintContainer(*this)), format, args...);
	}

private:
	void printImpl(std::string_view str);
	friend struct PrintContainer;

	int m_ArgC;
	char **m_ArgV;
	std::mutex m_PrintMutex;

#ifdef _WIN32
	HINSTANCE m_ExecutableModule;
	HICON m_ExecutableIcon;
	bool m_Utf8Clean;
#endif
};

struct PrintContainer
{
public:
	typedef char value_type;

private:
	char m_Buffer[PV_OUTPUT_CHAR_BUFFER];
	int m_Length = 0;
	Core &m_Core;
	std::unique_lock<std::mutex> m_Lock;

	void flush();

public:
	PV_FORCE_INLINE void push_back(char c)
	{
		m_Buffer[m_Length] = c;
		++m_Length;
		if (m_Length >= sizeof(m_Buffer))
			flush();
	}

#pragma warning(push)
#pragma warning(disable : 26495) // Buffer not initialized on purpose
	PV_FORCE_INLINE PrintContainer(Core &core)
	    : m_Core(core)
	    , m_Lock(core.m_PrintMutex)
	{
	}
#pragma warning(pop)
	PV_FORCE_INLINE ~PrintContainer()
	{
		if (m_Length)
			m_Core.printImpl(std::string_view(m_Buffer, m_Length));
	}
};

} /* namespace pv */

#define PV_PRINT_FORMAT(core, format, ...) ([&]() -> void { std::format_to(std::back_insert_iterator(pv::PrintContainer(core)), format, __VA_ARGS__); })()

#endif /* #ifndef PV_CORE_H */

/* end of file */
