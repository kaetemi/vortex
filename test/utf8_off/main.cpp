/*

Copyright (C) 2023  Jan BOON (Kaetemi) <jan.boon@kaetemi.be>
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

#include "platform.h"
#include "core.h"
#include "string_ex.h"

int main(int argc, char **argv)
{
	UINT origACP = GetACP();
	const wchar_t *origLocaleP = _wsetlocale(LC_ALL, null);
	std::wstring origLocale = origLocaleP ? origLocaleP : L"<null>";
	UINT consoleOutputCP = GetConsoleOutputCP();
	UINT consoleCP = GetConsoleCP();

#if 0
	printf("origACP: %u\n", origACP);
	printf("origLocale: %ls\n", origLocale);
	printf("consoleOutputCP: %u\n", consoleOutputCP);
	printf("consoleCP: %u\n", consoleCP);

	/*
	origACP: 932
	origLocale: C
	consoleOutputCP: 65001
	consoleCP: 65001
	*/

#else

	pv::Core core(argc, argv);
	
	{
		pv::PrintContainer pc(core);
		std::format_to(std::back_inserter(pc), "test {}\n", "ok");
		std::format_to(std::back_inserter(pc), "test {}\n", std::string("ok"));
	}
	core.printF("test {}\n", "ok");
	core.printF("test {}\n", std::string("ok")); // compiler vomit

	UINT newACP = GetACP();
	const wchar_t *newLocaleP = _wsetlocale(LC_ALL, null);
	std::wstring newLocale = newLocaleP ? newLocaleP : L"<null>";
	UINT newConsoleOutputCP = GetConsoleOutputCP();
	UINT newConsoleCP = GetConsoleCP();

	core.printF("origACP: {}\n"sv, origACP);
	core.printF("origLocale: {}\n"sv, pv::wideToUtf8(origLocale));
	core.printF("consoleOutputCP: {}\n"sv, consoleOutputCP);
	core.printF("consoleCP: {}\n"sv, consoleCP);

	core.printLf(""sv);

	core.printF("newACP: {}\n"sv, newACP);
	core.printF("newLocale: {}\n"sv, pv::wideToUtf8(newLocale));
	core.printF("newConsoleOutputCP: {}\n"sv, newConsoleOutputCP);
	core.printF("newConsoleCP: {}\n"sv, newConsoleCP);

	core.printLf(""sv);

	if (core.isUtf8Clean())
		core.printLf("UTF-8 clean"sv);
	else
		core.printLf("Not UTF-8 clean"sv);

	core.printLf("日本語"sv);

	/*
	origACP: 932
	consoleOutputCP: 65001
	consoleCP: 65001

	newACP: 932
	newConsoleOutputCP: 65001
	newConsoleCP: 65001

	Not UTF-8 clean
	日本語
	*/

#endif
}

/* end of file */
