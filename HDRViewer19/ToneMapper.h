//#pragma once
//
//#include <CommonStates.h>
//#include "C:/Users/lab/Downloads/DirectXTK-jun2019/DirectXTK-jun2019/Src/Shaders/Compiled/ToneMap_PSHDR10.inc"
//#include "C:/Users/lab/Downloads/DirectXTK-jun2019/DirectXTK-jun2019/Src/Shaders/Compiled/ToneMap_VSQuad.inc"
//
//#include "pch.h"
//#include <DirectXHelpers.h>
//
//struct ShaderBytecode
//{
//	void const* code;
//	size_t length;
//};
//
//const ShaderBytecode pixelShader = { ToneMap_PSHDR10, sizeof(ToneMap_PSHDR10) };
//
//class DeviceResources
//{
//public:
//	DeviceResources(ID3D11Device* device) : stateObjects(device), mDevice(device), mVertexShader{}, mPixelShader{}, mMutex{}
//	{
//	
//	}
//
//	ID3D11VertexShader* GetVertexShader()
//	{
//		Microsoft::WRL::ComPtr<ID3D11VertexShader> pResult;
//		mDevice->CreateVertexShader(ToneMap_VSQuad, sizeof(ToneMap_VSQuad), nullptr, pResult.GetAddressOf());
//	}
//
//	DirectX::CommonStates stateObjects;
//	Microsoft::WRL::ComPtr<ID3D11Device> mDevice;
//	Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
//	Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
//	std::mutex mMutex;
//};
//
//class ToneMapper
//{
//	void Process(ID3D11DeviceContext* context, )
//};
//
