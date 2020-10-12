#pragma once

// Windows Header
#include <Windows.h>

// OpenXR
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_WIN32
#include <openxr/openxr_platform.h>
#include <openxr/openxr.h>
#pragma comment( lib, "openxr_loader.lib" )

#include <GL/glew.h>

// STD Header
#include <iostream>
#include <array>
#include <vector>

class COpenXRGL
{
public:
	using TMatrix = std::array<float, 16>;

public:
	COpenXRGL()
	{
		enumerateApiLayers();
		enumerateExtensions();
	}

	bool init()
	{
		useExtension(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
		if (createInstance() &&
			getSystem() &&
			createSession() &&
			createReferenceSpace() &&
			checkViewConfiguration() &&
			createSwapChain() &&
			prepareCompositionLayer())
		{
			return createFrameBubber();
		}
		return false;
	}

	void release()
	{
		for (auto& rVData : m_vViewDatas)
			glDeleteFramebuffers(1, &rVData.m_glFrameBuffer);

		check(xrDestroyInstance(m_xrInstance), "xrDestroyInstance");
	}

	bool useExtension(const char* sExtName)
	{
		for (const auto& rExt : m_vSupportedExtensions)
			if (strcmp(rExt.extensionName, sExtName) == 0)
			{
				m_vRequiredExtensions.push_back(sExtName);
				return true;
			}

		return false;
	}

	bool beginSession()
	{
		XrSessionBeginInfo sbi{ XR_TYPE_SESSION_BEGIN_INFO, nullptr, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
		return check(xrBeginSession(m_xrSession, &sbi), "xrBeginSession");
	}

	bool endSession()
	{
		return check(xrEndSession(m_xrSession), "xrEndSession");
	}

	void processEvent()
	{
		while (true) {
			XrEventDataBuffer eventBuffer{ XR_TYPE_EVENT_DATA_BUFFER };
			auto pollResult = xrPollEvent(m_xrInstance, &eventBuffer);
			//check(pollResult, "xrPollEvent");
			if (pollResult == XR_EVENT_UNAVAILABLE) {
				break;
			}

			switch (eventBuffer.type) {
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
			{
				m_xrState = reinterpret_cast<XrEventDataSessionStateChanged&>(eventBuffer).state;
				switch (m_xrState) {
				case XR_SESSION_STATE_READY:
					beginSession();
					break;

				case XR_SESSION_STATE_STOPPING:
					endSession();
					break;
				}
			}
			break;

			default:
				break;
			}
		}
	}

	template<typename FUNC_DRAW>
	void draw(FUNC_DRAW func_draw)
	{
		switch (m_xrState) {
		case XR_SESSION_STATE_READY:
		case XR_SESSION_STATE_FOCUSED:
		case XR_SESSION_STATE_SYNCHRONIZED:
		case XR_SESSION_STATE_VISIBLE:
		{
			XrFrameState frameState{ XR_TYPE_FRAME_STATE };
			XrFrameWaitInfo frameWaitInfo{ XR_TYPE_FRAME_WAIT_INFO, nullptr };
			if (XR_UNQUALIFIED_SUCCESS(xrWaitFrame(m_xrSession, &frameWaitInfo, &frameState)))
			{
				uint32_t	uWidth = m_vViews[0].maxImageRectWidth,
					uHeight = m_vViews[0].maxImageRectHeight;

				XrFrameBeginInfo frameBeginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
				check(xrBeginFrame(m_xrSession, &frameBeginInfo), "xrBeginFrame");

				// Update views
				if (frameState.shouldRender)
				{
					XrViewState vs{ XR_TYPE_VIEW_STATE };
					XrViewLocateInfo vi{ XR_TYPE_VIEW_LOCATE_INFO, nullptr, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, frameState.predictedDisplayTime, m_xrSpace };

					uint32_t eyeViewStateCount = 0;
					std::vector<XrView> eyeViewStates;
					check(xrLocateViews(m_xrSession, &vi, &vs, eyeViewStateCount, &eyeViewStateCount, nullptr), "xrLocateViews-1");
					eyeViewStates.resize(eyeViewStateCount, { XR_TYPE_VIEW });
					check(xrLocateViews(m_xrSession, &vi, &vs, eyeViewStateCount, &eyeViewStateCount, eyeViewStates.data()), "xrLocateViews-2");

					for (int i = 0; i < eyeViewStates.size(); ++i)
					{
						const XrView& viewStates = eyeViewStates[i];
						m_vProjectionLayerViews[i].fov = viewStates.fov;
						m_vProjectionLayerViews[i].pose = viewStates.pose;

						uint32_t swapchainIndex;

						XrSwapchainImageAcquireInfo ai{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO, nullptr };
						check(xrAcquireSwapchainImage(m_vViewDatas[i].m_xrSwapChain, &ai, &swapchainIndex), "xrAcquireSwapchainImage");

						XrSwapchainImageWaitInfo wi{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr, XR_INFINITE_DURATION };
						check(xrWaitSwapchainImage(m_vViewDatas[i].m_xrSwapChain, &wi), "xrWaitSwapchainImage");

						{
							glViewport(0, 0, uWidth, uHeight);
							glScissor(0, 0, uWidth, uHeight);

							glBindFramebuffer(GL_FRAMEBUFFER, m_vViewDatas[i].m_glFrameBuffer);
							glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_vViewDatas[i].m_vSwapchainImages[swapchainIndex].image, 0);

							auto proj = createProjectionMatrix(viewStates.fov, 0.01, 1000);
							auto view = createModelViewMatrix(viewStates.pose);
							func_draw(proj, view);

							glBindFramebuffer(GL_FRAMEBUFFER, 0);
							glFinish();
						}

						XrSwapchainImageReleaseInfo ri{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, nullptr };
						check(xrReleaseSwapchainImage(m_vViewDatas[i].m_xrSwapChain, &ri), "xrReleaseSwapchainImage");
					}

					glBlitNamedFramebuffer(m_vViewDatas[0].m_glFrameBuffer, // backbuffer
						(GLuint)0,							// drawFramebuffer
						(GLint)0,							// srcX0
						(GLint)0,							// srcY0
						(GLint)uWidth,						// srcX1
						(GLint)uHeight,						// srcY1
						(GLint)0,							// dstX0
						(GLint)0,							// dstY0
						(GLint)uWidth,						// dstX1
						(GLint)uHeight,						// dstY1
						(GLbitfield)GL_COLOR_BUFFER_BIT,	// mask
						(GLenum)GL_LINEAR);					// filter
				}

				// End frame
				XrFrameEndInfo frameEndInfo{ XR_TYPE_FRAME_END_INFO, nullptr, frameState.predictedDisplayTime, XR_ENVIRONMENT_BLEND_MODE_OPAQUE };
				if (frameState.shouldRender) {
					frameEndInfo.layerCount = (uint32_t)m_vLayersPointers.size();
					frameEndInfo.layers = m_vLayersPointers.data();
				}

				check(xrEndFrame(m_xrSession, &frameEndInfo), "xrEndFrame");
			}
			break;
		}

		default:
			break;
		}
	}

protected:
	struct SViewData
	{
		XrSwapchain	m_xrSwapChain;
		GLuint		m_glFrameBuffer = 0;
		std::vector<XrSwapchainImageOpenGLKHR>	m_vSwapchainImages;
	};

protected:
	bool check(const XrResult& rs, const std::string& sExtMsg)
	{
		if (rs == XR_SUCCESS)
			return true;

		char sMsg[XR_MAX_RESULT_STRING_SIZE];
		if (xrResultToString(m_xrInstance, rs, sMsg) == XR_SUCCESS)
			std::cout << "Error: " << sMsg << "\n  " << sExtMsg << std::endl;
		else
			std::cout << "Error: " << rs << "\n  " << sExtMsg << std::endl;
		return false;
	}

	void enumerateApiLayers()
	{
		uint32_t uAPILayerNum = 0;
		if (check(xrEnumerateApiLayerProperties(0, &uAPILayerNum, nullptr), "xrEnumerateApiLayerProperties-1") && uAPILayerNum > 0)
		{
			m_vSupportedApiLayers.resize(uAPILayerNum, { XR_TYPE_API_LAYER_PROPERTIES });
			check(xrEnumerateApiLayerProperties(uAPILayerNum, &uAPILayerNum, m_vSupportedApiLayers.data()), "xrEnumerateApiLayerProperties-2");
		}
	}

	void enumerateExtensions()
	{
		uint32_t uExtensionNum = 0;
		if (check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &uExtensionNum, nullptr), "xrEnumerateInstanceExtensionProperties-1") && uExtensionNum > 0)
		{
			m_vSupportedExtensions.resize(uExtensionNum, { XR_TYPE_EXTENSION_PROPERTIES });
			check(xrEnumerateInstanceExtensionProperties(nullptr, uExtensionNum, &uExtensionNum, m_vSupportedExtensions.data()), "xrEnumerateInstanceExtensionProperties-2");
		}
	}

	bool createInstance()
	{
		XrInstanceCreateInfo infoCreate;
		infoCreate.type = XR_TYPE_INSTANCE_CREATE_INFO;
		infoCreate.next = nullptr;
		infoCreate.createFlags = 0;
		infoCreate.applicationInfo = { "TestApp", 1, "TestEngine", 1, XR_CURRENT_API_VERSION };
		infoCreate.enabledApiLayerCount = 0;
		infoCreate.enabledApiLayerNames = {};
		infoCreate.enabledExtensionCount = (uint32_t)m_vRequiredExtensions.size();
		infoCreate.enabledExtensionNames = m_vRequiredExtensions.data();

		return check(xrCreateInstance(&infoCreate, &m_xrInstance), "xrCreateInstance");
	}

	bool getSystem()
	{
		XrSystemGetInfo infoSysId{ XR_TYPE_SYSTEM_GET_INFO, nullptr,XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY };
		return check(xrGetSystem(m_xrInstance, &infoSysId, &m_xrSystem), "xrGetSystem");
	}

	bool createSession()
	{
		// Graphics requirements
		XrGraphicsRequirementsOpenGLKHR reqOpenGL{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };

		// Magic code block....
		// link error if call xrGetOpenGLGraphicsRequirementsKHR() directlly
		PFN_xrGetOpenGLGraphicsRequirementsKHR func;
		if (check(xrGetInstanceProcAddr(m_xrInstance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&func), "xrGetInstanceProcAddr"))
		{
			if (check(func(m_xrInstance, m_xrSystem, &reqOpenGL), "PFN_xrGetOpenGLGraphicsRequirementsKHR"))
			{
				XrGraphicsBindingOpenGLWin32KHR gbOpenGL{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR , nullptr, wglGetCurrentDC(), wglGetCurrentContext() };
				XrSessionCreateInfo infoSession{ XR_TYPE_SESSION_CREATE_INFO, &gbOpenGL, 0, m_xrSystem };
				return check(xrCreateSession(m_xrInstance, &infoSession, &m_xrSession), "xrCreateSession");
			}
		}
		return false;
	}

	bool checkViewConfiguration()
	{
		uint32_t uViewsNum = 0;
		if (check(xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystem, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &uViewsNum, nullptr), "xrEnumerateViewConfigurationViews-1") && uViewsNum > 0)
		{
			m_vViews.resize(uViewsNum, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
			m_vViewDatas.resize(uViewsNum);
			return check(xrEnumerateViewConfigurationViews(m_xrInstance, m_xrSystem, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, uViewsNum, &uViewsNum, m_vViews.data()), "xrEnumerateViewConfigurationViews-2");
		}

		return false;
	}

	bool createSwapChain()
	{
		XrSwapchainCreateInfo infoSwapchain;
		infoSwapchain.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		infoSwapchain.next = nullptr;
		infoSwapchain.createFlags = 0;
		infoSwapchain.usageFlags = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
		infoSwapchain.format = (int64_t)GL_SRGB8_ALPHA8;
		infoSwapchain.sampleCount = 1;
		infoSwapchain.width = m_vViews[0].recommendedImageRectWidth;
		infoSwapchain.height = m_vViews[0].recommendedImageRectHeight;
		infoSwapchain.faceCount = 1;
		infoSwapchain.arraySize = 1;
		infoSwapchain.mipCount = 1;

		bool bOK = true;
		for (auto& rVData : m_vViewDatas)
		{
			if (check(xrCreateSwapchain(m_xrSession, &infoSwapchain, &rVData.m_xrSwapChain), "xrCreateSwapchain"))
			{
				uint32_t uSwapchainNum = 0;
				if (check(xrEnumerateSwapchainImages(rVData.m_xrSwapChain, uSwapchainNum, &uSwapchainNum, nullptr), "xrEnumerateSwapchainImages-1") && uSwapchainNum > 0)
				{
					rVData.m_vSwapchainImages.resize(uSwapchainNum, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
					if (!check(xrEnumerateSwapchainImages(rVData.m_xrSwapChain, uSwapchainNum, &uSwapchainNum, (XrSwapchainImageBaseHeader*)rVData.m_vSwapchainImages.data()), "xrEnumerateSwapchainImages-2"))
					{
						bOK = false;
						break;
					}
				}
				else
				{
					bOK = false;
					break;
				}
			}
			else
			{
				bOK = false;
				break;
			}
		}

		return bOK;
	}

	bool createReferenceSpace()
	{
		XrReferenceSpaceCreateInfo infoRefSpace{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO, nullptr, XR_REFERENCE_SPACE_TYPE_LOCAL };
		infoRefSpace.poseInReferenceSpace.orientation = { 0,0,0,-1 };
		infoRefSpace.poseInReferenceSpace.position = { 0,0,0 };
		return check(xrCreateReferenceSpace(m_xrSession, &infoRefSpace, &m_xrSpace), "xrCreateReferenceSpace");
	}

	bool prepareCompositionLayer()
	{
		m_vProjectionLayerViews.resize(m_vViewDatas.size());
		for (int i = 0; i < m_vViewDatas.size(); ++i)
		{
			m_vProjectionLayerViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
			m_vProjectionLayerViews[i].next = nullptr;
			m_vProjectionLayerViews[i].subImage.imageArrayIndex = 0;
			m_vProjectionLayerViews[i].subImage.swapchain = m_vViewDatas[i].m_xrSwapChain;
			m_vProjectionLayerViews[i].subImage.imageRect.extent = { (int32_t)m_vViews[0].recommendedImageRectWidth, (int32_t)m_vViews[0].recommendedImageRectHeight };
		}

		XrCompositionLayerProjection* pProjectionLayer = new XrCompositionLayerProjection{ XR_TYPE_COMPOSITION_LAYER_PROJECTION, nullptr, 0, m_xrSpace,(uint32_t)m_vProjectionLayerViews.size(), m_vProjectionLayerViews.data() };
		m_vLayersPointers.push_back((XrCompositionLayerBaseHeader*)pProjectionLayer);

		return true;
	}

	bool createFrameBubber()
	{
		for (auto& rVData : m_vViewDatas)
			glGenFramebuffers(1, &rVData.m_glFrameBuffer);

		return true;
	}

	TMatrix createProjectionMatrix(const XrFovf& xrFov, const float fNear, const float fFar)
	{
		const float tanAngleLeft = tanf(xrFov.angleLeft);
		const float tanAngleRight = tanf(xrFov.angleRight);
		const float tanAngleDown = tanf(xrFov.angleDown);
		const float tanAngleUp = tanf(xrFov.angleUp);

		const float tanAngleWidth = tanAngleRight - tanAngleLeft;
		const float tanAngleHeight = tanAngleUp - tanAngleDown;

		TMatrix matProjection;
		matProjection[0] = 2 / tanAngleWidth;
		matProjection[4] = 0;
		matProjection[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
		matProjection[12] = 0;

		matProjection[1] = 0;
		matProjection[5] = 2 / tanAngleHeight;
		matProjection[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
		matProjection[13] = 0;

		matProjection[2] = 0;
		matProjection[6] = 0;
		matProjection[10] = -1;
		matProjection[14] = -(fNear + fNear);

		matProjection[3] = 0;
		matProjection[7] = 0;
		matProjection[11] = -1;
		matProjection[15] = 0;
		return matProjection;
	}

	TMatrix createModelViewMatrix(const XrPosef& xrPose)
	{
		// CreateFrom Quaternion
		TMatrix matRotation;
		{
			const float x2 = xrPose.orientation.x + xrPose.orientation.x;
			const float y2 = xrPose.orientation.y + xrPose.orientation.y;
			const float z2 = xrPose.orientation.z + xrPose.orientation.z;

			const float xx2 = xrPose.orientation.x * x2;
			const float yy2 = xrPose.orientation.y * y2;
			const float zz2 = xrPose.orientation.z * z2;

			const float yz2 = xrPose.orientation.y * z2;
			const float wx2 = xrPose.orientation.w * x2;
			const float xy2 = xrPose.orientation.x * y2;
			const float wz2 = xrPose.orientation.w * z2;
			const float xz2 = xrPose.orientation.x * z2;
			const float wy2 = xrPose.orientation.w * y2;

			matRotation[0] = 1.0f - yy2 - zz2;
			matRotation[1] = xy2 + wz2;
			matRotation[2] = xz2 - wy2;
			matRotation[3] = 0.0f;

			matRotation[4] = xy2 - wz2;
			matRotation[5] = 1.0f - xx2 - zz2;
			matRotation[6] = yz2 + wx2;
			matRotation[7] = 0.0f;

			matRotation[8] = xz2 + wy2;
			matRotation[9] = yz2 - wx2;
			matRotation[10] = 1.0f - xx2 - yy2;
			matRotation[11] = 0.0f;

			matRotation[12] = 0.0f;
			matRotation[13] = 0.0f;
			matRotation[14] = 0.0f;
			matRotation[15] = 1.0f;
		}

		// Create Translation
		TMatrix matTranslate;
		{
			matTranslate[0] = 1.0f;
			matTranslate[1] = 0.0f;
			matTranslate[2] = 0.0f;
			matTranslate[3] = 0.0f;
			matTranslate[4] = 0.0f;
			matTranslate[5] = 1.0f;
			matTranslate[6] = 0.0f;
			matTranslate[7] = 0.0f;
			matTranslate[8] = 0.0f;
			matTranslate[9] = 0.0f;
			matTranslate[10] = 1.0f;
			matTranslate[11] = 0.0f;
			matTranslate[12] = xrPose.position.x;
			matTranslate[13] = xrPose.position.y;
			matTranslate[14] = xrPose.position.z;
			matTranslate[15] = 1.0f;
		}

		TMatrix matRes = [](const TMatrix& a, const TMatrix& b) {
			TMatrix matResult;
			matResult[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
			matResult[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
			matResult[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
			matResult[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];

			matResult[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
			matResult[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
			matResult[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
			matResult[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];

			matResult[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
			matResult[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
			matResult[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
			matResult[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];

			matResult[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
			matResult[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
			matResult[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
			matResult[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];

			return matResult;
		}(matTranslate, matRotation);

		return [](const TMatrix& matSrc) {
			TMatrix matResult;
			matResult[0] = matSrc[0];
			matResult[1] = matSrc[4];
			matResult[2] = matSrc[8];
			matResult[3] = 0.0f;
			matResult[4] = matSrc[1];
			matResult[5] = matSrc[5];
			matResult[6] = matSrc[9];
			matResult[7] = 0.0f;
			matResult[8] = matSrc[2];
			matResult[9] = matSrc[6];
			matResult[10] = matSrc[10];
			matResult[11] = 0.0f;
			matResult[12] = -(matSrc[0] * matSrc[12] + matSrc[1] * matSrc[13] + matSrc[2] * matSrc[14]);
			matResult[13] = -(matSrc[4] * matSrc[12] + matSrc[5] * matSrc[13] + matSrc[6] * matSrc[14]);
			matResult[14] = -(matSrc[8] * matSrc[12] + matSrc[9] * matSrc[13] + matSrc[10] * matSrc[14]);
			matResult[15] = 1.0f;

			return matResult;
		}(matRes);
	}

protected:
	XrInstance	m_xrInstance;
	XrSystemId	m_xrSystem = XR_NULL_SYSTEM_ID;
	XrSession	m_xrSession;
	XrSpace		m_xrSpace;
	XrSessionState	m_xrState = XR_SESSION_STATE_IDLE;

	std::vector<XrApiLayerProperties>		m_vSupportedApiLayers;
	std::vector<XrExtensionProperties>		m_vSupportedExtensions;
	std::vector<XrViewConfigurationView>	m_vViews;
	std::vector<SViewData>					m_vViewDatas;

	std::vector<XrCompositionLayerProjectionView>	m_vProjectionLayerViews;
	std::vector<XrCompositionLayerBaseHeader*>		m_vLayersPointers;

	std::vector<const char*>	m_vRequiredExtensions;
};
