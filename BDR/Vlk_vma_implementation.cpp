// disable some VMA warnings
#pragma warning( disable: 4100 4127 4324 4189 )

#ifdef _DEBUG
#define VMA_DEBUG_MARGIN 16
#define VMA_DEBUG_DETECT_CORRUPTION 1
#endif
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>