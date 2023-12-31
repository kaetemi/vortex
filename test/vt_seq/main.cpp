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

// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

// Tools will have dumb shit like rewriting progress bars, so we'll need to parse the terminal sequences and track which lines are in use by which process
// Since we access from thread, we'll need to provide an ongoing accessor lock to print throug printImpl rather than going through print directly
// Those terminal sequences will need to be translated to alternate the output lines properly (maybe prefix them too)

// [1] https://github.com/microsoft/terminal/blob/f5b45c25c9dfe27e03fbea1c7d82a6dc2a009343/src/types/CodepointWidthDetector.cpp
// [2] https://github.com/microsoft/terminal/blob/171a21ad48eca9f57a3ae5692fe9a5c64e9ad276/src/inc/til/unicode.h
// [3] https://unicodelookup.com/#🏳️‍🌈

// Set output mode to handle virtual terminal sequences
// Not really needed, this just causes the OS to generate input sequences in response to specific output sequences
// Similarly, ENABLE_VIRTUAL_TERMINAL_INPUT causes the Terminal to generate input sequences for things like keyboard arrows
static bool enableVTMode()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
		return false;

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
		return false;

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
		return false;

	return true;
}

int main(int argc, char **argv)
{
	pv::Core core(argc, argv);

	core.print("Test virtual terminal sequences\n");
	core.print("\n");

	if (!enableVTMode())
	{
		core.print("Failed to enable virtual terminal mode\n");
		core.print("\n");
	}

	core.print("This is a line\n");
	core.print("This is another line 日本語\n");

	// Save cursor position
	core.print("\x1b[s");

	// Now we go up and replace "This is a line" with "This is a replaced line"
	core.print("\x1b[2A\x1b[0KThis is a replaced line\n");

	// Go to the end of the line (21 works without the Unicode) (28 with - the characters have a display width of 2) // [1]
	core.print("\x1b[28C");

	// Write " (appended)"
	core.print("(appended)");

	// Restore cursor
	core.print("\x1b[u");

	core.print("\n");

	core.print("First test done\n");
	core.print("\n");

	core.print("This is a very very long line. It is very long indeed. So long it should pop right off the side of the console onto the next line. What happens with our cursor?\n");

	// Save cursor position
	core.print("\x1b[s");

	// Go up
	core.print("\x1b[1A"); // This doesn't go up to the start of the previous line, since it wrapped!

	// Replace with "That"
	core.print("That");

	// Restore cursor
	core.print("\x1b[u");

	core.print("\n");

	core.print("Second test done\n");
	core.print("\n");

	core.print("And another line 🏳️‍🌈 123 (\n"); // This flag takes multiple codepoints, but only one glyph, great // [2] [3]
	// What if we are terrible people and push this glyph as separate print calls?
	// From our POV we just pretend the character after a zero width joiner is zero width itself, the effect is the same

	// Nevermind, Terminal just calculates width per codepoint and slaps the emoji in the middle.
	// Old command line prints the codepoints separately but the codepoints widths are mismatched

	// New text rendering engine of new Terminal draws the emoji at the start of the space

	// Save cursor position
	core.print("\x1b[s");

	// Go up
	core.print("\x1b[1A");

	// Go to end of line
	core.print("\x1b[28C"); // core.print("\x1b[20C"); // core.print("\x1b[17C");

	// Write "appended)"
	core.print("appended)");

	// Restore cursor
	core.print("\x1b[u");
	core.print("This works in Terminal. In Command Line the first bracket will be missing.\n");

	core.print("\n");

	core.print("Third test done\n");
	core.print("\n");

	core.print("This is another very very long line. It is very long indeed. So long it should pop right off the side of the console onto the next line. What happens with our cursor?\n");

	// Save cursor position
	core.print("\x1b[s");

	// Up one line
	core.print("\x1b[1F"); // Also does not go to the start, F does the same as the other one, but returns to start of line

	// Replace with "That"
	core.print("That");
	// core.print("\x1b[100@"); // This inserts spaces and removes text that overflows

	// Restore cursor
	core.print("\x1b[u");

	core.print("\n");

	core.print("Fourth test done\n");
	core.print("\n");

	return EXIT_SUCCESS;
}

/* end of file */
