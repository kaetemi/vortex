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

#pragma once
#ifndef PV_STRING_H
#define PV_STRING_H

#include "platform.h"

#include <string>
#include <string_view>

namespace pv {

#ifdef _WIN32

// Convert UTF-8 to wide character set
std::wstring utf8ToWide(const char *str, size_t len = 0);
std::wstring utf8ToWide(const std::string &str);

#endif

// Fast string to bool, reliably defined for strings starting with 0, 1, t, T, f, F, y, Y, n, N, and empty strings, anything else is undefined.
inline bool toBool(const char *str) { return str[0] == '1' || (str[0] & 0xD2) == 0x50; }
inline bool toBool(const std::string &str) { return toBool(str.c_str()); } // Safe because first byte may be null

inline bool startsWith(const char *str, const char *prefix)
{
	for (int i = 0;; ++i)
	{
		if (str[i] != prefix[i] || str[i] == '\0')
		{
			return prefix[i] == '\0';
		}
	}
}

inline bool startsWith(const std::string &str, const char *prefix) { return startsWith(str.c_str(), prefix); }
inline bool startsWith(const std::string &str, const std::string &prefix) { return startsWith(str.c_str(), prefix.c_str()); }

inline bool endsWith(const char *str, size_t strLen, const char *suffix, size_t suffixLen)
{
	if (strLen < suffixLen)
		return false;
	size_t minLen = strLen < suffixLen ? strLen : suffixLen;
	for (size_t i = 1; i <= minLen; ++i)
		if (str[strLen - i] != suffix[suffixLen - i])
			return false;
	return true;
}

inline bool endsWith(const char *str, const char *suffix) { return endsWith(str, strlen(str), suffix, strlen(suffix)); }
inline bool endsWith(const char *str, std::string_view suffix) { return endsWith(str, strlen(str), suffix.data(), suffix.length()); }
inline bool endsWith(std::string_view str, std::string_view suffix) { return endsWith(str.data(), str.length(), suffix.data(), suffix.length()); }
inline bool endsWith(const std::string &str, const char *suffix) { return endsWith(str.c_str(), str.size(), suffix, strlen(suffix)); }
inline bool endsWith(const std::string &str, const std::string &suffix) { return endsWith(str.c_str(), str.size(), suffix.c_str(), suffix.size()); }

#ifdef _WIN32

inline bool endsWith(const wchar_t *str, size_t strLen, const wchar_t *suffix, size_t suffixLen)
{
	if (strLen < suffixLen)
		return false;
	size_t minLen = strLen < suffixLen ? strLen : suffixLen;
	for (size_t i = 1; i <= minLen; ++i)
		if (str[strLen - i] != suffix[suffixLen - i])
			return false;
	return true;
}

inline bool endsWith(const wchar_t *str, const wchar_t *suffix) { return endsWith(str, wcslen(str), suffix, wcslen(suffix)); }
inline bool endsWith(const wchar_t *str, std::wstring_view suffix) { return endsWith(str, wcslen(str), suffix.data(), suffix.length()); }
inline bool endsWith(std::wstring_view str, std::wstring_view suffix) { return endsWith(str.data(), str.length(), suffix.data(), suffix.length()); }
inline bool endsWith(const std::wstring &str, const wchar_t *suffix) { return endsWith(str.c_str(), str.size(), suffix, wcslen(suffix)); }
inline bool endsWith(const std::wstring &str, const std::wstring &suffix) { return endsWith(str.c_str(), str.size(), suffix.c_str(), suffix.size()); }

#endif /* _WIN32 */

} /* namespace pv */

#endif /* #ifndef PV_STRING_H */

/* end of file */
