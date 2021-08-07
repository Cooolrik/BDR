
#include "Vlk_Common.inl"

#include "Vlk_ShaderModule.h"

#include <fstream>

using std::ifstream;
using std::ios;

Vlk::ShaderModule* Vlk::ShaderModule::CreateFromFile(
	VkShaderStageFlagBits shaderStage,
	const char* shaderFilepath,
	const char* entrypoint,
	const char* shaderName
	)
	{
	ShaderModule* pThis = new ShaderModule();

	pThis->Stage = shaderStage;
	if( shaderName != nullptr )
		pThis->Name = _strdup( shaderName );
	else
		pThis->Name = _strdup( shaderFilepath );
	pThis->Entrypoint = _strdup( entrypoint );

	// read in file
	ifstream file( shaderFilepath, ios::ate | ios::binary );
	if( !file.is_open() )
		{
		string err = string( "Error: CreateFromFile, could not read file: " ) + shaderFilepath;
		throw runtime_error( err.c_str() );
		}
	size_t fileSize = (size_t)file.tellg();
	pThis->Shader.resize( fileSize );
	file.seekg( 0 );
	file.read( pThis->Shader.data(), fileSize );
	file.close();

	return pThis;
	}


Vlk::ShaderModule::~ShaderModule()
	{
	free( this->Name );
	free( this->Entrypoint );
	}