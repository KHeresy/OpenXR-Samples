#include <iostream>
#include <vector>
#include <sstream>

#define XR_USE_GRAPHICS_API_OPENGL

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#pragma comment( lib, "openxr_loader.lib" )

// OpenXR instance
XrInstance gInstance;

#pragma region helper function for output
bool xrWORK(const XrResult& rs)
{
	if (rs == XR_SUCCESS)
		return true;

	char sMsg[XR_MAX_RESULT_STRING_SIZE];
	if(xrResultToString(gInstance, rs, sMsg) == XR_SUCCESS)
		std::cout << "   => Error: " << sMsg << std::endl;
	else
		std::cout << "   => Error: " << rs << std::endl;
	return false;
}

std::string toString(const XrVersion& rVer)
{
	return	std::to_string(XR_VERSION_MAJOR(rVer)) + "." +
			std::to_string(XR_VERSION_MINOR(rVer)) + "." +
			std::to_string(XR_VERSION_PATCH(rVer));
}

std::string toString(const XrViewConfigurationType& rViewConfType)
{
	switch (rViewConfType)
	{
	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:
		return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO";
		
	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:
		return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO";

	case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:
		return "XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO";

	case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT:
		return "XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT";
	}

	return "Unknown Type";
}

std::string toString(const XrEnvironmentBlendMode& rMode)
{
	switch (rMode)
	{
	case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:
		return "XR_ENVIRONMENT_BLEND_MODE_OPAQUE";

	case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:
		return "XR_ENVIRONMENT_BLEND_MODE_ADDITIVE";

	case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:
		return "XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND";
	}

	return "Unknown Mode";
}

std::ostream& operator<<(std::ostream& oss, const XrApiLayerProperties& rAPI)
{
	oss << rAPI.layerName << " (ver " << toString(rAPI.specVersion) << "/" << rAPI.layerVersion << "): " << rAPI.description;
	return oss;
}

std::ostream& operator<<(std::ostream& oss, const XrExtensionProperties& rExt)
{
	oss << rExt.extensionName << " (ver " << rExt.extensionVersion << ")";
	return oss;
}

std::ostream& operator<<(std::ostream& oss, const XrInstanceProperties& rProp)
{
	oss << rProp.runtimeName << " (" << toString(rProp.runtimeVersion) << ")";
	return oss;
}

std::ostream& operator<<(std::ostream& oss, const XrViewConfigurationView& rView)
{
	oss << rView.recommendedImageRectWidth << " * " << rView.recommendedImageRectHeight
		<< " (MAX: " << rView.maxImageRectWidth << " * " << rView.maxImageRectHeight << ")"
		<< " [Swap chain: " << rView.recommendedSwapchainSampleCount << "(" << rView.maxSwapchainSampleCount << ")]";
	return oss;
}
#pragma endregion

int main(int argc, char** argv)
{
	#pragma region API Layers information
	{
		std::cout << "Try to get API Layers: " << std::endl;

		// Get Numbers of API layers
		uint32_t uAPILayerNum = 0;
		if (xrWORK(xrEnumerateApiLayerProperties(0, &uAPILayerNum, nullptr)))
		{
			std::cout << " > Found " << uAPILayerNum << " API layers\n";
			if (uAPILayerNum > 0)
			{
				// enumrate and output API layer information
				std::vector<XrApiLayerProperties> vAPIs(uAPILayerNum, { XR_TYPE_API_LAYER_PROPERTIES });
				if (xrWORK(xrEnumerateApiLayerProperties(uAPILayerNum, &uAPILayerNum, vAPIs.data())))
				{
					for (const auto& rAPI : vAPIs)
						std::cout << "   - " << rAPI << "\n";
				}
			}
		}
		std::cout << "\n";
	}
	#pragma endregion

	#pragma region Extensions information
	std::vector<XrExtensionProperties> vSupportedExt;
	{
		std::cout << "Try to get supported entensions:" << std::endl;

		// get numbers of supported extensions
		uint32_t uExtensionNum = 0;
		if (!xrWORK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &uExtensionNum, nullptr)))
			return -1;

		std::cout << " > Found " << uExtensionNum << " extensions\n";
		if (uExtensionNum > 0)
		{
			// enumrate and output extension information
			std::vector<XrExtensionProperties> vExternsions(uExtensionNum, { XR_TYPE_EXTENSION_PROPERTIES });
			if (xrWORK(xrEnumerateInstanceExtensionProperties(nullptr, uExtensionNum, &uExtensionNum, vExternsions.data())))
			{
				for (const auto& rExt : vExternsions)
					std::cout << "  - " << rExt << "\n";
			}
		}
		std::cout << "\n";
	}
	#pragma endregion

	#pragma region OpenXR instance
	std::cout << "Create OpenXR instance" << std::endl;
	{
		std::cout << " > prepare instance create information" << std::endl;

		// setup required extensions
		std::vector<const char*> sExtensionList;
		//sExtensionList.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
		sExtensionList.push_back("XR_KHR_D3D11_enable");

		XrInstanceCreateInfo infoCreate;
		infoCreate.type = XR_TYPE_INSTANCE_CREATE_INFO;
		infoCreate.next = nullptr;
		infoCreate.createFlags = 0;
		infoCreate.applicationInfo = { "TestApp", 1, "TestEngine", 1, XR_CURRENT_API_VERSION };
		infoCreate.enabledApiLayerCount = 0;
		infoCreate.enabledApiLayerNames = {};
		infoCreate.enabledExtensionCount = (uint32_t)sExtensionList.size();
		infoCreate.enabledExtensionNames = sExtensionList.data();

		std::cout << " > create instance" << std::endl;
		if (!xrWORK(xrCreateInstance(&infoCreate, &gInstance)))
			return -1;

		std::cout << " > get instance information" << std::endl;
		XrInstanceProperties mProp;
		if (xrWORK(xrGetInstanceProperties(gInstance, &mProp)))
			std::cout << "   - " << mProp << "\n";

		std::cout << "\n";
	}
	#pragma endregion

	#pragma region OpenXR system
	std::vector<XrSystemId> vSysId;
	{
		std::cout << "Get systems" << std::endl;

		std::vector<XrSystemGetInfo> vSysGet = {
			{XR_TYPE_SYSTEM_GET_INFO, nullptr,XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY},
			{XR_TYPE_SYSTEM_GET_INFO, nullptr,XR_FORM_FACTOR_HANDHELD_DISPLAY}
		};

		for (const auto& rGetInfo : vSysGet)
		{
			std::cout << " > Try to get system " << rGetInfo.formFactor << "\n";

			XrSystemId mSysId = XR_NULL_SYSTEM_ID;
			if (xrWORK(xrGetSystem(gInstance, &rGetInfo, &mSysId)))
			{
				vSysId.push_back(mSysId);
				std::cout << "   > get system information" << std::endl;
				XrSystemProperties mSysProp;
				if (xrWORK(xrGetSystemProperties(gInstance, mSysId, &mSysProp)))
				{
					std::cout << "    - " << mSysProp.systemName << " (" << mSysProp.vendorId << ")\n";
					std::cout << "     - Graphics: " << mSysProp.graphicsProperties.maxSwapchainImageWidth << " * " << mSysProp.graphicsProperties.maxSwapchainImageHeight << " with " << mSysProp.graphicsProperties.maxLayerCount << " layer\n";
					std::cout << "     - Tracking:";
					if (mSysProp.trackingProperties.positionTracking == XR_TRUE)
						std::cout << " position";
					if (mSysProp.trackingProperties.orientationTracking == XR_TRUE)
						std::cout << " orientation";
					std::cout << "\n";
				}
			}
		}

		std::cout << "\n";
	}
	#pragma endregion

	#pragma region OpenXR View
	for(const auto& mSysId : vSysId )
	{
		std::cout << "Enumerate View Configurations for system " << mSysId << std::endl;
		uint32_t uViewConfNum = 0;
		if (xrWORK(xrEnumerateViewConfigurations(gInstance, mSysId, 0, &uViewConfNum, nullptr )))
		{
			if(uViewConfNum > 0)
			{
				std::vector<XrViewConfigurationType> vViewConf(uViewConfNum);
				if (xrWORK(xrEnumerateViewConfigurations(gInstance, mSysId, uViewConfNum, &uViewConfNum, vViewConf.data())))
				{
					for (const auto& rViewConfType : vViewConf)
					{
						std::cout << "  - " << toString(rViewConfType);

						// get view configuration properties
						XrViewConfigurationProperties mProp;
						if (xrWORK(xrGetViewConfigurationProperties(gInstance, mSysId, rViewConfType, &mProp)))
						{
							if (mProp.fovMutable)
								std::cout << " ( FoV mutable)";
						}
						std::cout << "\n";

						// Views
						uint32_t uViewsNum = 0;
						if (xrWORK(xrEnumerateViewConfigurationViews(gInstance, mSysId, rViewConfType, 0, &uViewsNum, nullptr)))
						{
							if (uViewsNum > 0)
							{
								std::cout << "   > " << uViewsNum << " views:\n";
								std::vector<XrViewConfigurationView> vViews(uViewsNum, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
								if (xrWORK(xrEnumerateViewConfigurationViews(gInstance, mSysId, rViewConfType, uViewsNum, &uViewsNum, vViews.data())))
								{
									for (const auto& rView : vViews)
										std::cout << "     - " << rView << "\n";
								}
							}
						}

						// Blend Type
						uint32_t uBlendModeNum = 0;
						if (xrWORK(xrEnumerateEnvironmentBlendModes(gInstance, mSysId, rViewConfType, 0, &uBlendModeNum, nullptr)))
						{
							if (uBlendModeNum > 0)
							{
								std::cout << "   > " << uBlendModeNum << " blend modes:\n";
								std::vector<XrEnvironmentBlendMode> vBlendMode(uBlendModeNum);
								if (xrWORK(xrEnumerateEnvironmentBlendModes(gInstance, mSysId, rViewConfType, uBlendModeNum, &uBlendModeNum, vBlendMode.data())))
								{
									for (const auto& rMode : vBlendMode)
										std::cout << "     - " << toString(rMode) << "\n";
								}
							}
						}
					}
				}
			}
		}
		std::cout << "\n";
	}
	#pragma endregion

	#pragma region Destroy Instance
	xrWORK(xrDestroyInstance(gInstance));
	#pragma endregion
	return 0;
}
