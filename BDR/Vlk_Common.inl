#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string>
#include "Vlk_Renderer.h"

// makes sure the return value is VK_SUCCESS or throws an exception
#define VLK_CALL( s ) { VkResult VLK_CALL_res = s; if( VLK_CALL_res != VK_SUCCESS ) { throw_vulkan_error( VLK_CALL_res , "Vulcan call " #s " failed (did not return VK_SUCCESS)"); } }

// makes a new unique_ptr object with this class as owner
#define BD_NEW_UNIQUE_PTR( _class ) std::unique_ptr<_class>( new _class(this) )

using Vlk::uint;
using std::vector;
using std::runtime_error;
using std::min;
using std::max;
using std::string;
using std::unique_ptr;

#ifdef VR_LESS_STRICT_WARNINGS
#pragma warning( disable: 4100 )
#endif

// dont warn about throw_vulkan_error local functions removed, if not used in a file
#pragma warning( push )
#pragma warning( disable : 4505 )
static void throw_vulkan_error( VkResult errorvalue, const char* errorstr )
	{
	char str[20];
	sprintf_s( str, "%d", (int)errorvalue );
	throw runtime_error( errorstr );
	}
#pragma warning( pop )

#define VR_GET_EXTENSION_FUNCTION_ADDRESS( func ) func = (PFN_##func)vkGetInstanceProcAddr( this->Parent->GetInstance(), #func ); if( !func ) { throw std::runtime_error( "vkGetInstanceProcAddr failed, could not retreive address for " #func ); }

#define VR_ADD_STRUCT_TO_VULKAN_LINKED_LIST( plist , strct , stype )\
	strct = {};\
	strct.sType = stype;\
	strct.pNext = (void*)plist->pNext;\
	plist->pNext = &strct;
	