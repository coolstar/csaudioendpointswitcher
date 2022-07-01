#include <mmdeviceapi.h>

interface DECLSPEC_UUID("CA286FC3-91FD-42C3-8E9B-CAAFA66242E3")
	IPolicyConfig10;

interface DECLSPEC_UUID("6BE54BE8-A068-4875-A49D-0C2966473B11")
	IPolicyConfig10_1;

interface DECLSPEC_UUID("F8679F50-850A-41CF-9C72-430F290290C8")
	IPolicyConfig7;
class DECLSPEC_UUID("870af99c-171d-4f9e-af0d-e63df40c2bc9")
	CPolicyConfigClient;

MIDL_INTERFACE("F8679F50-850A-41CF-9C72-430F290290C8")
IPolicyConfigWin7 : public IUnknown
{
private:
	virtual void Unused1(void);
	virtual void Unused2(void);
	virtual void Unused3(void);
	virtual void Unused4(void);
	virtual void Unused5(void);
	virtual void Unused6(void);
	virtual void Unused7(void);
	virtual void Unused8(void);
public:
	virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(
		/* [annotation][in] */
		_In_  LPCWSTR pwstrDeviceId,
		/* [annotation][in] */
		_In_ REFPROPERTYKEY key,
		/* [annotation][out] */
		_Out_ PROPVARIANT* pv) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(
		/* [annotation][in] */
		_In_  LPCWSTR pwstrDeviceId,
		/* [annotation][in] */
		_In_ REFPROPERTYKEY key,
		/* [annotation][in] */
		_In_ REFPROPVARIANT propvar) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(
		/* [annotation][in] */
		_In_  LPCWSTR pwstrDeviceId,
		/* [annotation][in] */
		_In_ ERole role) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(
		/* [annotation][in] */
		_In_  LPCWSTR pwstrDeviceId,
		/* [annotation][in] */
		_In_ INT isVisible) = 0;
};