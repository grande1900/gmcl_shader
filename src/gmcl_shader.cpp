#include<GarrysMod\Lua\Interface.h>
#include<GarrysMod\Lua\UserData.h>
#include<GarrysMod\Lua\Types.h>
#include<materialsystem\IShader.h>
#include<shaderlib\ShaderDLL.h>
#include<IShaderSystem.h>
#include"luashader.h"
#include<Windows.h>
#include<tier1\utlvector.h>
#include<tier1\lzmaDecoder.h>

using namespace GarrysMod::Lua;

IShaderSystem *g_pShaderSystem = NULL;
CUtlVector<CLuaShader*> LuaShaders;

typedef IShader *(__stdcall *FindShader)(char const* pShaderName);
FindShader oFindShader;
void *HookVTable(DWORD *vtable, int index, void *newFunc)
{
	DWORD dwOldProt;
	void *old;
	VirtualProtect(&vtable[index], 4, PAGE_EXECUTE_READWRITE, &dwOldProt);
	old = (void*)vtable[index];
	vtable[index] = (DWORD)newFunc;
	VirtualProtect(&vtable[index], 4, dwOldProt, &dwOldProt);
	return old;
}

void RestoreVTable(DWORD *vtable, int index, void *oldFunc)
{
	DWORD dwOldProt;
	VirtualProtect(&vtable[index], 4, PAGE_EXECUTE_READWRITE, &dwOldProt);
	vtable[index] = (DWORD)oldFunc;
	VirtualProtect(&vtable[index], 4, dwOldProt, &dwOldProt);
}

IShader *__stdcall NewFindShader(char const* pShaderName)
{
	IShader *ret = oFindShader(pShaderName);
	_asm pushad
	for (int i = 0; i < LuaShaders.Count(); i++)
	{
		CLuaShader *shader = LuaShaders[i];
		if (!strcmp(shader->GetName(), pShaderName))
		{
			ret = shader;
			break;
		}
	}
	_asm popad
	return ret;
}

int FindShaderParam(IShader *pShader, const char *pName)
{
	for (int i = 0; i < pShader->GetNumParams(); i++)
	{
		if (!strcmp(pShader->GetParamName(i), pName))
		{
			return i;
		}
	}
	return -1;
}

int LUA_CreateShader(lua_State *state)
{
	LUA->CheckType(1, Type::TABLE);
	LUA->GetField(1, "Name");
	const char *name = LUA->GetString(-1);
	CLuaShader *shader = new CLuaShader(name);
	LUA->Pop();

	LUA->GetField(1, "Params");
	LUA->PushNil();
	int paramCount = 0;
	while (LUA->Next(-2))
	{
		if (LUA->IsType(-1, Type::TABLE))
		{
			paramCount++;
		}
		LUA->Pop();
	}

	LUA->PushNil();
	ShaderParamInfo_t *params = new ShaderParamInfo_t[paramCount];
	int currentParam = 0;
	while (LUA->Next(-2))
	{
		if (LUA->IsType(-1, Type::TABLE))
		{
			LUA->PushNumber(1);
			LUA->GetTable(-2);
			const char *_name = LUA->GetString(-1);
			params[currentParam].m_pName = new char[strlen(_name)+1];
			strcpy((char*)params[currentParam].m_pName, _name);
			LUA->Pop();

			LUA->PushNumber(2);
			LUA->GetTable(-2);
			params[currentParam].m_Type = (ShaderParamType_t)int(LUA->GetNumber(-1));
			LUA->Pop();

			LUA->PushNumber(3);
			LUA->GetTable(-2);
			const char *_default = LUA->GetString(-1);
			params[currentParam].m_pDefaultValue = new char[strlen(_default)+1];
			strcpy((char*)params[currentParam].m_pDefaultValue, _default);
			LUA->Pop();

			params[currentParam].m_nFlags = 0;
			params[currentParam].m_pHelp = "";

			currentParam++;
		}
		LUA->Pop();
	}
	LUA->Pop();
	shader->SetupExtraParams(params, paramCount);

	LUA->GetField(1, "Passes");
	int passCount = 0;
	LUA->PushNumber(1);
	LUA->GetTable(-2);
	while (LUA->IsType(-1, Type::TABLE))
	{
		LUA->Pop();
		LUA->PushNumber(++passCount + 1);
	}
	LUA->Pop();

	LuaShaderPassInfo_t *passes = new LuaShaderPassInfo_t[passCount];
	int currentPass = 0;
	LUA->PushNumber(1);
	LUA->GetTable(-2);
	while (LUA->IsType(-1, Type::TABLE))
	{
		LUA->GetField(-1, "PSH");
		const char *psh = LUA->GetString(-1);
		passes[currentPass].m_pPSH = new char[strlen(psh)+1];
		strcpy(passes[currentPass].m_pPSH, psh);
		LUA->Pop();

		LUA->GetField(-1, "VSH");
		const char *vsh = LUA->GetString(-1);
		passes[currentPass].m_pVSH = new char[strlen(vsh)+1];
		strcpy(passes[currentPass].m_pVSH, vsh);
		LUA->Pop();

		LUA->GetField(-1, "Samplers");
		if (LUA->IsType(-1, Type::TABLE))
		{
			for (int i = 0; i < 16; i++)
			{
				LUA->PushNumber(i + 1);
				LUA->GetTable(-2);
				if (LUA->IsType(-1, Type::STRING))
				{
					const char *name = LUA->GetString(-1);
					passes[currentPass].m_iSamplers[i] = FindShaderParam(shader, name);
				}
				else
					passes[currentPass].m_iSamplers[i] = -1;
				LUA->Pop();
			}
		}
		else
			for (int i = 0; i < 16; i++)
				passes[currentPass].m_iSamplers[i] = -1;
		LUA->Pop();

		LUA->GetField(-1, "PSHConsts");
		if (LUA->IsType(-1, Type::TABLE))
		{
			for (int i = 0; i < 16; i++)
			{
				LUA->PushNumber(i + 1);
				LUA->GetTable(-2);
				if (LUA->IsType(-1, Type::STRING))
				{
					const char *name = LUA->GetString(-1);
					passes[currentPass].m_iPSHConsts[i] = FindShaderParam(shader, name);
				}
				else
					passes[currentPass].m_iPSHConsts[i] = -1;
				LUA->Pop();
			}
		}
		else
			for (int i = 0; i < 16; i++)
				passes[currentPass].m_iPSHConsts[i] = -1;
		LUA->Pop();

		LUA->GetField(-1, "VSHConsts");
		if (LUA->IsType(-1, Type::TABLE))
		{
			for (int i = 0; i < 10; i++)
			{
				LUA->PushNumber(i + 1);
				LUA->GetTable(-2);
				if (LUA->IsType(-1, Type::STRING))
				{
					const char *name = LUA->GetString(-1);
					passes[currentPass].m_iVSHConsts[i] = FindShaderParam(shader, name);
				}
				else
					passes[currentPass].m_iVSHConsts[i] = -1;
				LUA->Pop();
			}
		}
		else
			for (int i = 0; i < 16; i++)
				passes[currentPass].m_iVSHConsts[i] = -1;
		LUA->Pop();

		LUA->Pop();
		LUA->PushNumber(++currentPass + 1);
		LUA->GetTable(-2);
	}
	LUA->Pop();
	shader->SetupPasses(passes, passCount);

	LuaShaders.AddToTail(shader);
	return 0;
}

GMOD_MODULE_OPEN()
{
	CreateInterfaceFn materialsystem = Sys_GetFactory("materialsystem.dll");
	g_pShaderSystem = (IShaderSystem*)materialsystem(SHADERSYSTEM_INTERFACE_VERSION, NULL);
	oFindShader = (FindShader)HookVTable(*(DWORD**)g_pShaderSystem, 17, &NewFindShader);

	LUA->PushSpecial(SPECIAL_GLOB);

	LUA->CreateTable();
	LUA->PushCFunction(&LUA_CreateShader); LUA->SetField(-2, "Create");
	LUA->SetField(-2, "shader");

	LUA->PushNumber((int)SHADER_PARAM_TYPE_TEXTURE); LUA->SetField(-2, "SHADER_PARAM_TYPE_TEXTURE");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_INTEGER); LUA->SetField(-2, "SHADER_PARAM_TYPE_INTEGER");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_COLOR); LUA->SetField(-2, "SHADER_PARAM_TYPE_COLOR");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_VEC2); LUA->SetField(-2, "SHADER_PARAM_TYPE_VEC2");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_VEC3); LUA->SetField(-2, "SHADER_PARAM_TYPE_VEC3");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_VEC4); LUA->SetField(-2, "SHADER_PARAM_TYPE_VEC4");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_FLOAT); LUA->SetField(-2, "SHADER_PARAM_TYPE_FLOAT");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_BOOL); LUA->SetField(-2, "SHADER_PARAM_TYPE_BOOL");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_MATRIX); LUA->SetField(-2, "SHADER_PARAM_TYPE_MATRIX");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_MATERIAL); LUA->SetField(-2, "SHADER_PARAM_TYPE_MATERIAL");
	LUA->PushNumber((int)SHADER_PARAM_TYPE_STRING); LUA->SetField(-2, "SHADER_PARAM_TYPE_STRING");

	LUA->Pop();



	return 0;
}

GMOD_MODULE_CLOSE()
{
	RestoreVTable(*(DWORD**)g_pShaderSystem, 17, oFindShader);
	return 0;
}