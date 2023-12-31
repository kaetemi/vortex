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

#include "win32_exception.h"

namespace pv {

namespace /* anonymous */ {

std::string_view getWin32Message(DWORD errorCode) noexcept
{
	// Get UTF-16 message
	wchar_t *msgBuf;
	ptrdiff_t msgLen = FormatMessageW(
	    FORMAT_MESSAGE_ALLOCATE_BUFFER
	        | FORMAT_MESSAGE_FROM_SYSTEM
	        | FORMAT_MESSAGE_IGNORE_INSERTS,
	    null,
	    errorCode,
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    (LPWSTR)&msgBuf,
	    0, null);
	if (!msgLen)
		return std::string_view();
	auto f7 = gsl::finally([msgBuf] { LocalFree(msgBuf); });

	// Get required UTF-8 length (NUL-terminated length)
	int reqLen = WideCharToMultiByte(CP_UTF8, 0,
	    msgBuf, (int)(msgLen + 1),
	    null, 0,
	    null, null);
	if (reqLen <= 1) // Failed conversion or empty string
		return std::string_view();

	// Get UTF-8 string
	char *utf8Buf = new (std::nothrow) char[reqLen];
	if (!utf8Buf)
		return std::string_view();
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0,
	    msgBuf, (int)(msgLen + 1),
	    utf8Buf, reqLen,
	    null, null);
	if (!utf8Len) // Failed conversion
		return std::string_view();
	if (utf8Len <= 1) // Empty string
	{
		delete[] utf8Buf;
		return std::string_view();
	}
	return std::string_view(utf8Buf, (ptrdiff_t)utf8Len - 1);
}

std::string_view getMessage(const std::string_view systemMessage, const HRESULT hr, const DWORD errorCode, const std::string_view file, const int line) noexcept
{
	const std::string_view hresultTxt = "HRESULT: 0x"sv;
	const std::string_view dwordTxt = "DWORD: 0x"sv;
	const std::string_view fileTxt = "File: "sv;
	const std::string_view lineTxt = ", line: "sv;
	const ptrdiff_t maxLen = (!systemMessage.empty() ? (systemMessage.size() + 1) : 0) // Message // \n
	    + (hr != S_OK ? (hresultTxt.size() + sizeof(hr) * 2 + 1) : 0) // HRESULT: 0x // 00000000 // \n
	    + ((hr == S_OK && errorCode) ? (dwordTxt.size() + sizeof(errorCode) * 2 + 1) : 0) // DWORD: 0x // 00000000 // \n
	    + fileTxt.size() + file.size() + lineTxt.size() + 11 + 1; // File: // a.cpp // , line: // 0 // \0
	char *buf = new (std::nothrow) char[maxLen];
	if (!buf)
		return std::string_view();
	ptrdiff_t i = 0;
	if (!systemMessage.empty())
	{
		memcpy(buf /* &buf[i] */, systemMessage.data(), systemMessage.size());
		i += systemMessage.size();
		if (!i || buf[i - 1] != '\n')
		{
			buf[i] = '\n';
			++i;
		}
	}
	if (hr != S_OK)
	{
		memcpy(&buf[i], hresultTxt.data(), hresultTxt.size());
		i += hresultTxt.size();
		sprintf_s(&buf[i], maxLen - i, "%08x", hr);
		i += 8;
		buf[i] = '\n';
		++i;
	}
	else if (errorCode)
	{
		memcpy(&buf[i], dwordTxt.data(), dwordTxt.size());
		i += dwordTxt.size();
		sprintf_s(&buf[i], maxLen - i, "%08x", errorCode);
		i += 8;
		buf[i] = '\n';
		++i;
	}
	memcpy(&buf[i], fileTxt.data(), fileTxt.size());
	i += fileTxt.size();
	memcpy(&buf[i], file.data(), file.size());
	i += file.size();
	memcpy(&buf[i], lineTxt.data(), lineTxt.size());
	i += lineTxt.size();
	i += sprintf_s(&buf[i], maxLen - i, "%d", line);
	buf[i] = '\0';
	return std::string_view(buf, i);
}

std::string_view copyString(const std::string_view str) noexcept
{
	if (str.empty())
		return std::string_view();
	char *buf = new (std::nothrow) char[str.size() + 1];
	if (!buf)
		return std::string_view();
	memcpy(buf, str.data(), str.size());
	buf[str.size()] = '\0';
	return std::string_view(buf, str.size());
}

}

Win32Exception::Win32Exception(const HRESULT hr, const DWORD errorCode, const StringView file, const int line) noexcept
    : base("Unknown Win32 exception"sv, 1)
    , m_HResult(hr)
    , m_ErrorCode(errorCode ? errorCode : (HRESULT_FACILITY(hr) == FACILITY_WINDOWS ? HRESULT_CODE(hr) : hr))
    , m_File(file)
    , m_Line(line)
    , m_SystemMessage(getWin32Message(m_ErrorCode))
    , m_Message(getMessage(m_SystemMessage.sv(), hr, m_ErrorCode, file.sv(), line))
{
}

Win32Exception::~Win32Exception() noexcept
{
	delete[] m_Message.Data;
	delete[] m_SystemMessage.Data;
}

Win32Exception::Win32Exception(const Win32Exception &other) noexcept
    : base(other)
    , m_HResult(other.m_HResult)
    , m_ErrorCode(other.m_ErrorCode)
    , m_File(other.m_File)
    , m_Line(other.m_Line)
    , m_SystemMessage(copyString(other.m_SystemMessage.sv()))
    , m_Message(copyString(other.m_Message.sv()))
{
}

Win32Exception &Win32Exception::operator=(Win32Exception const &other) noexcept
{
	if (this != &other)
	{
		this->~Win32Exception();
		new (this) Win32Exception(other);
	}
	return *this;
}

[[nodiscard]] std::string_view Win32Exception::what() const
{
	return m_Message.Data ? m_Message.sv() : (m_SystemMessage.Data ? m_SystemMessage.sv() : base::what());
}

Exception::StringView Win32Exception::systemMessageImpl(const HRESULT hr, DWORD errorCode)
{
	if (!errorCode)
		errorCode = (HRESULT_FACILITY(hr) == FACILITY_WINDOWS ? HRESULT_CODE(hr) : hr);
	return getWin32Message(errorCode);
}

void Win32Exception::destroyStringView(StringView sv)
{
	delete[] sv.Data;
}

} /* namespace pv */

/* end of file */
