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
	pv::Core core(argc, argv);

	// This tests how the command line wraps long texts when full-width characters are unaligned
	constexpr std::string_view hiragana = "あいうえおかきくけこさしすせそたちつてとなにぬねの"sv;
	constexpr std::string_view katakana = "アイウエオカキクケコサシスセソタチツテトナニヌネノ"sv;
	constexpr std::string_view kanji = "一二三四五六七八九十百千万億兆"sv;

	// EASY! The terminal inserts a blank character where it got auto-wrapped.
	// This means: we don't need to track any complete strings!
	// -> All we need to do is count how many characters each virtual row has.

	core.printF("a{}{}{}\n\n"sv, hiragana, katakana, kanji);
	core.printF("bb{}{}{}\n\n"sv, hiragana, katakana, kanji);

	return EXIT_SUCCESS;
}

/* end of file */
