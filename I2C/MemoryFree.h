#include "Arduino.h"

#ifndef	MEMORY_FREE_H
#define MEMORY_FREE_H

#ifdef __cplusplus
extern "C" {
#endif

int freeMemory();

int availableMemory();

#ifdef  __cplusplus
}
#endif

#endif
