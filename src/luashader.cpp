#include"luashader.h"
#include<shaderlib\BaseShader.h>
#include<materialsystem\ishaderapi.h>
#include<IShaderSystem.h>
#include<materialsystem\itexture.h>

// Blatantly copied from BaseShader.cpp in the SDK
static ShaderParamInfo_t s_StandardParams[NUM_SHADER_MATERIAL_VARS] =
{
	{ "$flags",				"flags",			SHADER_PARAM_TYPE_INTEGER,	"0", SHADER_PARAM_NOT_EDITABLE },
	{ "$flags_defined",		"flags_defined",	SHADER_PARAM_TYPE_INTEGER,	"0", SHADER_PARAM_NOT_EDITABLE },
	{ "$flags2",  			"flags2",			SHADER_PARAM_TYPE_INTEGER,	"0", SHADER_PARAM_NOT_EDITABLE },
	{ "$flags_defined2",	"flags2_defined",	SHADER_PARAM_TYPE_INTEGER,	"0", SHADER_PARAM_NOT_EDITABLE },
	{ "$color",		 		"color",			SHADER_PARAM_TYPE_COLOR,	"[1 1 1]", 0 },
	{ "$alpha",	   			"alpha",			SHADER_PARAM_TYPE_FLOAT,	"1.0", 0 },
	{ "$basetexture",  		"Base Texture with lighting built in", SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", 0 },
	{ "$frame",	  			"Animation Frame",	SHADER_PARAM_TYPE_INTEGER,	"0", 0 },
	{ "$basetexturetransform", "Base Texture Texcoord Transform",SHADER_PARAM_TYPE_MATRIX,	"center .5 .5 scale 1 1 rotate 0 translate 0 0", 0 },
	{ "$flashlighttexture",  		"flashlight spotlight shape texture", SHADER_PARAM_TYPE_TEXTURE, "effects/flashlight001", SHADER_PARAM_NOT_EDITABLE },
	{ "$flashlighttextureframe",	"Animation Frame for $flashlight",	SHADER_PARAM_TYPE_INTEGER, "0", SHADER_PARAM_NOT_EDITABLE },
	{ "$color2",		 		"color2",			SHADER_PARAM_TYPE_COLOR,	"[1 1 1]", 0 },
	{ "$srgbtint",		 		"tint value to be applied when running on new-style srgb parts",			SHADER_PARAM_TYPE_COLOR,	"[1 1 1]", 0 },
};

IShaderSystem *GetShaderSystem()
{
	extern IShaderSystem* g_pShaderSystem;
	return g_pShaderSystem;
}

CLuaShader::CLuaShader(const char *szName)
{
	strncpy(m_szName, szName, sizeof(m_szName));
}

char const* CLuaShader::GetName() const
{
	return m_szName;
}

char const* CLuaShader::GetFallbackShader(IMaterialVar** params) const
{
	return NULL;
}

int CLuaShader::GetNumParams() const
{
	return NUM_SHADER_MATERIAL_VARS + m_iExtraParamCount;
}

void CLuaShader::InitShaderParams(IMaterialVar** ppParams, const char *pMaterialName)
{
	Msg("InitShaderParams %s\n", pMaterialName);
}

void CLuaShader::InitShaderInstance(IMaterialVar** ppParams, IShaderInit *pShaderInit, const char *pMaterialName, const char *pTextureGroupName)
{
	for (int nPass = 0; nPass < m_iPassCount; nPass++)
	{
		LuaShaderPassInfo_t *passInfo = &m_pPassInfo[nPass];
		for (int i = 0; i < 16; i++)
		{
			int p = passInfo->m_iSamplers[i];
			if (p != -1)
			{
				IMaterialVar *param = ppParams[p];
				if (param && param->IsDefined())
					pShaderInit->LoadTexture(param, pTextureGroupName);
			}
		}
	}
}

void CLuaShader::DrawElements(IMaterialVar **params, int nModulationFlags,
		IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	for (int i = 0; i < m_iPassCount; i++)
	{
		DrawPass(params, pShaderShadow, pShaderAPI, i, vertexCompression);
	}
}

char const* CLuaShader::GetParamName(int paramIndex) const
{
	if (paramIndex < NUM_SHADER_MATERIAL_VARS)
		return s_StandardParams[paramIndex].m_pName;
	return m_pExtraParams[paramIndex - NUM_SHADER_MATERIAL_VARS].m_pName;
}

char const* CLuaShader::GetParamHelp(int paramIndex) const
{
	if (paramIndex < NUM_SHADER_MATERIAL_VARS)
		return s_StandardParams[paramIndex].m_pHelp;
	return m_pExtraParams[paramIndex - NUM_SHADER_MATERIAL_VARS].m_pHelp;
}

ShaderParamType_t CLuaShader::GetParamType(int paramIndex) const
{
	if (paramIndex < NUM_SHADER_MATERIAL_VARS)
		return s_StandardParams[paramIndex].m_Type;
	return m_pExtraParams[paramIndex - NUM_SHADER_MATERIAL_VARS].m_Type;
}

char const* CLuaShader::GetParamDefault(int paramIndex) const
{
	if (paramIndex < NUM_SHADER_MATERIAL_VARS)
		return s_StandardParams[paramIndex].m_pDefaultValue;
	return m_pExtraParams[paramIndex - NUM_SHADER_MATERIAL_VARS].m_pDefaultValue;
}

int CLuaShader::GetParamFlags(int paramIndex) const
{
	if (paramIndex < NUM_SHADER_MATERIAL_VARS)
		return s_StandardParams[paramIndex].m_nFlags;
	return m_pExtraParams[paramIndex - NUM_SHADER_MATERIAL_VARS].m_nFlags;
}

int CLuaShader::ComputeModulationFlags(IMaterialVar** params, IShaderDynamicAPI* pShaderAPI)
{
	return 0;
}

bool CLuaShader::NeedsPowerOfTwoFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame) const
{
	Msg("NeedsPowerOfTwoFrameBufferTexture\n");
	return false;
}

bool CLuaShader::NeedsFullFrameBufferTexture(IMaterialVar **params, bool bCheckSpecificToThisFrame) const
{
	Msg("NeedsFullFrameBufferTexture\n");
	return false;
}

bool CLuaShader::IsTranslucent(IMaterialVar **params) const
{
	Msg("IsTranslucent\n");
	return false;
}

int CLuaShader::GetFlags() const
{
	Error("GetFlags\n");
	return 0;
}

void CLuaShader::Draw(IShaderShadow* pShaderShadow)
{
	if (pShaderShadow)
	{
		GetShaderSystem()->TakeSnapshot();
	}
	else
	{
		GetShaderSystem()->DrawSnapshot(true);
	}
}

void CLuaShader::DrawPass(IMaterialVar **params, IShaderShadow* pShaderShadow,
	IShaderDynamicAPI* pShaderAPI, int nPass, VertexCompressionType_t vertexCompression)
{
	LuaShaderPassInfo_t *passInfo = &m_pPassInfo[nPass];

	if (pShaderShadow)
	{
		int fmt = VERTEX_POSITION;
		pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);

		for (int i = 0; i < 16; i++)
			if (passInfo->m_iSamplers[i] != -1)
				pShaderShadow->EnableTexture((Sampler_t)(SHADER_SAMPLER0 + i), true);

		// Shadow state
		pShaderShadow->SetVertexShader(passInfo->m_pVSH, 0);
		pShaderShadow->SetPixelShader(passInfo->m_pPSH, 0);
	}
	if (pShaderAPI)
	{
		for (int i = 0; i < 16; i++)
			if (passInfo->m_iSamplers[i] != -1)
				GetShaderSystem()->BindTexture((Sampler_t)(SHADER_SAMPLER0 + i), params[passInfo->m_iSamplers[i]]->GetTextureValue(), 0);

		for (int i = 0; i < 16; i++)
		{
			int p = passInfo->m_iPSHConsts[i];
			if (p != -1)
			{
				float values[4];
				IMaterialVar *param = params[p];
				if (param->GetType() == MATERIAL_VAR_TYPE_FLOAT)
					values[0] = param->GetFloatValue();
				else if (param->GetType() == MATERIAL_VAR_TYPE_VECTOR)
					param->GetVecValue(values, param->VectorSize());
				pShaderAPI->SetPixelShaderConstant(i, values);
			}
		}

		for (int i = 0; i < 16; i++)
		{
			int p = passInfo->m_iVSHConsts[i];
			if (p != -1)
			{
				float values[4];
				IMaterialVar *param = params[p];
				if (param->GetType() == MATERIAL_VAR_TYPE_FLOAT)
					values[0] = param->GetFloatValue();
				else if (param->GetType() == MATERIAL_VAR_TYPE_VECTOR)
					param->GetVecValue(values, param->VectorSize());
				pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0 + i, values);
			}
		}

		// Dynamic state
		pShaderAPI->SetVertexShaderIndex(0);
		pShaderAPI->SetPixelShaderIndex(0);
	}
	Draw(pShaderShadow);
}