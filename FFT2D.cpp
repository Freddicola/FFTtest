// FFT_DX11.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include "sd_image.h"

using namespace std;


HRESULT AttachBuffersAndPrecompute(ID3D11Device *pDevice, ID3DX11FFT *pFFT, D3DX11_FFT_BUFFER_INFO &oBufferInfo)
{

	std::vector<ID3D11Buffer *>              ArrBufTemp;  //temporary buffer required by DX11 FFT
	std::vector<ID3D11UnorderedAccessView *> ArrBufTemp_UAV;

	for (UINT i = 0; i < oBufferInfo.NumTempBufferSizes; i++)
	{
		ID3D11Buffer *tmp_pBufT1 = 0;
		CreateByteOrderBufferOnGPU(pDevice, sizeof(float), oBufferInfo.TempBufferFloatSizes[i], 0, &tmp_pBufT1);

		ID3D11UnorderedAccessView *tmp_pBufT1_UAV = 0;
		CreateBufferUAV(pDevice, tmp_pBufT1, &tmp_pBufT1_UAV);

		ArrBufTemp.push_back(tmp_pBufT1);
		ArrBufTemp_UAV.push_back(tmp_pBufT1_UAV);
	}

	std::vector<ID3D11Buffer *>              ArrBufpreCompu;  //temporary buffer required by DX11 FFT
	std::vector<ID3D11UnorderedAccessView *> ArrBufpreCompu_UAV;

	for (UINT i = 0; i<oBufferInfo.NumPrecomputeBufferSizes; i++)
	{
		ID3D11Buffer *tmp_pBufT1 = 0;
		CreateByteOrderBufferOnGPU(pDevice, sizeof(float), oBufferInfo.TempBufferFloatSizes[i], 0, &tmp_pBufT1);

		ID3D11UnorderedAccessView *tmp_pBufT1_UAV = 0;
		CreateBufferUAV(pDevice, tmp_pBufT1, &tmp_pBufT1_UAV);

		ArrBufpreCompu.push_back(tmp_pBufT1);
		ArrBufpreCompu_UAV.push_back(tmp_pBufT1_UAV);

	}

	HRESULT hr;
	hr = pFFT->AttachBuffersAndPrecompute(
		oBufferInfo.NumTempBufferSizes,
		oBufferInfo.NumTempBufferSizes > 0 ? &ArrBufTemp_UAV[0] : NULL,
		oBufferInfo.NumPrecomputeBufferSizes,
		oBufferInfo.NumPrecomputeBufferSizes > 0 ? &ArrBufpreCompu_UAV[0] : NULL
		);

	return hr;
}


int _tmain(int argc, _TCHAR* argv[])
{
	CMatrix<short> oImage(3072, 3072);

	oImage.LoadFromFile("C:\\data\\test.raw");
	CMatrix<short> oSrc = oImage(128, 128, 2816, 2816);
	CMatrix<TComplex> oIn = oSrc.ToComplex();

	printf("width = %d\n", oIn.Width());
	printf("height = %d\n", oIn.Height());

	printf("data count = %d, in bytes(%d)\n", oIn.DataCount(), oIn.DataCount() * sizeof(float));


	CComPtr<ID3D11Device> pDevice = NULL;
	CComPtr<ID3D11DeviceContext> pContext = NULL;

	D3D_FEATURE_LEVEL flOut = D3D_FEATURE_LEVEL_9_1;
	D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_REFERENCE,
		0,
		0,
		0,
		0,
		D3D11_SDK_VERSION,
		&pDevice,
		&flOut,
		&pContext);

	ID3DX11FFT *pFFT = NULL;
	UINT Flags = 0;

	D3DX11_FFT_BUFFER_INFO oBufferInfo;
	ZeroMemory(&oBufferInfo, sizeof(D3DX11_FFT_BUFFER_INFO));
	HRESULT ret = D3DX11CreateFFT2DComplex(pContext, oSrc.Width(), oSrc.Height(), Flags, &oBufferInfo, &pFFT);

	printf("oBufferInfo.NumTempBufferSizes = %d\n", oBufferInfo.NumTempBufferSizes);
	for (int idx = 0; idx < D3DX11_FFT_MAX_TEMP_BUFFERS; idx++)
	{
		printf("oBufferInfo.TempBufferFloatSizes[%d] = %d\n", idx, oBufferInfo.TempBufferFloatSizes[idx]);
	}

	printf("oBufferInfo.NumPrecomputeBufferSizes = %d\n", oBufferInfo.NumPrecomputeBufferSizes);
	for (int idx = 0; idx < D3DX11_FFT_MAX_TEMP_BUFFERS; idx++)
	{
		printf("oBufferInfo.PrecomputeBufferFloatSizes[%d] = %d\n", idx, oBufferInfo.PrecomputeBufferFloatSizes[idx]);
	}





	// HRESULT CreateByteOrderBufferOnGPU(ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut)
	HRESULT hr;

	ID3D11Buffer* pIn = NULL;
	hr = CreateByteOrderBufferOnGPU(pDevice, sizeof(TComplex), oIn.DataCount(), oIn.ptr(), &pIn);

	ID3D11UnorderedAccessView *pInView = NULL;
	hr = CreateBufferUAV(pDevice, pIn, &pInView);

	hr = AttachBuffersAndPrecompute(pDevice, pFFT, oBufferInfo);
	
	CComPtr<ID3D11UnorderedAccessView> pRestoredView = NULL;
	CComPtr<ID3D11UnorderedAccessView> pOutFreqView = NULL;

	// why infinite?!!!!
	hr = pFFT->ForwardTransform(pInView, &pOutFreqView);


	hr = pFFT->InverseTransform(pOutFreqView, &pRestoredView);  //do the inverse transform

	printf("process done!!!");
	getchar();

	return 0;
}

