#include "Vlk_Common.inl"
#include "Vlk_Sampler.h"
#include "Vlk_Buffer.h"

Vlk::Sampler::~Sampler()
	{
	if( this->SamplerHandle != nullptr )
		{
		vkDestroySampler( this->Owner->GetDevice(), this->SamplerHandle, nullptr );
		}
	}

///////////////////////////////////////////

Vlk::SamplerTemplate Vlk::SamplerTemplate::Linear()
	{
	SamplerTemplate ret;

	ret.SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	ret.SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	ret.SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	ret.SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ret.SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ret.SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ret.SamplerCreateInfo.anisotropyEnable = VK_TRUE;
	ret.SamplerCreateInfo.maxAnisotropy = 16;
	ret.SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	ret.SamplerCreateInfo.compareEnable = VK_FALSE;
	ret.SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	ret.SamplerCreateInfo.mipLodBias = 0.0f;
	ret.SamplerCreateInfo.minLod = 0.f;
	ret.SamplerCreateInfo.maxLod = 16.f;

	return ret;
	}

Vlk::SamplerTemplate Vlk::SamplerTemplate::DepthReduce( VkSamplerReductionMode reductionMode )
	{
	SamplerTemplate ret;

	ret.SamplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	ret.SamplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	ret.SamplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	ret.SamplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	ret.SamplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	ret.SamplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	ret.SamplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	ret.SamplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	ret.SamplerCreateInfo.compareEnable = VK_FALSE;
	ret.SamplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	ret.SamplerCreateInfo.minLod = 0.f;
	ret.SamplerCreateInfo.maxLod = 16.f;

	if(reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE)
		{
		ret.UseSamplerReductionModeCreateInfo = true;
		ret.SamplerReductionModeCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };
		ret.SamplerReductionModeCreateInfo.reductionMode = reductionMode;
		}

	return ret;
	}
