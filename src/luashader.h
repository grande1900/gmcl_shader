#include<materialsystem\IShader.h>

struct LuaShaderPassInfo_t
{
	char *m_pPSH;
	char *m_pVSH;
	int m_iSamplers[16];
	int m_iPSHConsts[16];
	int m_iVSHConsts[16];
};

class CLuaShader : public IShader
{
public:
	CLuaShader(const char *szName);

	// Everything from IShader
	virtual char const* GetName() const;
	virtual char const* GetFallbackShader(IMaterialVar** params) const;
	virtual int GetNumParams() const;

	virtual void InitShaderParams(IMaterialVar** ppParams, const char *pMaterialName);
	virtual void InitShaderInstance(IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName);
	virtual void DrawElements(IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr);

	virtual char const* GetParamName(int paramIndex) const;
	virtual char const* GetParamHelp(int paramIndex) const;
	virtual ShaderParamType_t GetParamType(int paramIndex) const;
	virtual char const* GetParamDefault(int paramIndex) const;

	virtual int ComputeModulationFlags(IMaterialVar** params, IShaderDynamicAPI* pShaderAPI);
	virtual bool NeedsPowerOfTwoFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame = true) const;
	virtual bool NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame) const;
	virtual bool IsTranslucent(IMaterialVar **params) const;

	virtual int GetParamFlags(int paramIndex) const;

	virtual int GetFlags() const;

public:
	virtual void SetupExtraParams(ShaderParamInfo_t *pParams, int iCount) { m_pExtraParams = pParams; m_iExtraParamCount = iCount; }
	virtual void SetupPasses(LuaShaderPassInfo_t *pPasses, int iCount) { m_pPassInfo = pPasses; m_iPassCount = iCount; }

private:
	virtual void Draw(IShaderShadow* pShaderShadow);
	virtual void DrawPass(IMaterialVar **params, IShaderShadow* pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, int nPass, VertexCompressionType_t vertexCompression);

private:
	char m_szName[256];
	ShaderParamInfo_t *m_pExtraParams;
	int m_iExtraParamCount;
	LuaShaderPassInfo_t *m_pPassInfo;
	int m_iPassCount;
};