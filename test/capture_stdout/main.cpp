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
#include "win32_exception.h"

#if (NTDDI_VERSION < 0x0A000006)

#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016

// CreatePseudoConsole Flags
#define PSEUDOCONSOLE_INHERIT_CURSOR (0x1)

typedef HRESULT(WINAPI *FCreatePseudoConsole)(_In_ COORD size, _In_ HANDLE hInput, _In_ HANDLE hOutput, _In_ DWORD dwFlags, _Out_ HPCON *phPC);
typedef HRESULT(WINAPI *FResizePseudoConsole)(_In_ HPCON hPC, _In_ COORD size);
typedef VOID(WINAPI *FClosePseudoConsole)(_In_ HPCON hPC);
FCreatePseudoConsole CreatePseudoConsole;
FResizePseudoConsole ResizePseudoConsole;
FClosePseudoConsole ClosePseudoConsole;

bool loadConPTY()
{
	// Load the ConPTY functions
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	if (!hKernel32)
		return false;
	CreatePseudoConsole = (FCreatePseudoConsole)GetProcAddress(hKernel32, "CreatePseudoConsole");
	ResizePseudoConsole = (FResizePseudoConsole)GetProcAddress(hKernel32, "ResizePseudoConsole");
	ClosePseudoConsole = (FClosePseudoConsole)GetProcAddress(hKernel32, "ClosePseudoConsole");
	if (!CreatePseudoConsole || !ResizePseudoConsole || !ClosePseudoConsole)
		return false;
	return true;
}

#endif

void captureStdout(pv::Core &core, std::string_view exe) // TODO: Enable conversion option
{
	// Regular CreateProcess and capturing stdout and stderr
	// Not using ConPTY
	// This is the old way that works on all systems
	std::wstring exeW = pv::utf8ToWide(exe.data(), exe.length());

	// Create pipes for stdout and stderr
	HANDLE stdoutRead, stdoutWrite;
	HANDLE stderrRead, stderrWrite;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	bool writeClosed = false, readClosed = false;
	PV_THROW_LAST_ERROR_IF(!CreatePipe(&stdoutRead, &stdoutWrite, &sa, 0));
	PV_FINALLY([&]() { if (!writeClosed) CloseHandle(stdoutWrite); if (!readClosed) CloseHandle(stdoutRead); });
	PV_THROW_LAST_ERROR_IF(!CreatePipe(&stderrRead, &stderrWrite, &sa, 0));
	PV_FINALLY([&]() { if (!writeClosed) CloseHandle(stderrWrite); if (!readClosed) CloseHandle(stderrRead); });

	// Create process
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdOutput = stdoutWrite;
	si.hStdError = stderrWrite;
	PV_THROW_LAST_ERROR_IF(!CreateProcessW(exeW.c_str(), NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi));
	PV_FINALLY([&]() { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); });

	// Close the write end of the pipes
	CloseHandle(stdoutWrite);
	CloseHandle(stderrWrite);
	writeClosed = true;

	// Read stdout and stderr
	std::string stdoutStr;
	std::string stderrStr;
	char buf[4096];
	DWORD bytesRead;
	for (;;)
	{
		if (!ReadFile(stdoutRead, buf, sizeof(buf), &bytesRead, NULL))
			break;
		if (bytesRead == 0)
			break;
		stdoutStr.append(buf, bytesRead);
	}
	for (;;)
	{
		if (!ReadFile(stderrRead, buf, sizeof(buf), &bytesRead, NULL))
			break;
		if (bytesRead == 0)
			break;
		stderrStr.append(buf, bytesRead);
	}

	// Close the read end of the pipes
	CloseHandle(stdoutRead);
	CloseHandle(stderrRead);
	readClosed = true;

	// Wait for process to exit
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Print stdout and stderr
	core.printF("stdout: {}\n", stdoutStr);
	core.printF("stderr: {}\n", stderrStr);

	// Print exit code
	DWORD exitCode;
	if (GetExitCodeProcess(pi.hProcess, &exitCode))
		core.printF("exit code: {}\n", exitCode);
	else
		core.printF("exit code: unknown\n");
}

void captureConPTY(pv::Core &core, std::string_view exe)
{
	// Same but through the new ConPTY API and CreateProcess
	// https://learn.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session
	std::wstring exeW = pv::utf8ToWide(exe.data(), exe.length());

	// Create pipes for input and output
	HANDLE inputRead, outputWrite; // Child side
	HANDLE outputRead, inputWrite; // Parent side
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	bool childSideClosed = false;
	PV_THROW_LAST_ERROR_IF(!CreatePipe(&inputRead, &inputWrite, &sa, 0));
	PV_FINALLY([&]() { if (!childSideClosed) CloseHandle(inputRead); CloseHandle(inputWrite); });
	PV_THROW_LAST_ERROR_IF(!CreatePipe(&outputRead, &outputWrite, &sa, 0));
	PV_FINALLY([&]() { CloseHandle(outputRead); if (!childSideClosed) CloseHandle(outputWrite); });

	// Create pseudoconsole
	HPCON hPC;
	COORD size;
	size.X = 80;
	size.Y = 25;
	bool pseudoconsoleClosed = false;
	PV_THROW_IF_HRESULT(CreatePseudoConsole(size, inputRead, outputWrite, 0, &hPC));
	PV_FINALLY([&]() { if (!pseudoconsoleClosed) ClosePseudoConsole(hPC); });

	// Prepare Startup Information structure
	STARTUPINFOEXW siEx;
	ZeroMemory(&siEx, sizeof(siEx));
	siEx.StartupInfo.cb = sizeof(STARTUPINFOEXW);

	// Discover the size required for the list
	size_t bytesRequired = 0;
	(void)InitializeProcThreadAttributeList(NULL, 1, 0, &bytesRequired);
	if (!bytesRequired)
		PV_THROW_HRESULT(E_UNEXPECTED);

	// Allocate memory to represent the list
	siEx.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, bytesRequired);
	if (!siEx.lpAttributeList)
		PV_THROW_HRESULT(E_OUTOFMEMORY);
	PV_FINALLY([&]() { HeapFree(GetProcessHeap(), 0, siEx.lpAttributeList); });

	// Initialize the list memory location
	PV_THROW_LAST_ERROR_IF(!InitializeProcThreadAttributeList(siEx.lpAttributeList, 1, 0, &bytesRequired));
	PV_FINALLY([&]() { DeleteProcThreadAttributeList(siEx.lpAttributeList); });

	// Set the pseudoconsole information into the list
	PV_THROW_LAST_ERROR_IF(!UpdateProcThreadAttribute(siEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hPC, sizeof(hPC), NULL, NULL));

	// Create process
	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));
	PV_THROW_LAST_ERROR_IF(!CreateProcessW(exeW.c_str(), NULL, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &siEx.StartupInfo, &pi));
	PV_FINALLY([&]() { CloseHandle(pi.hProcess); CloseHandle(pi.hThread); });

	// Close the child side of the pipes
	CloseHandle(inputRead);
	CloseHandle(outputWrite);
	childSideClosed = true;

	// Wait for process to exit
	WaitForSingleObject(pi.hProcess, INFINITE);
	ClosePseudoConsole(hPC);
	pseudoconsoleClosed = true;

	// Read output
	std::string outputStr;
	char buf[4096];
	DWORD bytesRead;
	for (;;)
	{
		if (!ReadFile(outputRead, buf, sizeof(buf), &bytesRead, NULL))
			break;
		if (bytesRead == 0)
			break;
		outputStr.append(buf, bytesRead);
	}

	// Print output and stderr
	core.printF("output: {}\n", outputStr);

	// Print exit code
	DWORD exitCode;
	if (GetExitCodeProcess(pi.hProcess, &exitCode))
		core.printF("exit code: {}\n", exitCode);
	else
		core.printF("exit code: unknown\n");
}

int main(int argc, char **argv)
{
	pv::Core core(argc, argv);

	// We need to implement both ConPTY and direct stdout capture support
	// Because some tools do not set the console codepage correctly, they output garbage Unicode on the console
	// Even though their stdout is in at least some codepage (it has to be, of course), which we can convert to UTF-8 ourselves if we know it

	// Set working dir to exe dir
	// Get exe path
	wchar_t exePath[MAX_PATH];
	DWORD res = GetModuleFileNameW(NULL, exePath, MAX_PATH);
	PV_THROW_LAST_ERROR_IF(!res || res == MAX_PATH);
	// Find the last backslash - the start of the filename
	wchar_t *lastBackslash = wcsrchr(exePath, L'\\');
	if (lastBackslash)
		*lastBackslash = L'\0'; // Truncate the path to remove the filename
	else
		throw std::runtime_error("Failed to find the directory of the executable");
	// Set working dir
	PV_THROW_LAST_ERROR_IF(!SetCurrentDirectoryW(exePath));

	// Test basic stdout capture
	core.printLf("The following two should work:");
	captureStdout(core, "test_utf8_on.exe"); // This should give UTF-8 output, because it's direct clean binary UTF-8 output, no conversions
	captureStdout(core, "test_utf8_off.exe"); // This also should give UTF-8 output, because we push UTF-16 (wide Unicode) into the _O_U8TEXT stream
	core.printLf("");
	core.printLf("The following two should not work (Japanese characters will be missing):");
	core.printLf("Invalid UTF-8 characters (like NUL for badly interpreted UTF-16) will simply be skipped by the console");
	captureStdout(core, "test_stdout_932.exe"); // This will give Shift-JIS output on stdout because we push it binary, but console is also set to 932 so ConPTY should convert it correctly to UTF-8
	captureStdout(core, "test_stdout_utf16.exe"); // Here we push UTF-16 into a _O_U16TEXT stream, so this should be UTF-16 on stdout, and UTF-8 on ConPTY
	core.printLf("");
	core.printLf("");

	if (!loadConPTY())
	{
		core.printLf("No ConPTY support");
		return EXIT_SUCCESS;
		;
	}
	core.printLf("ConPTY loaded");
	core.printLf("");
	core.printLf("The following four should all work:");
	captureConPTY(core, "test_utf8_on.exe");
	captureConPTY(core, "test_utf8_off.exe");
	captureConPTY(core, "test_stdout_932.exe");
	captureConPTY(core, "test_stdout_utf16.exe");
	core.printLf("");
	core.printLf("");

	return EXIT_SUCCESS;
	;
}

/* end of file */
