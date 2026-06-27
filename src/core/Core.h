#pragma once

// Все абстракции ядра в одном месте.
// Подключай этот файл вместо отдельных заголовков ядра.

#include "interfaces/IService.h"
#include "interfaces/ILogger.h"
#include "ServiceManager.h"
#include "types/Result.h"
#include "types/Types.h"
#include "types/Version.h"
#include "utils/NonCopyable.h"
#include "utils/NonMovable.h"