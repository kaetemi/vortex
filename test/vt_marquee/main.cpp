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

// Cursed testing app
// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences

static int charLenUtf8(std::string_view str, int i)
{
	if (i < 0 || i >= str.size())
		return 0;
	unsigned char c = str[i];
	if (c < 0x80)
		return 1;
	if (c < 0xC0)
		return 0;
	if (c < 0xE0)
		return 2;
	if (c < 0xF0)
		return 3;
	if (c < 0xF8)
		return 4;
	if (c < 0xFC)
		return 5;
	if (c < 0xFE)
		return 6;
	return 0;
}

static void hsvToRgb(int &r, int &g, int &b, int h, int s, int v)
{
	int i = h / 60;
	int f = h % 60;
	int p = (v * (255 - s)) / 255;
	int q = (v * (15300 - s * f)) / 15300;
	int t = (v * (15300 - s * (60 - f))) / 15300;
	switch (i)
	{
	case 0: r = v; g = t; b = p; break;
	case 1: r = q; g = v; b = p; break;
	case 2: r = p; g = v; b = t; break;
	case 3: r = p; g = q; b = v; break;
	case 4: r = t; g = p; b = v; break;
	case 5: r = v; g = p; b = q; break;
	}
}

int main(int argc, char **argv)
{
	pv::Core core(argc, argv);

	// Set console title
	core.printF("\x1b]0;{}\x07", "<marquee>");

	constexpr std::string_view text = "    ✨.·´¯`·.·★  🦄 𝓦 𝓮𝓵𝓬𝓸𝓶 𝓮 𝓽𝓸 𝓶 𝔂 𝐜𝐨𝐧𝐬𝐨𝐥𝐞 𝐚𝐩𝐩𝐥𝐢𝐜𝐚𝐭𝐢𝐨𝐧! 🦄  ★·.·`¯´·.✨    "sv;
	constexpr std::string_view fw0 = "✨"sv;
	constexpr std::string_view fw1 = "🦄"sv;

	std::vector<std::string_view> chars;
	for (int i = 0; i < text.length();)
	{
		int cLen = charLenUtf8(text, i);
		std::string_view cView = text.substr(i, cLen);
		chars.push_back(cView);
		int cWidth = 1;
		if (cView == fw0)
			cWidth = 2;
		else if (cView == fw1)
			cWidth = 2;
		if (cWidth == 2)
			chars.push_back(""sv);
		i += cLen;
	}

	// ESC [ <n> m
	// 38	Foreground Extended
	// 38 ; 2 ; <r> ; <g> ; <b>	Set foreground color to RGB value specified in <r>, <g>, <b> parameters*
	std::vector<std::string> colors;
	for (int i = 0; i < chars.size(); ++i)
	{
		int hue = (i * 360) / chars.size();
		int r, g, b;
		hsvToRgb(r, g, b, hue, 255, 255);
		colors.push_back("\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m");
	}

	for (int i = 0; i < chars.size(); ++i)
	{
		core.printF("{}{}", colors[i], chars[i]);
	}
	core.printLf();
	core.printF("\x1b[0m");

	int i = 0;
	for (int t = 0; t < 9000; ++t)
	{
		// Up one, remove one character, move to the back, add character, back down
		core.printF("\x1b[1A\x1b[1G\x1b[0P\x1b[{}G{}{}\x1b[1E", chars.size(), colors[i], chars[i]);
		++i;
		i %= chars.size();

		// Sleep 100ms
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return EXIT_SUCCESS;
}

/* end of file */
