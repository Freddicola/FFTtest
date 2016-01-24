// FFT_DX11.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define SIZE1 102

// int _tmain(int argc, _TCHAR* argv[])
int test_main(int argc, _TCHAR* argv[])
{
	CComPtr<ID3D11Device> pDeviceOut=NULL;
	CComPtr<ID3D11DeviceContext> pContextOut=NULL;

	D3D_FEATURE_LEVEL flOut = D3D_FEATURE_LEVEL_9_1;
	D3D11CreateDevice(
		NULL,
		D3D_DRIVER_TYPE_REFERENCE,
		0,
		0,
		0,
		0,
		D3D11_SDK_VERSION,
		&pDeviceOut,
		&flOut,
		&pContextOut);

	//just filling up the buffer
	::std::vector<complex> vBuf1;
	for(UINT i=0;i<SIZE1;i++)
	{
		complex c;
		c.x = (float)i;
		c.y = (float)i;
		vBuf1.push_back(c);
	}

	CComPtr<ID3D11Buffer>              pBufin1=NULL;     
	CreateByteOrderBufferOnGPU( pDeviceOut, sizeof(complex), (UINT)vBuf1.size(), &vBuf1[0], &pBufin1 );
	
	CComPtr<ID3D11UnorderedAccessView> pBufin1_UAV=NULL; 
	CreateBufferUAV( pDeviceOut, pBufin1, &pBufin1_UAV );

	D3DX11_FFT_DESC  fftdesc={};
	fftdesc.ElementLengths[0] = fftdesc.ElementLengths[1] = fftdesc.ElementLengths[2] = 1;

	fftdesc.NumDimensions=1;
	fftdesc.ElementLengths[0]=vBuf1.size();
	fftdesc.DimensionMask=D3DX11_FFT_DIM_MASK_1D   ;
	fftdesc.Type=D3DX11_FFT_DATA_TYPE_COMPLEX;

	D3DX11_FFT_BUFFER_INFO  fftbufferinfo={};

	CComPtr<ID3DX11FFT> pFFT=0;
	HRESULT hr = D3DX11CreateFFT(pContextOut, &fftdesc, 0, &fftbufferinfo, &pFFT);

	std::vector<ID3D11Buffer *>              ArrBufTemp;  //temporary buffer required by DX11 FFT
	std::vector<ID3D11UnorderedAccessView *> ArrBufTemp_UAV;

	for (UINT i = 0; i < fftbufferinfo.NumTempBufferSizes; i++)
	{
		ID3D11Buffer *tmp_pBufT1=0;
		CreateByteOrderBufferOnGPU( pDeviceOut, sizeof(float), fftbufferinfo.TempBufferFloatSizes[i], 0, &tmp_pBufT1 );
		
		ID3D11UnorderedAccessView *tmp_pBufT1_UAV=0;
		CreateBufferUAV( pDeviceOut, tmp_pBufT1, &tmp_pBufT1_UAV );

		ArrBufTemp.push_back(tmp_pBufT1);
		ArrBufTemp_UAV.push_back(tmp_pBufT1_UAV);
	}

	std::vector<ID3D11Buffer *>              ArrBufpreCompu;  //temporary buffer required by DX11 FFT
	std::vector<ID3D11UnorderedAccessView *> ArrBufpreCompu_UAV;

	for(UINT i=0;i<fftbufferinfo.NumPrecomputeBufferSizes;i++)
	{
		ID3D11Buffer *tmp_pBufT1=0;
		CreateByteOrderBufferOnGPU( pDeviceOut, sizeof(float), fftbufferinfo.TempBufferFloatSizes[i], 0, &tmp_pBufT1 );
		
		ID3D11UnorderedAccessView *tmp_pBufT1_UAV=0;
		CreateBufferUAV( pDeviceOut, tmp_pBufT1, &tmp_pBufT1_UAV );

		ArrBufpreCompu.push_back(tmp_pBufT1);
		ArrBufpreCompu_UAV.push_back(tmp_pBufT1_UAV);

	}

	hr = pFFT->AttachBuffersAndPrecompute(
			fftbufferinfo.NumTempBufferSizes,
			fftbufferinfo.NumTempBufferSizes > 0 ? &ArrBufTemp_UAV[0] : NULL,
		    fftbufferinfo.NumPrecomputeBufferSizes,
			fftbufferinfo.NumPrecomputeBufferSizes > 0 ? &ArrBufpreCompu_UAV[0] : NULL
		);

	CComPtr<ID3D11UnorderedAccessView> respBufin1_UAV, respBufin1_UAV1;
	hr = pFFT->ForwardTransform(pBufin1_UAV, &respBufin1_UAV1);
	hr = pFFT->InverseTransform(respBufin1_UAV1, &respBufin1_UAV);  //do the inverse transform
	
	CComPtr<ID3D11Buffer> resultOutput;
	respBufin1_UAV->GetResource((ID3D11Resource **)&resultOutput);
	
	CComPtr<ID3D11Buffer> reultoutput_cpuread;
	reultoutput_cpuread.Attach(CreateAndCopyToDebugBuf(resultOutput,pContextOut));
	
	D3D11_MAPPED_SUBRESOURCE MappedResource; 
	pContextOut->Map(reultoutput_cpuread , 0, D3D11_MAP_READ, 0, &MappedResource );
	
	float *p = (float*)MappedResource.pData;
	
	for(UINT i=0;i<fftbufferinfo.NumTempBufferSizes;i++)
	{
		SafeRelease(ArrBufTemp[i]);
		SafeRelease(ArrBufTemp_UAV[i]);
	}

	for(UINT i=0;i<fftbufferinfo.NumPrecomputeBufferSizes;i++)
	{
		SafeRelease(ArrBufpreCompu[i]);
		SafeRelease(ArrBufpreCompu_UAV[i]);
	}

	/////////////-------------------------///////////////////
	//to test DX11 FFT output
	{
		complex *pft=fourrier(&vBuf1[0], vBuf1.size());			//do the IFT
		complex *pift=invfourrier(pft, vBuf1.size());           //do the FT to get the original buffer
	
		int j=0;
		for(UINT i=0;i<vBuf1.size();i++)
		{

			printf("org: %lf %lf, FT(IFT) %lf %lf FT(IFT dx11) %lf %lf\n",vBuf1[i].x,vBuf1[i].y,
											   pift[i].x,pift[i].y,
											   p[j],p[j+1] /* diff calculated by DX11 library and the invfourrier*/
											   );
			j+=2;							 	
		}

	
		free(pft);
		free(pift);
	}
	/////////////-------------------------///////////////////

	pContextOut->Unmap(reultoutput_cpuread, 0); //unmap the buffer

	return 0;
}

