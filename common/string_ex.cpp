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

#include "string_ex.h"

namespace pv {

#ifdef _WIN32

std::wstring winCpToWide(const char *str, size_t len, UINT cp)
{
	if (!len)
		len = strlen(str);
	if (!len)
		return std::wstring();

	// Convert from codepage to wide
	wchar_t *tmp = (wchar_t *)_malloca((len + 1) * 4);
	if (!tmp)
		throw std::bad_alloc();
	PV_FINALLY([&] { _freea(tmp); });
	int tmpLen = MultiByteToWideChar(cp, 0,
	    str, (int)(len + 1), /* include null-termination */
	    tmp, (int)((len + 1) * 2));
	if (tmpLen <= 1)
		return std::wstring();

	std::wstring res(tmp, (size_t)tmpLen - 1);
	return res;
}

// Convert UTF-8 to wide character set
std::wstring utf8ToWide(const char *str, size_t len)
{
	return winCpToWide(str, len, CP_UTF8);
}

std::wstring utf8ToWide(const std::string &str)
{
	return utf8ToWide(str.c_str(), str.size());
}

#endif

} /* namespace pv */

/* end of file */
