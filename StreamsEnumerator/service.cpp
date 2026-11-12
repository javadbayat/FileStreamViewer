#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0501

#include "service.h"
#include <stdlib.h>

// Constructor and Destructor

CStationService::CStationService(void)
	: m_cRef( 0 )
{

}

CStationService::~CStationService(void)
{

}

// IUnknown Members

STDMETHODIMP CStationService::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    if (ppv)
    {
        *ppv = NULL;

        if (IsEqualIID( riid, IID_IDispatch ) || IsEqualIID( riid, IID_IUnknown ))
            *ppv = (IDispatch *) (this);
        else
            hr = E_NOINTERFACE;

        if (*ppv)
            AddRef();
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

STDMETHODIMP_(ULONG) CStationService::AddRef(void)
{
    return((ULONG) (InterlockedIncrement((long *) &m_cRef)));
}

STDMETHODIMP_(ULONG) CStationService::Release(void)
{
    ULONG cRef = (ULONG) (InterlockedDecrement((long *) &m_cRef));
    if (!cRef)
        delete this;

    return cRef;
}

// IDispatch Members

STDMETHODIMP CStationService::GetTypeInfoCount(UINT FAR* pctInfo)
{
    if (!pctInfo)
        return E_INVALIDARG;

    *pctInfo = 0;
    return S_OK;
}

STDMETHODIMP CStationService::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo FAR* FAR* ppTInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CStationService::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames, LCID lcid, DISPID FAR* rgDispId)
{
	return E_NOTIMPL;
}

STDMETHODIMP CStationService::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, UINT FAR* puArgErr)
{
	try
	{
		if (!pDispParams)
			_com_issue_error( E_INVALIDARG );

		if (!IsEqualIID( riid, IID_NULL ))
			_com_issue_error( DISP_E_UNKNOWNINTERFACE );

		HRESULT hr = S_OK;
		_variant_t varTempResult;

		switch (dispIdMember)
		{
		case DISPID_VALUE :
			{
				if (!(wFlags & DISPATCH_METHOD))
					_com_issue_error( DISP_E_MEMBERNOTFOUND );

				if (pDispParams->cArgs != 2)
					_com_issue_error( DISP_E_BADPARAMCOUNT );

				_variant_t varFileName( pDispParams->rgvarg + 1 );
				varFileName.ChangeType( VT_BSTR );

				_variant_t varStreamCallback( pDispParams->rgvarg );

				EnumFileStreams( varFileName.bstrVal, varStreamCallback );

				break;
			}
		default:
			{
				_com_issue_error( DISP_E_MEMBERNOTFOUND );
				break;
			}
		}

		if (pVarResult)
		{
			hr = VariantCopy( pVarResult, &varTempResult );
			if (FAILED(hr))
				_com_issue_error( hr );
		}
	}
	catch (_com_error &ce)
	{
		return ce.Error();
	}

	return S_OK;
}

// StationService Members

void STDMETHODCALLTYPE CStationService::EnumFileStreams(BSTR bsFileName, IDispatchPtr pStreamCallback)
{
	WIN32_FIND_STREAM_DATA w32fsd;
	HANDLE hFindStream = FindFirstStreamW( bsFileName, FindStreamInfoStandard, &w32fsd, 0 );
	if (hFindStream == INVALID_HANDLE_VALUE)
	{
		const DWORD dwLastError = GetLastError();
		if (dwLastError == ERROR_HANDLE_EOF)
			return;

		_com_issue_error( __HRESULT_FROM_WIN32(dwLastError) );
	}

	HRESULT hr = S_OK;
	while (true)
	{
		_variant_t varStreamInfo[2], varProceed;
		DISPPARAMS dp = { varStreamInfo, NULL, 2, 0 };

		varStreamInfo[0] = w32fsd.StreamSize.QuadPart;
		varStreamInfo[1] = w32fsd.cStreamName;

		hr = pStreamCallback->Invoke( DISPID_VALUE, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, &varProceed, NULL, NULL );
		if (FAILED(hr))
			break;

		if (!varProceed)
			break;

		if (!FindNextStreamW( hFindStream, &w32fsd ))
		{
			const DWORD dwLastError = GetLastError();
			if (dwLastError != ERROR_HANDLE_EOF)
				hr = __HRESULT_FROM_WIN32(dwLastError);

			break;
		}
	}

	FindClose( hFindStream );

	if (FAILED(hr))
		_com_issue_error( hr );
}