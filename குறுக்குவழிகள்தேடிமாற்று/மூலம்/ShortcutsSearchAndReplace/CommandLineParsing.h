#pragma once

#include <Windows.h>
#include "SearchInfos.h"

BOOL ParseCommandLine(OUT BOOL* pbHasParams,OUT CCommandLineOptions* pCommandLineOptions,OUT SEARCH_INFOS* pSearchInfos, OUT COptions* pOptions);
