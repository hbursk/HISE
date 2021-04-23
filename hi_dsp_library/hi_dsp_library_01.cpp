/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/* HI Module */


#include "AppConfig.h"

// Import the files here when not building a library (see comment in hi_dsp_library.h)
#if HI_EXPORT_DSP_LIBRARY
#else
#include "hi_dsp_library.h"

#include "dywapitchtrack/dywapitchtrack.c"

#include "dsp_library/DspBaseModule.cpp"



#include "snex_basics/snex_Types.cpp"
#include "snex_basics/snex_ArrayTypes.cpp"
#include "snex_basics/snex_Math.cpp"


#include "snex_basics/snex_DynamicType.cpp"
#include "snex_basics/snex_TypeHelpers.cpp"

#include "snex_basics/snex_ExternalData.cpp"
#include "node_api/helpers/ParameterData.cpp"
#include "node_api/nodes/Base.cpp"
#include "node_api/nodes/OpaqueNode.cpp"
#include "node_api/nodes/prototypes.cpp"
#include "node_api/nodes/duplicate.cpp"

#include "dsp_basics/AllpassDelay.cpp"
#include "dsp_basics/Oscillators.cpp"
#include "dsp_basics/MultiChannelFilters.cpp"

#include "unit_test/wrapper_tests.cpp"
#include "unit_test/node_tests.cpp"
#include "unit_test/container_tests.cpp"

namespace hise
{
	using namespace juce;

	size_t HelperFunctions::writeString(char* location, const char* content)
	{
		strcpy(location, content);
		return strlen(content);
	}

	juce::String HelperFunctions::createStringFromChar(const char* charFromOtherHeap, size_t length)
	{
		return juce::String(charFromOtherHeap, length);
	}


}



#endif


