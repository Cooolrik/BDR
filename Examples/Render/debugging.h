#pragma once

// more free form file that has debugging stuff in it. not part of main code

#include "../Common/Common.h"

#include <Vlk_CommandPool.h>

class RenderData;
class PerFrameData;

extern void debugSetup( RenderData* _renderData );

extern void debugRecreateSwapChain();

extern void debugDrawPreFrame( PerFrameData* currentFrame , PerFrameData* previousFrame );
extern void debugDrawPostFrame();

extern void debugDrawGraphicsPipeline( Vlk::CommandPool* pool );

extern void debugCommandBuffer( Vlk::CommandPool* pool, VkCommandBuffer buffer );

extern void debugPerFrameLoop();

extern void debugCleanup();