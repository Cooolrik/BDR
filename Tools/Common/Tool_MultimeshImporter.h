#pragma once

#include <vector>
#include "Tool_Multimesh.h"

namespace Tools
	{
	class MultimeshImporter
		{
		public:
			// build from .obj file, store allocated object in *dest
			void buildFromWavefrontFile( const char* path, unsigned int max_tris, unsigned int max_verts, Multimesh **dest );
		};
	};
