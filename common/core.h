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

#ifdef WIN32
	PV_FORCE_INLINE HINSTANCE executableModule()
	{
		return m_ExecutableModule;
	}
	PV_FORCE_INLINE HICON executableIcon()
	{
		return m_ExecutableIcon;
	}
#endif

private:
	int m_ArgC;
	char **m_ArgV;

#ifdef WIN32
	HINSTANCE m_ExecutableModule;
	HICON m_ExecutableIcon;
#endif
};

} /* namespace pv */

#endif /* #ifndef PV_CORE_H */

/* end of file */
