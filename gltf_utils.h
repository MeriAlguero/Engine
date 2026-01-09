#pragma once


//Disableing tintgltf's built in image loading

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE

#pragma warning(push)
#pragma warning(disable : 4018)  // Signed/unsigned mismatch
#pragma warning(disable : 4267)  // Size_t to int conversion

#include "tiny_gltf.h"

#pragma warning(pop) 

inline bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int index)
{
	// Get the accessor (describes the data format and location)
	const tinygltf::Accessor& accessor = model.accessors[index];
	//Calculates the natural stride (element size) based on the accessor's type
	size_t defaultStride = tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

	if (elemCount == accessor.count && defaultStride == elemSize)
	{
		//Getting buffer view
		const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];

		//Calculate pointer to start od data in gltf buffer
		const uint8_t* bufferData = reinterpret_cast<const uint8_t*>( &(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
		//If it haves custom stride of it haves to use natural stride
		size_t bufferStride = view.byteStride == 0 ? defaultStride : view.byteStride;

		for (uint32_t i = 0; i < elemCount; ++i)
		{
			memcpy(data, bufferData, elemSize); //Copy one element from source to destination
			data += stride;
			bufferData += bufferStride;
		}
		return true;

	}
	return false; //Validation failed
}

//Looks up an accesor by name and loads its data. Used for vertex attributes
inline bool loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int>& attributes, const char* accesorName)
{
	const auto& it = attributes.find(accesorName);
	if (it != attributes.end())
	{
		
		return loadAccessorData(data, elemSize, stride, elemCount, model, it->second);
	}
	return false;

}

template <class T>//Template version: Loads data into a typed array
//Automatically allocates memory for the data
inline bool loadAccessorTyped(std::unique_ptr<T[]>& data, UINT& count,
	const tinygltf::Model& model, int index)
{
	const tinygltf::Accessor& accessor = model.accessors[index];

	// Get number of elements from the accessor
	count = UINT(accessor.count);

	// Allocate memory for that many elements of type T
	data = std::make_unique<T[]>(count);

	return loadAccessorData(reinterpret_cast<uint8_t*>(data.get()),
		sizeof(T), sizeof(T), count, model, index);
}

template <class T> //template version that looks up accessor by name.
inline bool loadAccessorTyped(std::unique_ptr<T[]>& data, UINT& count, const tinygltf::Model& model, const std::map<std::string, int>& attributes, const char* accesorName)
{
	// Look for the attribute by name
	const auto& it = attributes.find(accesorName);
	if (it != attributes.end())
	{
		//Call indexed version
		return loadAccessorTyped(data, count, model, it->second);
	}

	
	count = 0;  // Set count to 0 to indicate no data if not found
	return false;
}