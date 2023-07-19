#include "directxtex_wrapper.hpp"

#include "DirectXTex.h"
using namespace DirectX;

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#include <fmt/xchar.h>

namespace
{
bool GetDXGIFactory(IDXGIFactory1** pFactory)
{
	if (!pFactory)
		return false;

	*pFactory = nullptr;

	typedef HRESULT(WINAPI* pfn_CreateDXGIFactory1)(REFIID riid, _Out_ void **ppFactory);

	static pfn_CreateDXGIFactory1 s_CreateDXGIFactory1 = nullptr;

	if (!s_CreateDXGIFactory1)
	{
		HMODULE hModDXGI = LoadLibraryW(L"dxgi.dll");
		if (!hModDXGI)
			return false;

		s_CreateDXGIFactory1 = reinterpret_cast<pfn_CreateDXGIFactory1>(reinterpret_cast<void*>(GetProcAddress(hModDXGI, "CreateDXGIFactory1")));
		if (!s_CreateDXGIFactory1)
			return false;
	}

	return SUCCEEDED(s_CreateDXGIFactory1(IID_PPV_ARGS(pFactory)));
}

bool CreateDevice(int adapter, ID3D11Device** pDevice)
{
	if (!pDevice)
		return false;

	*pDevice = nullptr;

	static PFN_D3D11_CREATE_DEVICE s_DynamicD3D11CreateDevice = nullptr;

	if (!s_DynamicD3D11CreateDevice)
	{
		HMODULE hModD3D11 = LoadLibraryW(L"d3d11.dll");
		if (!hModD3D11)
			return false;

		s_DynamicD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(hModD3D11, "D3D11CreateDevice")));
		if (!s_DynamicD3D11CreateDevice)
			return false;
	}

	const D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	ComPtr<IDXGIAdapter> pAdapter;
	if (adapter >= 0)
	{
		ComPtr<IDXGIFactory1> dxgiFactory;
		if (GetDXGIFactory(dxgiFactory.GetAddressOf()))
		{
			if (FAILED(dxgiFactory->EnumAdapters(static_cast<UINT>(adapter), pAdapter.GetAddressOf())))
			{
				wprintf(L"\nERROR: Invalid GPU adapter index (%d)!\n", adapter);
				return false;
			}
		}
	}

	D3D_FEATURE_LEVEL fl;
	HRESULT hr = s_DynamicD3D11CreateDevice(pAdapter.Get(),
		(pAdapter) ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
		nullptr, createDeviceFlags, featureLevels, static_cast<UINT>(std::size(featureLevels)),
		D3D11_SDK_VERSION, pDevice, &fl, nullptr);
	if (SUCCEEDED(hr))
	{
		if (fl < D3D_FEATURE_LEVEL_11_0)
		{
			D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hwopts;
			hr = (*pDevice)->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hwopts, sizeof(hwopts));
			if (FAILED(hr))
				memset(&hwopts, 0, sizeof(hwopts));

			if (!hwopts.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
			{
				if (*pDevice)
				{
					(*pDevice)->Release();
					*pDevice = nullptr;
				}
				hr = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		ComPtr<IDXGIDevice> dxgiDevice;
		hr = (*pDevice)->QueryInterface(IID_PPV_ARGS(dxgiDevice.GetAddressOf()));
		if (SUCCEEDED(hr))
		{
			hr = dxgiDevice->GetAdapter(pAdapter.ReleaseAndGetAddressOf());
			if (SUCCEEDED(hr))
			{
				DXGI_ADAPTER_DESC desc;
				hr = pAdapter->GetDesc(&desc);
				if (SUCCEEDED(hr))
				{
					wprintf(L"\n[Using DirectCompute %ls on \"%ls\"]\n",
						(fl >= D3D_FEATURE_LEVEL_11_0) ? L"5.0" : L"4.0", desc.Description);
				}
			}
		}

		return true;
	}
	else
		return false;
}

const wchar_t* GetErrorDesc(HRESULT hr)
{
	static wchar_t desc[1024] = {};

	LPWSTR errorText = nullptr;

	const DWORD result = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr, static_cast<DWORD>(hr),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorText), 0, nullptr
	);

	*desc = 0;

	if (result > 0 && errorText)
	{
		swprintf_s(desc, L": %ls", errorText);

		size_t len = wcslen(desc);
		if (len >= 1)
		{
			desc[len - 1] = 0;
		}

		if (errorText)
			LocalFree(errorText);

		for (wchar_t* ptr = desc; *ptr != 0; ++ptr)
			if (*ptr == L'\r' || *ptr == L'\n')
				*ptr = L' ';
	}

	return desc;
}

// globals just for this cpp
ComPtr<ID3D11Device> pDevice;
int const adapter = 1; // 0 = CPU, 1 = GPU
}


namespace Wrapper
{
bool Initialize()
{
	if (pDevice)
		return true;

//	https://github.com/microsoft/DirectXTex/blob/4d9d7a8ceba6d6a121cd1aae160a0b856ef03d89/Texconv/texconv.cpp#L3592
	if (CreateDevice(adapter, pDevice.GetAddressOf()))
		return true;

	fmt::print("WARNING: DirectCompute is not available, using BC6H / BC7 CPU codec\n");
	return false;
}

Buffer BC7(Buffer const & src_img)
{
	auto pixel_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	auto pixel_count = src_img.size / (BitsPerPixel(pixel_format) / 8);
	auto height = pixel_count / src_img.width;

	size_t row_pitch, slice_pitch;
	ComputePitch(pixel_format, src_img.width, height, row_pitch, slice_pitch);

	Image img{
        .width = src_img.width,
        .height = height,
		.format = pixel_format,
        .rowPitch = row_pitch,
        .slicePitch = slice_pitch,
        .pixels = reinterpret_cast<uint8_t *>(src_img.ptr),
	};

	ScratchImage dst;

	auto hr = Compress(pDevice.Get(), img, DXGI_FORMAT_BC7_UNORM, TEX_COMPRESS_BC7_USE_3SUBSETS, 1, dst);
	if (FAILED(hr))
	{
		fmt::print(L"FAILED [compress] Error[{}] {})\n", static_cast<unsigned int>(hr), GetErrorDesc(hr));
		return {};
	}

	auto & dst_info = dst.GetMetadata();
	assert(img.width == dst_info.width);
	assert(img.height == dst_info.height);
	assert(1 == dst_info.depth);
	assert(1 == dst_info.mipLevels);

	auto size = dst.GetPixelsSize();
	auto * ptr = malloc(size);
	memcpy(ptr, dst.GetPixels(), size);

	return {
		.ptr = ptr,
		.size = size,
		.width = dst_info.width,
	};
}
}