// stdafx.cpp : source file that includes just the standard includes
// FFT_DX11.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

HRESULT CreateByteOrderBufferOnGPU( ID3D11Device* pDevice, UINT uElementSize, UINT uCount, VOID* pInitData, ID3D11Buffer** ppBufOut )
{
	*ppBufOut = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );

	/*
		size of the buffer in bytes
		버퍼 사이즈 (byte 단위)
	*/
	desc.ByteWidth = uElementSize * uCount;

	/*
	D3D11_BIND_SHADER_RESOURCE
		Bind a buffer or texture to a shader stage; this flag cannot be used with the D3D11_MAP_WRITE_NO_OVERWRITE flag.
		Note  
		The Direct3D 11.1 runtime, which is available starting with Windows 8, enables mapping dynamic constant buffers 
		and shader resource views (SRVs) of dynamic buffers with D3D11_MAP_WRITE_NO_OVERWRITE. 
		The Direct3D 11 and earlier runtimes limited mapping to vertex or index buffers. 
		To determine if a Direct3D device supports these features, call ID3D11Device::CheckFeatureSupport with D3D11_FEATURE_D3D11_OPTIONS. 
		CheckFeatureSupport fills members of a D3D11_FEATURE_DATA_D3D11_OPTIONS structure with the device's features. 
		The relevant members here are MapNoOverwriteOnDynamicConstantBuffer and MapNoOverwriteOnDynamicBufferSRV.
	D3D11_BIND_UNORDERED_ACCESS
		Bind an unordered access resource.

	D3D11_BIND_STREAM_OUTPUT
	D3D11_BIND_RENDER_TARGET
	D3D11_BIND_DEPTH_STENCIL
	D3D11_BIND_VERTEX_BUFFER
	D3D11_BIND_INDEX_BUFFER
	D3D11_BIND_CONSTANT_BUFFER
	D3D11_BIND_DECODER
	D3D11_BIND_VIDEO_ENCODER
	*/
	desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

	/*
	D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS
		Enables a resource as a byte address buffer.

		A byte address buffer is a buffer whose contents are addressable by a byte offset. 
		이것은 byte offset으로 addressing할 수 있는 content를 가지는 버퍼를 말한다.
		Normally, the contents of a buffer are indexed per element using a stride for each element (S) and the element number (N) as given by S*N. 
		일반적으로 버퍼의 내용은 각 엘리먼트에 대하여, 스트라이드를 이용하여 인덱스된다.
		
		A byte address buffer, 	which can also be called a raw buffer, 
		A byte address buffer 이것은 raw 버퍼로 부를 수 있다.
		uses a byte value offset from the beginning of the buffer to access data. 
		이 버퍼는 데이터를 엑세스하기위한 버퍼의 시작위치 부터의 바이트 값의 오프셋을 사용한다.
		The byte value must be a multiple of four so that it is DWORD aligned. 
		바이트 값은 4의 배수여야한다.
		If any other value is provided, behavior is undefined.

		Shader model 5 introduces objects for accessing a read-only byte address buffer 
		as well as 
		a read-write byte address buffer. 
		The contents of a byte address buffer is designed to be a 32-bit unsigned integer; 
		if the value in the buffer is not really an unsigned integer, use a function such as asfloat to read it.
	*/
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	desc.StructureByteStride = uElementSize;

	if ( pInitData )
	{
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = pInitData;

		return pDevice->CreateBuffer( &desc, &InitData, ppBufOut );
	} 
	else
		return pDevice->CreateBuffer( &desc, NULL, ppBufOut );
}

//--------------------------------------------------------------------------------------
// Create Shader Resource View for Structured or Raw Buffers
//--------------------------------------------------------------------------------------
HRESULT CreateBufferSRV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11ShaderResourceView** ppSRVOut )
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory( &descBuf, sizeof(descBuf) );
	pBuffer->GetDesc( &descBuf );

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	desc.BufferEx.FirstElement = 0;

	if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
	{
		// This is a Raw Buffer
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		desc.BufferEx.NumElements = descBuf.ByteWidth / 4;
	} 
	else if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
	{
		// This is a Structured Buffer
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.BufferEx.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride;
	} 
	else
	{
		return E_INVALIDARG;
	}

	return pDevice->CreateShaderResourceView( pBuffer, &desc, ppSRVOut );
}

//--------------------------------------------------------------------------------------
// Create Unordered Access View for Structured or Raw Buffers
//-------------------------------------------------------------------------------------- 
HRESULT CreateBufferUAV( ID3D11Device* pDevice, ID3D11Buffer* pBuffer, ID3D11UnorderedAccessView** ppUAVOut )
{
	D3D11_BUFFER_DESC descBuf;
	ZeroMemory( &descBuf, sizeof(descBuf) );
	pBuffer->GetDesc( &descBuf );

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	desc.Buffer.FirstElement = 0;

	if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS )
	{
		// This is a Raw Buffer
		// Format must be DXGI_FORMAT_R32_TYPELESS, when creating Raw Unordered Access View
		desc.Format = DXGI_FORMAT_R32_TYPELESS; 
		desc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
		desc.Buffer.NumElements = descBuf.ByteWidth / 4; 
	} 
	else if ( descBuf.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED )
	{
		// This is a Structured Buffer
		// Format must be must be DXGI_FORMAT_UNKNOWN, when creating a View of a Structured Buffer
		desc.Format = DXGI_FORMAT_UNKNOWN;      
		desc.Buffer.NumElements = descBuf.ByteWidth / descBuf.StructureByteStride; 
	} 
	else
	{
		return E_INVALIDARG;
	}

	return pDevice->CreateUnorderedAccessView( pBuffer, &desc, ppUAVOut );
}

ID3D11Buffer* CreateAndCopyToDebugBuf(ID3D11Buffer* pBuffer,ID3D11DeviceContext *pContextOut)  //release the returned buffer
{
	ID3D11Buffer* debugbuf = NULL;

	D3D11_BUFFER_DESC desc;
	ZeroMemory( &desc, sizeof(desc) );
	pBuffer->GetDesc( &desc );
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.MiscFlags = 0;

	CComPtr<ID3D11Device> pDeviceOut=NULL;
	pContextOut->GetDevice(&pDeviceOut);

	pDeviceOut->CreateBuffer(&desc, NULL, &debugbuf);
	pContextOut->CopyResource( debugbuf, pBuffer );

	return debugbuf;
}

/////////////-------------------------///////////////////
//to test DX11 FFT output
complex operator + (complex a, complex b)		//addition
{
	complex z;
	z.x = a.x + b.x;
	z.y = a.y + b.y;

	return z;
}

template<class T>
complex operator + (complex a, T z)
{
	a.x = a.x + z;
	return a;
}
////////////////////////////////////////////////////////////////////////
complex operator - (complex a, complex b)
{
	complex z;
	z.x = a.x - b.x;
	z.y = a.y - b.y;

	return z;
}

template<class T>
complex operator - (complex a, T z)
{
	a.x = a.x - z;
	return a;
}
///////////////////////////////////////////////////////////////////////////
complex operator * (complex a, complex b)
{
	complex z;

	z.x = a.x * b.x - a.y * b.y;
	z.y = a.x * b.y + a.y * b.x;
	return z;
}

template<class T>
complex operator * (complex a, T z)
{
	a.x = a.x * z;
	a.y = a.y * z;
	return a;
}
/////////////////////////////////////////////////////////////////////////////

complex operator / (complex a, complex b)
{
	complex z;

	b.y = b.y*-1;
	z = a*b;
	z.x = z.x / (b.x*b.x + b.y*b.y);
	z.y = z.y / (b.x*b.x + b.y*b.y);

	return z;
}

template<class T>
complex operator / (complex a, T z)
{
	a.x = a.x / z;
	a.y = a.y / z;
	return a;
}
/////////////////////////////////////////////////////// power functions and exponentials
complex exp(complex a)   //pow(e,?)
{
	complex z;

	z.x = powf(expf(1.0), a.x)*cos(a.y);
	z.y = powf(expf(1.0), a.x)*sin(a.y);

	return z;
}

template<class T>
T exp(T z)
{
	z = powf(2.718281828, z);
	return z;
}

complex *fourrier(complex *a, int n)
{
	complex *b, i;
	i.x = 0;
	i.y = 1;
	b = (complex *)malloc(sizeof(complex)*n);

	int k, j;

	for (k = 0; k<n; k++)
	{
		b[k].x = 0;
		b[k].y = 0;
	}

	for (k = 0; k<n; k++)
		for (j = 0; j<n; j++)
			b[k] = b[k] + a[j] * exp(i * -1.0f * k * j * 2.0f * 3.1415926f / (float)n);


	return b;
}
/////////////-------------------------///////////////

complex *invfourrier(complex *a, int n)
{
	complex *b, i;
	b = (complex *)malloc(sizeof(complex)*n);

	i.x = 0.0f;
	i.y = 1.0f;

	int k, j;

	for (j = 0; j < n; j++)
	{
		b[j].x = 0.0f;
		b[j].y = 0.0f;
	}

	for (k = 0; k < n; k++)
		for (j = 0; j < n; j++)
			b[k] = b[k] + a[j] * exp(i * k * j * 2.0f * 3.1415926f / (float)n);

	for (k = 0; k<n; k++)
	{
		b[k].x = b[k].x / (float)n;
		b[k].y = b[k].y / (float)n;
	}

	return b;
}
