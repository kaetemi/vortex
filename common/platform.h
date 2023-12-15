/*

Copyright (C) 2019-2023  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

/*

Polyverse OÜ base include file for C++.

*/

#pragma once
#ifndef PV_PLATFORM_H
#define PV_PLATFORM_H

// Use C math defines for M_PI
#define _USE_MATH_DEFINES

#ifdef _WIN32
// Include Win32.
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0600
#ifdef __cplusplus
#define NOMINMAX
#endif /* __cplusplus */
#endif /* _WIN32 */

// Ensure malloc is included before anything else,
// There are some odd macros that may conflict otherwise.
#include <malloc.h>
#ifdef __cplusplus
// Include STL algorithm to ensure std::min and std::max
// are used everywhere, instead of min and max macros.
#include <algorithm>
using std::max;
using std::min;
#endif /* __cplusplus */

#ifdef _WIN32
#include <Windows.h>
#ifdef _MSC_VER
// #include <codeanalysis\sourceannotations.h>
#endif
#endif /* _WIN32 */

// C++
#ifdef __cplusplus

// Require C++17
#if defined(_MSC_VER) && (!defined(_HAS_CXX17) || !_HAS_CXX17)
static_assert(false, "C++17 is required");
#endif

// Require C++20
#if defined(_MSC_VER) && (!defined(_HAS_CXX20) || !_HAS_CXX20)
static_assert(false, "C++20 is required");
#endif

// Define null, with color highlight
#ifndef null
constexpr decltype(nullptr) null = nullptr;
#define null null
#endif

// Include STL string and allow string literals.
// Always use sv suffix when declaring string literals.
// Ideally, assign them as `constexpr std::string_view`.
#include <string>
#include <string_view>
using namespace std::string_literals;
using namespace std::string_view_literals;

// A couple of types for the global namespace, for everyone's sanity.
#if defined(_HAS_CXX20) && _HAS_CXX20
#include <compare>
// using std::strong_ordering;
#endif
// using std::move;
// using std::forward;
// using std::nothrow_t;
// using std::nothrow;

// Include GSL
// auto _ = gsl::finally([&] { delete xyz; });
#include "gsl/util"

// The usual
#include <functional>
#include <format>

#endif /* __cplusplus */

// Force inline
#ifdef _MSC_VER
#define PV_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define PV_FORCE_INLINE inline __attribute__((always_inline))
#else
#define PV_FORCE_INLINE inline
#endif

#if defined(_DEBUG) && !defined(NDEBUG)
#define PV_DEBUG
#else
#define PV_RELEASE
#endif

#define PV_STR(a) PV_STR_IMPL(a)
#define PV_STR_IMPL(a) #a

#define PV_CONCAT_IMPL(a, b) a##b
#define PV_CONCAT(a, b) PV_CONCAT_IMPL(a, b)

#define PV_FINALLY(f) auto PV_CONCAT(finally__, __COUNTER__) = gsl::finally(f)

// Include debug_break
#include <debugbreak.h>
#define PV_RELEASE_BREAK() debug_break()
#define PV_RELEASE_ASSERT(cond)          \
	do {                                 \
		if (!(cond)) PV_RELEASE_BREAK(); \
	} while (false)
#define PV_RELEASE_VERIFY(cond)          \
	do {                                 \
		if (!(cond)) PV_RELEASE_BREAK(); \
	} while (false)

#ifdef PV_DEBUG
#define PV_DEBUG_BREAK() debug_break()
#define PV_DEBUG_ASSERT(cond)          \
	do {                               \
		if (!(cond)) PV_DEBUG_BREAK(); \
	} while (false)
#define PV_DEBUG_VERIFY(cond)          \
	do {                               \
		if (!(cond)) PV_DEBUG_BREAK(); \
	} while (false)
#ifdef _WIN32
#define PV_DEBUG_OUTPUT(str)                                      \
	do {                                                          \
		auto s = (str);                                           \
		std::string_view sv = s;                                  \
		wchar_t *wstr = (wchar_t *)_malloca((sv.size() + 2) * 2); \
		if (!wstr) break;                                         \
		PV_FINALLY([&]() -> void { _freea(wstr); });              \
		int wlen = MultiByteToWideChar(CP_UTF8, 0,                \
		    &sv[0], (int)sv.size(),                               \
		    wstr, (int)sv.size() * 2);                            \
		if (!wlen) break;                                         \
		wstr[wlen] = 0;                                           \
		OutputDebugStringW(wstr);                                 \
	} while (false)
#define PV_DEBUG_OUTPUT_LF(str)                                   \
	do {                                                          \
		auto s = (str);                                           \
		std::string_view sv = s;                                  \
		wchar_t *wstr = (wchar_t *)_malloca((sv.size() + 2) * 2); \
		if (!wstr) break;                                         \
		PV_FINALLY([&]() -> void { _freea(wstr); });              \
		int wlen = MultiByteToWideChar(CP_UTF8, 0,                \
		    &sv[0], (int)sv.size(),                               \
		    wstr, (int)sv.size() * 2);                            \
		if (!wlen) break;                                         \
		wstr[wlen] = '\n';                                        \
		wstr[wlen + 1] = 0;                                       \
		OutputDebugStringW(wstr);                                 \
	} while (false)
#else
#define PV_DEBUG_OUTPUT(str) \
	do {                     \
	} while (false)
#define PV_DEBUG_OUTPUT_LF(str) \
	do {                        \
	} while (false)
#endif
#define PV_THROW(ex)                    \
	do {                                \
		auto ex_ = (ex);                \
		PV_DEBUG_OUTPUT_LF(ex_.what()); \
		debug_break();                  \
		throw ex_;                      \
	} while (false)
#else
#define PV_DEBUG_BREAK() \
	do {                 \
	} while (false)
#define PV_DEBUG_ASSERT(cond) \
	do {                      \
	} while (false)
#define PV_DEBUG_VERIFY(cond) \
	do {                      \
		cond;                 \
	} while (false)
#define PV_DEBUG_OUTPUT(str) \
	do {                     \
	} while (false)
#define PV_DEBUG_OUTPUT_LF(str) \
	do {                        \
	} while (false)
#define PV_THROW(ex) \
	do {             \
		throw ex;    \
	} while (false)
#endif

#define PV_OUTPUT_CHAR_BUFFER (_ALLOCA_S_THRESHOLD / 4)

namespace pv {

struct OutputDebugContainer
{
public:
	typedef char value_type;

private:
	char m_Buffer[PV_OUTPUT_CHAR_BUFFER];
	int m_Length = 0;

	void flush()
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
		PV_DEBUG_OUTPUT(std::string_view(m_Buffer, len));
		int remain = m_Length - len;
		for (int i = 0; i < remain; ++i)
			m_Buffer[i] = m_Buffer[len + i];
		m_Length = remain;
	}

public:
	PV_FORCE_INLINE void push_back(char c)
	{
		m_Buffer[m_Length] = c;
		++m_Length;
		if (m_Length >= sizeof(m_Buffer))
			flush();
	}

#pragma warning(push)
#pragma warning(disable : 26495) // C not initialized on purpose
	PV_FORCE_INLINE OutputDebugContainer()
	{
	}
#pragma warning(pop)
	PV_FORCE_INLINE ~OutputDebugContainer()
	{
		if (m_Length)
			PV_DEBUG_OUTPUT(std::string_view(m_Buffer, m_Length));
	}
};

} /* namespace pv */

#ifdef PV_DEBUG
#define PV_DEBUG_FORMAT(format, ...) ([&]() -> void { pv::OutputDebugContainer pv; std::format_to(std::back_inserter(pv), format, __VA_ARGS__); })()
#else
#define PV_DEBUG_FORMAT(format, ...) \
	do {                             \
	} while (false)
#endif

#define PV_SAFE_C_DELETE(del, ptr) \
	if (ptr)                       \
	{                              \
		del(ptr);                  \
		ptr = NULL;                \
	}

#define PV_SAFE_DELETE(ptr) \
	delete ptr;             \
	ptr = null;

#endif /* PV_PLATFORM_H */

/* end of file */
