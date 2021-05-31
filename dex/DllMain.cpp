#pragma region include

/* D3D */
#include "D3dhook.h"
#include <DirectXMath.h>
#include "Vector3.h"
/* D3D */

/* Input */
#include "Input.h"
/* Input */

/* VC++ */
#include <iostream>
#include <fstream>
#include <string>
#include <winhttp.h>
#include <time.h>
#include <Windows.h>
#include <Tlhelp32.h>
#include <atlstr.h>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <winbase.h>
#include <vector>
#include <thread>
#include <string>
#include <ostream>
#include <bitset>
#include <tuple>
#include <mutex>
#include <array>
#include <algorithm>
/* VC++ */

/* BreakPoint */
#include "BreakPoint.h"
/* BreakPoint */

/* OFFSETS */
#include "Offset.h"
/* OFFSETS */

/* Kernel */
#include "communication.hpp"
#include "kernelinterface.hpp"
/* Kernel */

/* Config */
#include "config.h"
/* Config */

/* Utils */
#include "gamedata.hpp"
#include "Memory.h"
#include "Skin/Heroes.h"
#include "renderer.h"
#include "Skin/BotUtils.h"
#include "Memory.hpp"
#include "skCrypter.h"
#include "defs.h"
#include "load.h"
/* Utils */

#define M_PI       3.14159265358979323846
#define DEG2RAD(x) x * M_PI / 180.0
#pragma comment (lib, "urlmon.lib")
#pragma comment (lib, "winhttp.lib")
#include "Spoofcall.h"
#include "sha256.h"
#include "SendInput.h"
#include "VMP/VMProtectSDK.h"
using namespace DirectX;
#pragma endregion

#pragma region Declare
HINSTANCE g_Module;

Heroes eSkinEnum;
Heroes mSkinEnum;

DWORD64 viewMatrixPtr;
Matrix viewMatrix;
MatrixTo viewMatrixTo;

float Health, HealthMax, Armor, ArmorMax, Barrier, BarrierMax;

#define GravityForce 9.81f * 0.5f * Distance / Hanzo_BulletSpeed * Distance / Hanzo_BulletSpeed

Vector3 MyAngle, TargetAngle, EnPos, GetMyPos;

static Vector3 staticAngle;

std::string EnVisHook, EnFovHook, EnAngleHook, EnWallHook, EnVelocityHook;

DirectX::XMFLOAT3 MyXMAngle;

uint64_t AnglePTR;

struct Entity
{
	Vector3 Location, savedVelocity, Velocity, lastPos, rootPos, BonePos;
	bool Enemy, Alive, MyTeam;
	uint8_t VisCheck;
	clock_t lastVelocityUpdate;
	float PlayerHealth, eVec, Health, HealthMax, ARMOR, ARMORMAX, BARRIER, BARRIERMAX;
	uint64_t eParent, eHealthComponent;
	WORD HeroID, SkinID, MySkinID;

	uint64_t lasteParentPTR;
};

Entity Entitys[100];

vector<ULONG64>EntityPTR;

struct Color {
	int R, G, B, A;
};
#pragma endregion

#pragma region EncryptData
std::string encrypt(UINT64 ui64)
{
	return sha256(std::to_string(ui64));
}

void SaveEncrypted() // VMP 
{
	VMProtectBeginUltra("VEH");
	EnVisHook = encrypt(Config::Get().BaseAddress + offset::CompoenetHook);
	EnFovHook = encrypt(Config::Get().BaseAddress + offset::FovHook);
	EnAngleHook = encrypt(Config::Get().BaseAddress + offset::AngleHook);
	EnWallHook = encrypt(Config::Get().BaseAddress + offset::BorderLine);
	VMProtectEnd();
}
#pragma endregion

#pragma region Angle

Vector3 CalcAngle(Vector3 MyPos, Vector3 EnPos, float Dis)
{
	Vector3 Result;

	Result.x = (EnPos.x - MyPos.x) / Dis;
	Result.y = (EnPos.y - MyPos.y) / Dis;
	Result.z = (EnPos.z - MyPos.z) / Dis;

	return Result;
}

Vector3 GetAngle(Vector3 RAngle, Vector3 MPos, Vector3 EPos)
{
	float Distance = MPos.Distance(EPos);

	Vector3 Result;

	Result.x = (EPos.x - MPos.x) / Distance;
	Result.y = (EPos.y - MPos.y) / Distance;
	Result.z = (EPos.z - MPos.z) / Distance;

	return Result;
}

Vector3 SmoothAngle(Vector3 LocalAngle, Vector3 TargetAngle, float X_Speed, float Y_Speed)
{
	Vector3 Result;
	Result.x = (TargetAngle.x - LocalAngle.x) * X_Speed + LocalAngle.x;
	Result.y = (TargetAngle.y - LocalAngle.y) * Y_Speed + LocalAngle.y;
	Result.z = (TargetAngle.z - LocalAngle.z) * X_Speed + LocalAngle.z;

	return Result;
}
#pragma endregion

#pragma region NAMETAG
vector<MEMORY_BASIC_INFORMATION> mbis;
bool UpdateMemoryQuery()
{
	MEMORY_BASIC_INFORMATION mbi = { 0, };
	MEMORY_BASIC_INFORMATION old = { 0, };
	ULONG64 current_address = 0x7ffe0000;
	vector<MEMORY_BASIC_INFORMATION> addresses;
	while (true)
	{
		if (!VirtualQueryEx(GetCurrentProcess(), (PVOID)current_address, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
			break;
		if ((mbi.State & 0x1000) != 0 && (mbi.Protect & 0x100) == 0)
		{
			addresses.push_back(mbi);

			old = mbi;
		}
		current_address = ULONG64(mbi.BaseAddress) + mbi.RegionSize;
	}

	mbis = addresses;


	return (mbis.size() > 0);
}


ULONG64 FindPattern2(BYTE* buffer, BYTE* pattern, string mask, int bufSize)
{
	int pattern_len = mask.length();

	for (int i = 0; i < bufSize - pattern_len; i++)
	{
		bool found = true;
		for (int j = 0; j < pattern_len; j++)
		{
			if (mask[j] != '?' && pattern[j] != buffer[(i + j)])
			{
				found = false;
				break;
			}
		}
		if (found)
			return i;
	}
	return -1;
}

vector<ULONG64> FindPatternEx(ULONG64 start, ULONG64 end, BYTE* pattern, string mask, MEMORY_BASIC_INFORMATION mbi, ULONG64 RgSize)
{
	ULONG64 current_chunk = start;
	vector<ULONG64> found;
	if ((end - current_chunk > RgSize && RgSize != 0) || (end - current_chunk < RgSize && RgSize != 0))
		return found;
	while (current_chunk < end)
	{
		int bufSize = (int)(end - start);
		BYTE* buffer = new BYTE[bufSize];
		if (!ReadProcessMemory(GetCurrentProcess(), (LPVOID)current_chunk, buffer, bufSize, nullptr))
		{
			current_chunk += bufSize;
			delete[] buffer;
			continue;
		}

		ULONG64 internal_address = FindPattern2(buffer, pattern, mask, bufSize);
		if (internal_address != -1)
		{
			found.push_back(current_chunk + internal_address);
		}
		current_chunk += bufSize;
		delete[] buffer;

	}
	return found;
}

vector<ULONG64> _FindPatterns(BYTE* buffer, BYTE* pattern, string mask, int bufSize)
{
	vector<ULONG64> ret;
	int pattern_len = mask.length();
	for (int i = 0; i < bufSize - pattern_len; i++)
	{
		bool found = true;
		for (int j = 0; j < pattern_len; j++)
		{
			if (mask[j] != '?' && pattern[j] != buffer[i + j])
			{
				found = false;
				break;
			}
		}
		if (found)
			ret.push_back(i);
	}
	return ret;
}

ULONG64 FindPattern(BYTE* pattern, string mask, ULONG64 RgSize)
{
	if (!UpdateMemoryQuery())
		return 0;

	for (int i = 0; i < mbis.size(); i++) {
		MEMORY_BASIC_INFORMATION info = mbis[i];

		vector<ULONG64> arr = FindPatternEx(ULONG64(info.BaseAddress), info.RegionSize + ULONG64(info.BaseAddress), pattern, mask, info, RgSize);
		if (arr.size() > 0)
			return arr[0];
	}

	return 0;
}

vector<ULONG64> FindPatterns(BYTE* pattern, string mask, ULONG64 RgSize)
{
	vector<ULONG64> Result;
	ULONG64 PatternStart = FindPattern(pattern, mask, RgSize);
	if (PatternStart)
	{
		for (int i = 0; i < mbis.size(); i++)
		{
			if (ULONG64(mbis[i].BaseAddress) < PatternStart && PatternStart - ULONG64(mbis[i].BaseAddress) < mbis[i].RegionSize)
			{
				PatternStart = ULONG64(mbis[i].BaseAddress);
			}
		}

		BYTE* buf = new BYTE[RgSize];
		memcpy_s(buf, RgSize, PVOID(PatternStart), RgSize);

		vector<ULONG64> Pointers = _FindPatterns(buf, pattern, mask, RgSize);
		delete[] buf;

		for (int i = 0; i < Pointers.size(); i++)
			Pointers[i] += PatternStart;

		Result = Pointers;
	}

	return Result;
}

void Pointer() // 네임태그 포인터 쓰레드
{
	while (true)
	{
		EntityPTR = FindPatterns((PBYTE)"\xFA\x42\x00\x00\x00\x00\x00\x00\x00\x00\x01\x04\x1E\x00\x07", "xx????xxxxxxxxx", 0x16000);

		Sleep(25);
	}
}
#pragma endregion

#pragma region Thread

//unsigned __int64 __fastcall DecryptVis(__int64 a1)
//{
//	__int64 v1; // rbx
//	unsigned __int64 v2; // rdi
//	unsigned __int64 v3; // rax
//	__int64 v4; // rbx
//	unsigned __int64 v5; // rdx
//	unsigned __int64 v6; // rcx
//	__m128i v7; // xmm1
//	__m128i v8; // xmm2
//	__m128i v9; // xmm0
//	__m128i v10; // xmm1
//
//	v1 = a1;
//	v2 = Config::Get().BaseAddress + 0x5eec30; // 어레이검색후 첫번째
//
//	v3 = v2 + 0x8;
//
//	DWORD64* VisibleKeyPTR = (DWORD64*)(Config::Get().BaseAddress + 0x2bc4480); // 두번째 값
//	v4 = v2 ^ *(DWORD64*)((char*)&VisibleKeyPTR[((BYTE)v1 + 0x3C) & 0x7F]
//		+ (((unsigned __int64)(v1 - 0x6A0FD9FBE3F650C4i64) >> 7) & 7)) ^ (v1 - 0x6A0FD9FBE3F650C4i64);
//
//
//	v5 = (v3 - v2 + 7) >> 3;
//	v6 = 0i64;
//	if (v2 > v3)
//		v5 = 0i64;
//	if (v5)
//	{
//		if (v5 >= 4)
//		{
//			ZeroMemory(&v7, sizeof(v7));
//			ZeroMemory(&v8, sizeof(v8));
//			do
//			{
//				v6 += 4i64;
//				v7 = _mm_xor_si128(v7, _mm_loadu_si128((const __m128i*)v2));
//				v9 = _mm_loadu_si128((const __m128i*)(v2 + 16));
//				v2 += 0x20i64;
//				v8 = _mm_xor_si128(v8, v9);
//			} while (v6 < (v5 & 0xFFFFFFFFFFFFFFFCui64));
//			v10 = _mm_xor_si128(v7, v8);
//			v4 ^= *(DWORD64*)&_mm_xor_si128(v10, _mm_srli_si128(v10, 8));
//		}
//		for (; v6 < v5; ++v6)
//		{
//			v4 ^= *(DWORD64*)v2;
//			v2 += 8i64;
//		}
//	}
//	return v4 ^ ~v3 ^ 0x6A0FD9FBE3F650C4i64;
//}

#pragma endregion

#pragma region Function
void ReadView() // 뷰메 쓰레드
{
	uint64_t viewMatrixVal = Config::Get().RPM<uint64_t>(Config::Get().BaseAddress + offset::ViewMatrixOffset);
	viewMatrixVal = Config::Get().RPM<uint64_t>(viewMatrixVal + 0x3D8);
	viewMatrixVal = Config::Get().RPM<uint64_t>(viewMatrixVal + 0x560);
	viewMatrixVal = Config::Get().RPM<uint64_t>(viewMatrixVal + 0x478);
	viewMatrixVal = Config::Get().RPM<uint64_t>(viewMatrixVal + 0x60);
	viewMatrixPtr = viewMatrixVal + 0x470;
	viewMatrix = Config::Get().RPM<Matrix>(viewMatrixPtr);
	viewMatrixTo = Config::Get().RPM<MatrixTo>(viewMatrixPtr);
}

void StructT() //네임태그 쓰레드
{
	while (true)
	{
		vector<ULONG64>tempEntityPTR = EntityPTR;
		if (tempEntityPTR.size())
		{
			for (int i = 0; i < tempEntityPTR.size(); i++)
			{
				Entitys[i].Location = Config::Get().RPM<Vector3>(tempEntityPTR[i] + 0x5A);
				Entitys[i].Enemy = ((Config::Get().RPM<BYTE>(tempEntityPTR[i] + 0x4) == 0x8) || Config::Get().RPM<BYTE>(tempEntityPTR[i] + 0x4) == 0x99) ? true : false; // 팀구분 ? true : false;
				Entitys[i].Alive = (Config::Get().RPM<BYTE>(tempEntityPTR[i] + 0x4) != 0x60) && Config::Get().RPM<BYTE>(tempEntityPTR[i] + 0x4) != 0xB4 ? true : false;
				ReadView();
			}
			this_thread::sleep_for(1ms);
			viewMatrix = Config::Get().RPM<Matrix>(viewMatrixPtr);
			viewMatrixTo = Config::Get().RPM<MatrixTo>(viewMatrixPtr);
		}
		else
		{
			this_thread::sleep_for(5ms);
		}
	}
}

void AimCorrection(Vector3* InVecArg, Vector3 currVelocity, float Distance, float Bulletspeed, float Gravity)
{
	if (Config::Get().GravityBool)
	{
		float m_time = (Distance / Bulletspeed);

		(*InVecArg).x = (*InVecArg).x + ((currVelocity.x) * (Distance / (Bulletspeed)));
		(*InVecArg).y = (*InVecArg).y + ((currVelocity.y) * (Distance / (Bulletspeed)));
		(*InVecArg).z = (*InVecArg).z + ((currVelocity.z) * (Distance / (Bulletspeed)));

		(*InVecArg).y += (0.5f * 9.81f * m_time * m_time);
	}
	else
	{
		(*InVecArg).x = (*InVecArg).x + ((currVelocity.x) * (Distance / (Bulletspeed)));
		(*InVecArg).y = (*InVecArg).y + ((currVelocity.y) * (Distance / (Bulletspeed)));
		(*InVecArg).z = (*InVecArg).z + ((currVelocity.z) * (Distance / (Bulletspeed)));
	}
}

//struct RaytraceIn
//{
//	D3DXQUATERNION Coord1 = { 0,0,0,0 }; //myCoord
//	D3DXQUATERNION Coord2 = { 0,0,0,0 }; //enemyCoord
//	float var20 = 1;
//	float var24 = 1;
//	unsigned long var28 = 0;
//	unsigned long var2C = 0;
//	D3DXQUATERNION unknownCoord = { 0,0,0,0 }; //0x30
//	uint64_t var40 = 0;
//	uint64_t var48 = 0;
//	uint64_t var50 = 0;
//	uint64_t var58 = 0;
//	uint64_t* var60 = &var70;
//	uint32_t var68 = 0;
//	uint32_t var6C = 0x80000004;
//	uint64_t var70 = 0;
//	uint64_t var78 = 0;
//	uint64_t var80;
//	uint64_t var88;
//	uint64_t* var90 = &varA0;
//	uint32_t var98 = 0;
//	uint32_t var9C = 0x80000004;
//	uint64_t varA0 = 0;
//	uint64_t varA8 = 0;
//	uint64_t varB0 = 0;
//	uint64_t varB8 = 0;
//	uint64_t varC0 = 0;
//	uint64_t varC8 = 0;
//	uint64_t* varD0 = &varE0;
//	uint32_t varD8 = 0;
//	uint32_t varDC = 0x80000008;
//	uint64_t varE0 = 0;
//	uint64_t* varE8 = &var100;
//	uint64_t varF0 = 0;
//	uint64_t varF8 = 0;
//	uint64_t var100 = 0;
//	uint64_t var108 = 0;
//	uint64_t var110 = 0;
//	uint64_t var118 = 0;
//	uint64_t var120;
//	uint64_t var128;
//	uint64_t var130;
//	uint64_t var138;
//	uint64_t var140;
//	uint64_t var148;
//	uint64_t var150;
//	uint64_t var158;
//	uint64_t var160 = 0;
//	uint64_t var168 = 0;
//	uint64_t var170 = 0;
//	uint64_t var178 = 0;
//	uint64_t var180 = 0;
//	uint64_t var188 = 0;
//	uint64_t var190 = 0;
//	uint32_t var198 = 0x10000;
//	uint32_t var19c = 0;
//	uint64_t var1A0 = 0;
//	uint64_t var1A8 = 0;
//	uint64_t var1B0 = 0;
//	uint64_t var1B8 = 0;
//	uint64_t var1C0 = 0;
//	uint64_t var1C8 = 0;
//	uint64_t var1D0 = 0;
//	uint64_t var1D8 = 0;
//	uint32_t var1E0 = 0;
//	uint32_t var1E4 = 0;
//	uint32_t var1E8 = 0;
//	uint64_t var1F0 = 0;
//	uint64_t var1F8 = 0;
//	uint64_t var200 = 0;
//	uint64_t var208 = 0;
//	uint64_t var210 = 0;
//	uint64_t var218 = 0;
//	uint64_t var220 = 0;
//	uint64_t var228 = 0;
//	uint64_t var230 = 0;
//	uint64_t var238 = 0;
//	uint32_t var240 = 0;
//	uint32_t var244 = 0;
//	uint64_t var248 = 0;
//	uint32_t var250 = 0;
//};
//
//struct RaytraceOut
//{
//	D3DXQUATERNION Coord1 = { 0,0,0,0 };
//	D3DXQUATERNION Coord2 = { 0,0,0,0 };
//	unsigned __int64 unknown0 = 0;
//	unsigned __int32 unknown1 = 0xFFFFFFFF;
//	unsigned __int32 unknown2 = 0xFFFF0000;
//	float unknown3 = 1;
//	unsigned long unknown4 = -1;
//	unsigned short unknown5 = -1;
//	unsigned short unknown6 = -1;
//	unsigned long unknown7 = 1;
//	unsigned __int64 unknown8 = 0;
//	unsigned __int64 unknown9 = 0;
//	unsigned __int64 unknown10 = 0;
//	unsigned __int64 unknown11 = 0;
//};
//
//typedef int(*pConstructSetting)(unsigned __int64** Parameter1, unsigned __int64* Parameter2);
//typedef int(*pRayForce)(RaytraceIn* Parameter1, RaytraceOut* Parameter2, DWORD a1);
//
//int RayForce(uint64_t target_player_address, D3DXVECTOR3 targetPos, uint64_t flag, RaytraceOut& RTOut)
//{
//	RaytraceIn RTIn;
//
//	pConstructSetting xSetting = (pConstructSetting)(Config::Get().BaseAddress + 0x1B8FCA0); //E8 ? ? ? ? 41 3B 7E 08
//	pRayForce Rayforce = (pRayForce)(Config::Get().BaseAddress + 0xBBA0A0); //E8 ? ? ? ? 8B F8 48 8B 76 20
//
//	RTIn.var1B8 = //get value for debugging "Overwatch.exe+BBA0A0" rcx+1B8
//		RTIn.var1C0 = //get value for debugging "Overwatch.exe+BBA0A0" rcx+1C0
//
//		uint64_t local = // you can get value
//
//		if (local == 0)
//			return -1;
//
//	xSetting(&RTIn.var60, &local);
//	xSetting(&RTIn.var60, &target_player_address);
//
//	D3DXVECTOR3 camera = getCameraLocation(); //camera location
//
//	RTIn.Coord1 = D3DXQUATERNION(camera.x, camera.y, camera.z, 0);
//	RTIn.Coord2 = D3DXQUATERNION(targetPos.x, targetPos.y, targetPos.z, 0);
//	RTIn.var58 = flag;
//
//	return Rayforce(&RTIn, &RTOut, 0);
//}

void RemovePeHeader(HMODULE hModule)
{
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader + (DWORD)pDosHeader->e_lfanew);

	if (pNTHeader->FileHeader.SizeOfOptionalHeader)
	{
		DWORD Protect;
		WORD Size = pNTHeader->FileHeader.SizeOfOptionalHeader;
		VirtualProtect((void*)hModule, Size, PAGE_EXECUTE_READWRITE, &Protect);
		SecureZeroMemory((void*)hModule, Size);
		VirtualProtect((void*)hModule, Size, Protect, &Protect);
	}
}

void GetKey(uint64_t* a0, uint64_t* a1) // 스푸퍼
{
	spoof_call((PVOID)(Config::Get().BaseAddress + 0x1e2c310), reinterpret_cast<uint64_t(__fastcall*)(uint64_t*, uint64_t*)>(Config::Get().BaseAddress + 0xb648c), a0, a1);//패치
}

uint64_t fnDecryptChildInParent(uint64_t parent, uint8_t compid) // 컴포넌트 복호화
{
	unsigned __int64 v1 = parent;
	unsigned __int64 v2 = (uint64_t)1 << (uint64_t)(compid & 0x3F);
	unsigned __int64 v3 = v2 - 1;
	unsigned __int64 v4 = compid & 0x3F;
	unsigned __int64 v5 = compid / 0x3F;
	unsigned __int64 v6 = Config::Get().RPM<uint64_t>(v1 + 8 * (uint32_t)v5 + 0x88);
	__int64 v7 = (v2 & Config::Get().RPM<uint64_t>(v1 + 8 * (uint32_t)v5 + 0x88)) >> v4;
	unsigned __int64 v8 = (v3 & v6) - (((v3 & v6) >> 1) & 0x5555555555555555);
	unsigned __int64* v9 = (uint64_t*)(Config::Get().RPM<uint64_t>(v1 + 0x58) + 8 * (Config::Get().RPM<uint8_t>((uint32_t)v5 + v1 + 0xA8) + ((0x101010101010101 * (((v8 & 0x3333333333333333) + ((v8 >> 2) & 0x3333333333333333) + (((v8 & 0x3333333333333333) + ((v8 >> 2) & 0x3333333333333333)) >> 4)) & 0xF0F0F0F0F0F0F0F)) >> 0x38)));
	uint64_t Key1 = 0xE0102D93977B818C;// 키패치1
	uint64_t Key2 = 0x24379DBC304A997D;// 키패치2
	GetKey(&Key2, &Key1); // 우회
	uint64_t v10 = *v9;
	uint64_t v11 = (unsigned int)v10 | ((~(unsigned int)v10 ^ __ROR4__(*(_QWORD*)(Config::Get().BaseAddress + 0x2CE9900 + (*(_WORD*)&Key1 & 0xFFF)), 11) ^ (v10 >> 32)) << 32);//패치
	uint64_t v12 = Key2 ^ ((unsigned int)v11 | (((unsigned int)(-970729662 - v11) ^ (v11 >> 32)) << 32));//패치
	uint64_t v13 = (unsigned int)v12 | ((((unsigned int)v12 - (unsigned int)*(_QWORD*)(Config::Get().BaseAddress + 0x2CE9900 + (*(_QWORD*)&Key1 >> 52))) ^ (v12 >> 32)) << 32);//패치
	uint64_t v14 = -(int)v7 & ((unsigned int)v13 | (((unsigned int)(v13 - (*(_QWORD*)(Config::Get().BaseAddress + 0x2CE9900 + (*(_WORD*)&Key1 & 0xFFF)) >> 32)) ^ (v13 >> 32)) << 32));//패치
	return v14;
}

uint64_t GetComponent(uint64_t ComponentParent, DWORD ComponentID) // 컴포넌트 불러오기
{
	__try
	{
		uint64_t Result = 0;

		uint64_t pPointerTable = ComponentParent + 0x58;

		uint64_t PointerTable = Config::Get().RPM<uint64_t>(pPointerTable + 0x0);
		uint32_t PointerCount = Config::Get().RPM<uint32_t>(pPointerTable + 0x8);

		if (IsBadReadPtr((PVOID)(PointerTable), 0x8 * PointerCount))
			return 0;

		for (int i = 0; i < PointerCount; i++)
		{
			uint64_t CurrentComponent = fnDecryptChildInParent(ComponentParent, ComponentID);

			if (!CurrentComponent || IsBadReadPtr((PVOID)CurrentComponent, 0x510))
				continue;

			if (*(BYTE*)(CurrentComponent + 0x10) == ComponentID)
			{
				Result = CurrentComponent;
				break;
			}
		}

		return Result;
	}
	__except (1)
	{
		return 0;
	}
}

unsigned long RGBA2ARGB(int r, int g, int b, int a)
{
	return ((a & 0xFF) << 24) + ((b & 0xFF) << 16) + ((g & 0xFF) << 8) + (r & 0xFF);
}

//void Hook(EXCEPTION_POINTERS* ExceptionInfo)
//{
//	auto ctx = ExceptionInfo->ContextRecord;
//	GameData& data = GameData::Get();
//	ctx->Rbp = ctx->Rax;
//	static DWORD64 count = NULL;
//	uint64_t ComponentBase = ctx->Rax;
//	uint64_t ComponentParent = ctx->Rsi;
//	byte ComponentIdx = LOBYTE(ctx->Rcx);
//	++count;
//
//	if (ComponentIdx == OFFSET_PLAYER_VISIBILITY)
//	{
//		//Utils::ConsolePrint("VISIBILITY[%d] : %p - %p\n", rcx, rbp, rdi);
//		OWCOMPONENT c = OWCOMPONENT();
//		c.componentBase = ComponentBase;
//		c.componentParentPtr = ComponentParent;
//		c.componentType = OFFSET_PLAYER_VISIBILITY;
//		data.visibilityComponentList.push_back(c);
//	}
//	else if (ComponentIdx == OFFSET_UNIT_HEALTH)
//	{
//		//Utils::ConsolePrint("HEALTH[%d] : %p - %p\n", rcx, rbp, rdi);
//		OWCOMPONENT c = OWCOMPONENT();
//		c.componentBase = ComponentBase;
//		c.componentParentPtr = ComponentParent;
//		c.componentType = OFFSET_UNIT_HEALTH;
//
//		data.healthComponentList.push_back(c);
//	}
//}

int TarGetIndex = -1;

Vector3 GetVector3Predit()
{
	Vector3 target = Vector3(0, 0, 0);
	Vector2 CrossHair = Vector2(1920 / 2.0f, 1080 / 2.0f);

	float origin = 100000.f;

	if (TarGetIndex == -1)
	{
		if (EntityPTR.size())
		{
			for (int i = 0; i < EntityPTR.size(); i++)
			{
				if (Entitys[i].Alive && Entitys[i].Enemy)
				{
					Vector3 world = Entitys[i].BonePos;
					AimCorrection(&world, Entitys[i].Velocity, viewMatrix.GetCameraVec().Distance(world), Config::Get().PreditLevel, 9.8f);

					Vector2 Vec2 = viewMatrix.WorldToScreen(world, 1920, 1080);
					Vector2 RealVe2 = Vector2(Vec2.x - CrossHair.x, Vec2.y - CrossHair.y);
					float CrossDist = CrossHair.Distance(Vec2);
					if (CrossDist < origin && CrossDist < Config::Get().Fov)
					{
						target = world;
						origin = CrossDist;
						TarGetIndex = i;
					}
					else
					{
						TarGetIndex = -1;
					}
				}
				else
				{
					TarGetIndex = -1;
				}

			}
		}
	}
	else
	{
		if (Entitys[TarGetIndex].Alive && Entitys[TarGetIndex].Enemy)
		{
			Vector3 world = Entitys[TarGetIndex].BonePos;
			AimCorrection(&world, Entitys[TarGetIndex].Velocity, viewMatrix.GetCameraVec().Distance(world), Config::Get().PreditLevel, 9.8f);

			Vector2 Vec2 = viewMatrix.WorldToScreen(world, 1920, 1080);

			float CrossDist = CrossHair.Distance(Vec2);
			if (CrossDist < origin && CrossDist < Config::Get().Fov)
			{
				target = world;
				origin = CrossDist;
			}
			else
			{
				TarGetIndex = -1;

			}
		}
		else
		{
			TarGetIndex = -1;
		}
	}

	return target;
}

Vector3 GetVector3()
{
	Vector3 target{};
	Vector2 CrossHair = Vector2(1920 / 2.0f, 1080 / 2.0f);

	float origin = 100000.f;

	if (TarGetIndex == -1)
	{
		if (EntityPTR.size())
		{
			for (int i = 0; i < EntityPTR.size(); i++)
			{
				if (Entitys[i].Alive && Entitys[i].Enemy)
				{
					Vector2 Vec2 = viewMatrix.WorldToScreen(Entitys[i].BonePos, 1920, 1080);
					float CrossDist = CrossHair.Distance(Vec2);

					if (CrossDist < origin && CrossDist < Config::Get().Fov)
					{
						target = Entitys[i].BonePos;
						origin = CrossDist;
						TarGetIndex = i;
					}
					else
					{
						TarGetIndex = -1;
					}
				}
				else
				{
					TarGetIndex = -1;
				}

			}
		}
	}
	else
	{
		if (Entitys[TarGetIndex].Alive && Entitys[TarGetIndex].Enemy)
		{
			Vector2 Vec2 = viewMatrix.WorldToScreen(Entitys[TarGetIndex].BonePos, 1920, 1080);
			float CrossDist = CrossHair.Distance(Vec2);

			if (CrossDist < origin && CrossDist < Config::Get().Fov)
			{
				target = Entitys[TarGetIndex].BonePos;
				origin = CrossDist;
			}
			else
			{
				TarGetIndex = -1;

			}
		}
		else
		{
			TarGetIndex = -1;
		}
	}

	return target;
}

int TarGetIndex2 = -1;

Vector3 GetVector3123123()
{
	Vector3 target{};
	Vector2 CrossHair = Vector2(1920 / 2.0f, 1080 / 2.0f);

	float origin = FLT_MAX;

	if (EntityPTR.size())
	{
		for (int i = 0; i < EntityPTR.size(); i++)
		{
			Vector3 world = Entitys[i].BonePos;
			AimCorrection(&world, Entitys[i].Velocity, viewMatrix.GetCameraVec().Distance(world), Config::Get().PreditLevel, 9.8f);
			if (Entitys[i].Alive && Entitys[i].Enemy)
			{
				Vector2 Vec2 = viewMatrix.WorldToScreen(world, 1920, 1080);
				float CrossDist = viewMatrix.GetCameraVec().Distance(world);

				if (CrossDist < origin)
				{
					target = world;
					origin = CrossDist;
					TarGetIndex2 = i;
				}
			}

		}
	}
	return target;
}

Vector3 GetVector3123123Health()
{
	Vector3 target{};
	Vector2 CrossHair = Vector2(1920 / 2.0f, 1080 / 2.0f);

	float origin = FLT_MAX;

	if (EntityPTR.size())
	{
		for (int i = 0; i < EntityPTR.size(); i++)
		{
			Vector3 world = Entitys[i].BonePos;
			AimCorrection(&world, Entitys[i].Velocity, viewMatrix.GetCameraVec().Distance(world), Config::Get().PreditLevel, 9.8f);
			if (Entitys[i].Alive && Entitys[i].Enemy && Entitys[i].PlayerHealth <= 55.f)
			{
				Vector2 Vec2 = viewMatrix.WorldToScreen(world, 1920, 1080);
				float CrossDist = viewMatrix.GetCameraVec().Distance(world);

				if (CrossDist < origin)
				{
					target = world;
					origin = CrossDist;
					TarGetIndex2 = i;
				}
			}

		}
	}
	return target;
}

Vector3 GetVector3123123Health1()
{
	Vector3 target{};
	Vector2 CrossHair = Vector2(1920 / 2.0f, 1080 / 2.0f);

	float origin = FLT_MAX;

	if (EntityPTR.size())
	{
		for (int i = 0; i < EntityPTR.size(); i++)
		{
			Vector3 world = Entitys[i].BonePos;
			if (Entitys[i].Alive && Entitys[i].Enemy && Entitys[i].PlayerHealth <= 35.f)
			{
				Vector2 Vec2 = viewMatrix.WorldToScreen(world, 1920, 1080);
				float CrossDist = viewMatrix.GetCameraVec().Distance(world);

				if (CrossDist < origin)
				{
					target = world;
					origin = CrossDist;
					TarGetIndex2 = i;
				}
			}

		}
	}
	return target;
}

std::array<HeroID_Bone, MAX_PATH> Head_HeroIDs
{
	HeroID_Bone("Ana", 0x02E000000000013B, 67),
	HeroID_Bone("Ashe", 0x02E0000000000200, 125),
	HeroID_Bone("Baptiste", 0x02E0000000000221, 167),
	HeroID_Bone("Bastion", 0x02E0000000000015, 81),
	HeroID_Bone("Brigitte", 0x2E0000000000195, 77),
	HeroID_Bone("Doomfist", 0x02E000000000012F, 83),
	HeroID_Bone("Dva", 0x02E000000000007A, 140), // 140
	HeroID_Bone("Echo", 0x02E0000000000206, 42),
	HeroID_Bone("Genji", 0x02E0000000000029, 53),
	HeroID_Bone("Hanzo", 0x02E0000000000005, 38),
	HeroID_Bone("Junkrat", 0x02E0000000000065, 100),
	HeroID_Bone("Lucio", 0x02E0000000000079, 53),
	HeroID_Bone("Mccree", 0x02E0000000000042, 53),
	HeroID_Bone("Mei", 0x02E00000000000DD, 50),
	HeroID_Bone("Mercy", 0x02E0000000000004, 150),
	HeroID_Bone("Moira", 0x02E00000000001A2, 64),
	HeroID_Bone("Orisa", 0x02E000000000013E, 50),
	HeroID_Bone("Pharah", 0x02E0000000000008, 38),
	HeroID_Bone("Reaper", 0x02E0000000000002, 36),
	HeroID_Bone("Reinhardt", 0x02E0000000000007, 41),
	HeroID_Bone("Roadhog", 0x02E0000000000040, 61),
	HeroID_Bone("Soldier", 0x02E000000000006E, 38),
	HeroID_Bone("Sombra", 0x02E000000000012E, 42),
	HeroID_Bone("Sigma", 0x02E000000000023B, 180), //180
	HeroID_Bone("Symmetra", 0x02E0000000000016, 89),
	HeroID_Bone("Torbjorn", 0x02E0000000000006, 45),
	HeroID_Bone("Tracer", 0x02E0000000000003, 52),
	HeroID_Bone("Widowmaker", 0x02E000000000000A, 40),
	HeroID_Bone("Winston", 0x02E0000000000009, 109),
	HeroID_Bone("WreckingBall", 0x02E00000000001CA, 199),
	HeroID_Bone("Zarya", 0x02E0000000000068, 65),
	HeroID_Bone("Zenyatta", 0x02E0000000000020, 149),
	HeroID_Bone("TrainingBot1", 0x02E000000000016B, 37),
	HeroID_Bone("TrainingBot2", 0x02E000000000016C, 37),
	HeroID_Bone("TrainingBot3", 0x02E000000000016D, 37),
	HeroID_Bone("TrainingBot4", 0x02E000000000016E, 37),
};

std::array<pair<WORD, int>, 550> Head_SkinIDs
{
	//Ana:
pair<WORD, int>(0x1921,67),
pair<WORD, int>(0x1924,67),
pair<WORD, int>(0x1925,67),
pair<WORD, int>(0x1922,67),
pair<WORD, int>(0x1926,67),
pair<WORD, int>(0x194B,68),
pair<WORD, int>(0x1923,67),
pair<WORD, int>(0x1927,67),
pair<WORD, int>(0x199C,67),
pair<WORD, int>(0x1B11,67),
pair<WORD, int>(0x194A,68),
pair<WORD, int>(0x1950,68),
pair<WORD, int>(0x1951,68),
pair<WORD, int>(0x1BB8,64),
pair<WORD, int>(0x1B0F,75),
pair<WORD, int>(0x1C0F,68),
pair<WORD, int>(0x4570, 117),
//Ashe:
pair<WORD, int>(0x1ED3,125),
pair<WORD, int>(0x21CF,111),
pair<WORD, int>(0x2A02,125),
pair<WORD, int>(0x2A01,125),
pair<WORD, int>(0x2A04,125),
pair<WORD, int>(0x2A03,125),
pair<WORD, int>(0x2A06,125),
pair<WORD, int>(0x2A05,125),
pair<WORD, int>(0x2A10,125),
pair<WORD, int>(0x2A07,111),
pair<WORD, int>(0x2A09,108),
pair<WORD, int>(0x21D2,108),
pair<WORD, int>(0x2A13,122),
//Baptiste:
pair<WORD, int>(0x2188,66),
pair<WORD, int>(0x2C8A,66),
pair<WORD, int>(0x2C87,66),
pair<WORD, int>(0x2C89,66),
pair<WORD, int>(0x2C88,66),
pair<WORD, int>(0x2C92,66),
pair<WORD, int>(0x2C90,69),
pair<WORD, int>(0x2CDD,50),
pair<WORD, int>(0x2C77,50),
pair<WORD, int>(0x2C7F,52),
pair<WORD, int>(0x2CDB,52),
pair<WORD, int>(0x2C7A,166),
//Bastion:
pair<WORD, int>(0x1627,81),
pair<WORD, int>(0x1628,81),
pair<WORD, int>(0x1629,81),
pair<WORD, int>(0x18CC,81),
pair<WORD, int>(0x18CD,81),
pair<WORD, int>(0x2A46,81),
pair<WORD, int>(0x1799,98),
pair<WORD, int>(0x162A,81),
pair<WORD, int>(0x18CE,81),
pair<WORD, int>(0x199F,81),
pair<WORD, int>(0x1AE8,81),
pair<WORD, int>(0x1B93,81),
pair<WORD, int>(0x1798,98),
pair<WORD, int>(0x1795,109),
pair<WORD, int>(0x1794,109),
pair<WORD, int>(0x1C3E,99),
pair<WORD, int>(0x1B90,96),
pair<WORD, int>(0x1E97,97),
pair<WORD, int>(0x1623,106),
pair<WORD, int>(0x2A58,86),
//Brigitte:
pair<WORD, int>(0x1BCE,77),
pair<WORD, int>(0x1EAB,77),
pair<WORD, int>(0x1EAA,77),
pair<WORD, int>(0x1EA8,77),
pair<WORD, int>(0x1EA9,77),
pair<WORD, int>(0x1EA5,77),
pair<WORD, int>(0x1EAD,75),
pair<WORD, int>(0x2131,77),
pair<WORD, int>(0x2A54,121),
pair<WORD, int>(0x1EC2,70),
pair<WORD, int>(0x1E80,70),
pair<WORD, int>(0x1E81,73),
pair<WORD, int>(0x1EC3,73),
pair<WORD, int>(0x1EBC,61),
//Doomfist:
pair<WORD, int>(0x160F,83),
pair<WORD, int>(0x1610,83),
pair<WORD, int>(0x18C3,83),
pair<WORD, int>(0x1611,83),
pair<WORD, int>(0x1612,83),
pair<WORD, int>(0x18C5,83),
pair<WORD, int>(0x18C4,83),
pair<WORD, int>(0x1C14,64),
pair<WORD, int>(0x1BE3,65),
pair<WORD, int>(0x1BD9,65),
pair<WORD, int>(0x1BE4,81),
//Dva:

pair<WORD, int>(0x1613, 151),
pair<WORD, int>(0x1616,151),
pair<WORD, int>(0x18C6,151),
pair<WORD, int>(0x197A,151),
pair<WORD, int>(0x1764,72),
pair<WORD, int>(0x1615, 151),
pair<WORD, int>(0x1614, 151),
pair<WORD, int>(0x18C7, 151),
pair<WORD, int>(0x18C8, 151),
pair<WORD, int>(0x17A0, 75),
pair<WORD, int>(0x17A1, 75),
pair<WORD, int>(0x1765, 72),
pair<WORD, int>(0x1C3D, 88),
pair<WORD, int>(0x1BB0, 71),
pair<WORD, int>(0x1EA2, 44),
pair<WORD, int>(0x1A19, 92),
pair<WORD, int>(0x196C, 71),

// Echo
pair<WORD, int>(0x20E0, 42),
pair<WORD, int>(0x46C3, 42),
pair<WORD, int>(0x46C6, 42),
pair<WORD, int>(0x46C4, 42),
pair<WORD, int>(0x46C5, 42),
pair<WORD, int>(0x46C7, 42),
pair<WORD, int>(0x46C8, 42),
pair<WORD, int>(0x46B7, 53),
pair<WORD, int>(0x46E4, 53),
pair<WORD, int>(0x46BC, 43),
pair<WORD, int>(0x46D4, 43),
pair<WORD, int>(0x47A6, 51),
pair<WORD, int>(0x4733, 55),

//Genji:
pair<WORD, int>(0x16D7, 53),
pair<WORD, int>(0x16D8, 53),
pair<WORD, int>(0x18EA, 53),
pair<WORD, int>(0x16D9, 53),
pair<WORD, int>(0x16DA, 53),
pair<WORD, int>(0x18EB, 53),
pair<WORD, int>(0x1785, 55),
pair<WORD, int>(0x1789, 60),
pair<WORD, int>(0x1994, 47),
pair<WORD, int>(0x18EC, 53),
pair<WORD, int>(0x197C, 53),
pair<WORD, int>(0x1784, 55),
pair<WORD, int>(0x1788, 60),
pair<WORD, int>(0x1C1D, 54),
pair<WORD, int>(0x1B7D, 42),
pair<WORD, int>(0x1B91, 57),
//hanzo:
pair<WORD, int>(0x15EB, 38),
pair<WORD, int>(0x15ED, 38),
pair<WORD, int>(0x18B4, 38),
pair<WORD, int>(0x15EC, 38),
pair<WORD, int>(0x15EE, 38),
pair<WORD, int>(0x18B6, 38),
pair<WORD, int>(0x18B5, 38),
pair<WORD, int>(0x19A7, 38),
pair<WORD, int>(0x19B4, 34),
pair<WORD, int>(0x15E3, 40),
pair<WORD, int>(0x15E4, 40),
pair<WORD, int>(0x15E8, 55),
pair<WORD, int>(0x15E7, 55),
pair<WORD, int>(0x19B2, 46),
pair<WORD, int>(0x2129, 42),
pair<WORD, int>(0x1C28, 33),
pair<WORD, int>(0x19B0, 33),
//Junkrat:
pair<WORD, int>(0x1607, 100),
pair<WORD, int>(0x1608, 100),
pair<WORD, int>(0x1609, 100),
pair<WORD, int>(0x18BD, 100),
pair<WORD, int>(0x1A2D, 106),
pair<WORD, int>(0x179C, 149),
pair<WORD, int>(0x1962, 137),
pair<WORD, int>(0x160A, 100),
pair<WORD, int>(0x1C4D, 100),
pair<WORD, int>(0x18BE, 100),
pair<WORD, int>(0x18BF, 100),
pair<WORD, int>(0x179D, 149),
pair<WORD, int>(0x177D, 84),
pair<WORD, int>(0x177C, 84),
pair<WORD, int>(0x1BC1, 128),
pair<WORD, int>(0x20E7, 146),
pair<WORD, int>(0x1C13, 99),
pair<WORD, int>(0x1C73, 155),
pair<WORD, int>(0x2CE5, 100),
pair<WORD, int>(0x41F5, 108),
//Lucio:
pair<WORD, int>(0x160B, 52),
pair<WORD, int>(0x18C0, 52),
pair<WORD, int>(0x160C, 52),
pair<WORD, int>(0x160E, 52),
pair<WORD, int>(0x18C2, 52),
pair<WORD, int>(0x18C1, 52),
pair<WORD, int>(0x1778, 52),
pair<WORD, int>(0x160D, 52),
pair<WORD, int>(0x1A0B, 52),
pair<WORD, int>(0x20EC, 52),
pair<WORD, int>(0x1BC2, 65),
pair<WORD, int>(0x1779, 52),
pair<WORD, int>(0x1911, 46),
pair<WORD, int>(0x1910, 46),
pair<WORD, int>(0x1947, 51),
pair<WORD, int>(0x1946, 51),
pair<WORD, int>(0x257D, 54),
pair<WORD, int>(0x1E2F, 47),
pair<WORD, int>(0x1B94, 51),
//Mccree:
pair<WORD, int>(0x15F7, 53),
pair<WORD, int>(0x15FA, 53),
pair<WORD, int>(0x18B7, 53),
pair<WORD, int>(0x15F9, 53),
pair<WORD, int>(0x18B9, 53),
pair<WORD, int>(0x15F3, 37),
pair<WORD, int>(0x15F0, 57),
pair<WORD, int>(0x15F8, 53),
pair<WORD, int>(0x18B8, 53),
pair<WORD, int>(0x1C4C, 53),
pair<WORD, int>(0x1976, 53),
pair<WORD, int>(0x21CD, 53),
pair<WORD, int>(0x1A12, 56),
pair<WORD, int>(0x15F4, 37),
pair<WORD, int>(0x15EF, 57),
pair<WORD, int>(0x1BCD, 94),
pair<WORD, int>(0x1BCA, 85),
pair<WORD, int>(0x1C12, 125),
pair<WORD, int>(0x19B6, 94),
pair<WORD, int>(0x1E0C, 97),
pair<WORD, int>(0x2A08, 54),
pair<WORD, int>(0x2CD5, 54),
//Mei:
pair<WORD, int>(0x161F, 50),
pair<WORD, int>(0x1620, 50),
pair<WORD, int>(0x18C9, 50),
pair<WORD, int>(0x1621, 50),
pair<WORD, int>(0x1982, 72),
pair<WORD, int>(0x1B67, 58),
pair<WORD, int>(0x1622, 50),
pair<WORD, int>(0x18CB, 50),
pair<WORD, int>(0x18CA, 50),
pair<WORD, int>(0x21AC, 50),
pair<WORD, int>(0x1BE1, 58),
pair<WORD, int>(0x17A5, 55),
pair<WORD, int>(0x17A4, 55),
pair<WORD, int>(0x1915, 106),
pair<WORD, int>(0x1914, 106),
pair<WORD, int>(0x1BEC, 112),
pair<WORD, int>(0x1A10, 50),
pair<WORD, int>(0x1981, 86),
pair<WORD, int>(0x46A2, 115),
//Mercy:
pair<WORD, int>(0x15DB, 150),
pair<WORD, int>(0x18AE, 150),
pair<WORD, int>(0x15DC, 150),
pair<WORD, int>(0x15DE, 150),
pair<WORD, int>(0x15D4, 149),
pair<WORD, int>(0x15D8, 157),
pair<WORD, int>(0x15DD, 168),
pair<WORD, int>(0x18B0, 168),
pair<WORD, int>(0x18AF, 168),
pair<WORD, int>(0x1979, 168),
pair<WORD, int>(0x1A2A, 168),
pair<WORD, int>(0x15D3, 149),
pair<WORD, int>(0x15D7, 157),
pair<WORD, int>(0x1BC6, 171),
pair<WORD, int>(0x196A, 157),
pair<WORD, int>(0x1F71, 171),
pair<WORD, int>(0x1C11, 170),
pair<WORD, int>(0x1B7B, 152),
//Moira:
pair<WORD, int>(0x1BE8, 64),
pair<WORD, int>(0x1C5E, 64),
pair<WORD, int>(0x1C5F, 64),
pair<WORD, int>(0x1C61, 64),
pair<WORD, int>(0x1C60, 64),
pair<WORD, int>(0x1C62, 64),
pair<WORD, int>(0x1C63, 64),
pair<WORD, int>(0x2136, 64),
pair<WORD, int>(0x1C6A, 69),
pair<WORD, int>(0x1C42, 69),
pair<WORD, int>(0x1C45, 114),
pair<WORD, int>(0x1C79, 114),
pair<WORD, int>(0x212A, 51),
pair<WORD, int>(0x1C75, 42),
pair<WORD, int>(0x2C72, 95),
//Orisa:
pair<WORD, int>(0x1933, 50),
pair<WORD, int>(0x1935, 50),
pair<WORD, int>(0x1934, 50),
pair<WORD, int>(0x1937, 50),
pair<WORD, int>(0x1936, 50),
pair<WORD, int>(0x1938, 50),
pair<WORD, int>(0x1B95, 49),
pair<WORD, int>(0x2A11, 58),
pair<WORD, int>(0x1C10, 58),
pair<WORD, int>(0x1B62, 59),
pair<WORD, int>(0x1B65, 59),
pair<WORD, int>(0x1B64, 55),
pair<WORD, int>(0x1B63, 55),
pair<WORD, int>(0x1B61, 54),
pair<WORD, int>(0x1C6F, 56),
//Pharah:
pair<WORD, int>(0x16BF, 38),
pair<WORD, int>(0x16C2, 38),
pair<WORD, int>(0x1768, 49),
pair<WORD, int>(0x16C1, 38),
pair<WORD, int>(0x18E1, 38),
pair<WORD, int>(0x16C0, 38),
pair<WORD, int>(0x1929, 42),
pair<WORD, int>(0x1928, 42),
pair<WORD, int>(0x19A0, 38),
pair<WORD, int>(0x1A16, 42),
pair<WORD, int>(0x19EA, 92),
pair<WORD, int>(0x1769, 49),
pair<WORD, int>(0x192A, 112),
pair<WORD, int>(0x192B, 112),
pair<WORD, int>(0x20E6, 5), // voir 6 ou 87
pair<WORD, int>(0x1C0B, 38),
pair<WORD, int>(0x19E6, 111),
pair<WORD, int>(0x16A7, 105),
pair<WORD, int>(0x2D7C, 108),
//Reaper:
pair<WORD, int>(0x1697, 36),
pair<WORD, int>(0x1699, 36),
pair<WORD, int>(0x18DB, 36),
pair<WORD, int>(0x18DD, 36),
pair<WORD, int>(0x167C, 47),
pair<WORD, int>(0x169A, 36),
pair<WORD, int>(0x1698, 36),
pair<WORD, int>(0x18DC, 36),
pair<WORD, int>(0x1C3C, 36),
pair<WORD, int>(0x1A07, 36),
pair<WORD, int>(0x1770, 49),
pair<WORD, int>(0x1771, 49),
pair<WORD, int>(0x167B, 47),
pair<WORD, int>(0x1BC0, 45),
pair<WORD, int>(0x195A, 58),
pair<WORD, int>(0x1BEB, 41),
pair<WORD, int>(0x21C1, 38),
pair<WORD, int>(0x1C74, 34),
pair<WORD, int>(0x168B, 50),
//Reinhardt:
pair<WORD, int>(0x165F, 41),
pair<WORD, int>(0x1660, 41),
pair<WORD, int>(0x1662, 41),
pair<WORD, int>(0x18D6, 41),
pair<WORD, int>(0x163F, 41),
pair<WORD, int>(0x1954, 53),
pair<WORD, int>(0x18D5, 41),
pair<WORD, int>(0x1661, 41),
pair<WORD, int>(0x17B1, 41),
pair<WORD, int>(0x19A1, 41),
pair<WORD, int>(0x2A0C, 41),
pair<WORD, int>(0x1B87, 41),
pair<WORD, int>(0x1B7E, 37),
pair<WORD, int>(0x1640, 41),
pair<WORD, int>(0x163B, 39),
pair<WORD, int>(0x163C, 39),
pair<WORD, int>(0x1955, 53),
pair<WORD, int>(0x1EB6, 37),
pair<WORD, int>(0x21BB, 38),
pair<WORD, int>(0x1991, 44),
//Roadhog:
pair<WORD, int>(0x16A3, 61),
pair<WORD, int>(0x18DE, 61),
pair<WORD, int>(0x16A4, 61),
pair<WORD, int>(0x1A14, 61),
pair<WORD, int>(0x1C1C, 80),
pair<WORD, int>(0x16A5, 61),
pair<WORD, int>(0x16A6, 61),
pair<WORD, int>(0x18E0, 61),
pair<WORD, int>(0x18DF, 61),
pair<WORD, int>(0x1C1B, 74),
pair<WORD, int>(0x16A0, 53),
pair<WORD, int>(0x169F, 53),
pair<WORD, int>(0x169C, 65),
pair<WORD, int>(0x169B, 65),
pair<WORD, int>(0x20E9, 112),
pair<WORD, int>(0x195C, 62),
pair<WORD, int>(0x197D, 53),
pair<WORD, int>(0x2D76, 78),
//Soldier:
pair<WORD, int>(0x16C7, 38),
pair<WORD, int>(0x16C9, 38),
pair<WORD, int>(0x16CA, 38),
pair<WORD, int>(0x18E4, 38),
pair<WORD, int>(0x16C8, 38),
pair<WORD, int>(0x18E6, 38),
pair<WORD, int>(0x18E5, 38),
pair<WORD, int>(0x19A6, 38),
pair<WORD, int>(0x20E8, 38),
pair<WORD, int>(0x178C, 55),
pair<WORD, int>(0x178D, 55),
pair<WORD, int>(0x1791, 54),
pair<WORD, int>(0x1790, 54),
pair<WORD, int>(0x1BC8, 41),
pair<WORD, int>(0x1EBE, 47),
pair<WORD, int>(0x1C1E, 45),
pair<WORD, int>(0x1A1E, 116),
pair<WORD, int>(0x16C3, 43),
pair<WORD, int>(0x2A5D, 46),
//Sombra:
pair<WORD, int>(0x162B, 42),
pair<WORD, int>(0x162D, 42),
pair<WORD, int>(0x162E, 42),
pair<WORD, int>(0x1A1D, 42),
pair<WORD, int>(0x162C, 42),
pair<WORD, int>(0x18CF, 42),
pair<WORD, int>(0x18D0, 42),
pair<WORD, int>(0x18D1, 42),
pair<WORD, int>(0x2138, 42),
pair<WORD, int>(0x198B, 57),
pair<WORD, int>(0x198A, 57),
pair<WORD, int>(0x198C, 36),
pair<WORD, int>(0x198D, 36),
pair<WORD, int>(0x1BC7, 37),
pair<WORD, int>(0x20DB, 52),
pair<WORD, int>(0x1C16, 46),
pair<WORD, int>(0x1C17, 36),
pair<WORD, int>(0x213B, 49),

// SIGMA
pair<WORD, int>(0x28CF, 110),
pair<WORD, int>(0x2D65, 110),
pair<WORD, int>(0x2D64, 110),
pair<WORD, int>(0x2D62, 110),
pair<WORD, int>(0x2D66, 110),
pair<WORD, int>(0x2D67, 110),

//Symmetra:
pair<WORD, int>(0x16E7, 89),
pair<WORD, int>(0x16E9, 89),
pair<WORD, int>(0x1998, 89),
pair<WORD, int>(0x16E4, 86),
pair<WORD, int>(0x16E3, 86),
pair<WORD, int>(0x16DB, 92),
pair<WORD, int>(0x16E8, 89),
pair<WORD, int>(0x16EA, 89),
pair<WORD, int>(0x18ED, 89),
pair<WORD, int>(0x1C44, 89),
pair<WORD, int>(0x18EE, 89),
pair<WORD, int>(0x18EF, 89),
pair<WORD, int>(0x1A27, 89),
pair<WORD, int>(0x16DC, 92),
pair<WORD, int>(0x16DF, 97),
pair<WORD, int>(0x2133, 90),
pair<WORD, int>(0x1E0A, 95),
pair<WORD, int>(0x1B92, 83),
pair<WORD, int>(0x46A3, 89),
//Torbjorn:
pair<WORD, int>(0x1637, 45),
pair<WORD, int>(0x1638, 45),
pair<WORD, int>(0x1639, 45),
pair<WORD, int>(0x1BAD, 64),
pair<WORD, int>(0x163A, 45),
pair<WORD, int>(0x18D2, 45),
pair<WORD, int>(0x18D4, 45),
pair<WORD, int>(0x18D3, 45),
pair<WORD, int>(0x197B, 45),
pair<WORD, int>(0x1C18, 92),
pair<WORD, int>(0x162F, 51),
pair<WORD, int>(0x1630, 51),
pair<WORD, int>(0x1633, 59),
pair<WORD, int>(0x1634, 59),
pair<WORD, int>(0x1BCB, 96),
pair<WORD, int>(0x1968, 92),
pair<WORD, int>(0x21AB, 55),
pair<WORD, int>(0x1B60, 121),
pair<WORD, int>(0x1C6E, 118),
pair<WORD, int>(0x46A6, 46),
//Tracer:
pair<WORD, int>(0x170F, 52),
pair<WORD, int>(0x1711, 52),
pair<WORD, int>(0x1712, 52),
pair<WORD, int>(0x1710, 52),
pair<WORD, int>(0x18F1, 52),
pair<WORD, int>(0x1942, 45),
pair<WORD, int>(0x195E, 46),
pair<WORD, int>(0x18F0, 52),
pair<WORD, int>(0x18F2, 52),
pair<WORD, int>(0x1B07, 52),
pair<WORD, int>(0x20EB, 52),
pair<WORD, int>(0x16EB, 49),
pair<WORD, int>(0x16EC, 49),
pair<WORD, int>(0x170B, 50),
pair<WORD, int>(0x170C, 50),
pair<WORD, int>(0x1943, 45),
pair<WORD, int>(0x21F2, 58),
pair<WORD, int>(0x1B66, 41),
pair<WORD, int>(0x1B2B, 37),
pair<WORD, int>(0x16F7, 123),
//Widowmaker:
pair<WORD, int>(0x171B, 40),
pair<WORD, int>(0x171C, 40),
pair<WORD, int>(0x171E, 40),
pair<WORD, int>(0x18F3, 40),
pair<WORD, int>(0x171D, 40),
pair<WORD, int>(0x1C21, 38),
pair<WORD, int>(0x18F4, 40),
pair<WORD, int>(0x18F5, 40),
pair<WORD, int>(0x1978, 40),
pair<WORD, int>(0x21BA, 40),
pair<WORD, int>(0x1714, 39),
pair<WORD, int>(0x1713, 39),
pair<WORD, int>(0x1919, 47),
pair<WORD, int>(0x1918, 47),
pair<WORD, int>(0x1C20, 38),
pair<WORD, int>(0x1BC9, 38),
pair<WORD, int>(0x2113, 46),
pair<WORD, int>(0x1C1F, 46),
pair<WORD, int>(0x1BAF, 38),
pair<WORD, int>(0x1717, 42), // 누아르
//Winston:
pair<WORD, int>(0x16D3, 109),
pair<WORD, int>(0x16D6, 109),
pair<WORD, int>(0x18E7, 109),
pair<WORD, int>(0x18E9, 109),
pair<WORD, int>(0x197F, 87),
pair<WORD, int>(0x16D5, 109),
pair<WORD, int>(0x16D4, 109),
pair<WORD, int>(0x18E8, 109),
pair<WORD, int>(0x16CF, 61),
pair<WORD, int>(0x16D0, 61),
pair<WORD, int>(0x16CB, 105),
pair<WORD, int>(0x16CC, 105),
pair<WORD, int>(0x1EAC, 61),
pair<WORD, int>(0x1964, 61),
pair<WORD, int>(0x1CC6, 56),
//WreckingBall:
pair<WORD, int>(0x1C56, 199),
pair<WORD, int>(0x213F, 199),
pair<WORD, int>(0x213D, 199),
pair<WORD, int>(0x213C, 199),
pair<WORD, int>(0x213E, 199),
pair<WORD, int>(0x2143, 199),
pair<WORD, int>(0x2142, 199),
pair<WORD, int>(0x214A, 145),
pair<WORD, int>(0x2128, 138),
pair<WORD, int>(0x2147, 153),
pair<WORD, int>(0x2130, 153),
pair<WORD, int>(0x214E, 153),
pair<WORD, int>(0x2CD8, 199),
//Zarya:
pair<WORD, int>(0x166B, 65),
pair<WORD, int>(0x166E, 65),
pair<WORD, int>(0x18D8, 65),
pair<WORD, int>(0x166D, 65),
pair<WORD, int>(0x166C, 65),
pair<WORD, int>(0x18D9, 65),
pair<WORD, int>(0x1A1B, 65),
pair<WORD, int>(0x1664, 36),
pair<WORD, int>(0x18DA, 65),
pair<WORD, int>(0x1C1A, 51),
pair<WORD, int>(0x1668, 43),
pair<WORD, int>(0x1667, 43),
pair<WORD, int>(0x1663, 36),
pair<WORD, int>(0x1944, 52),
pair<WORD, int>(0x1945, 52),
pair<WORD, int>(0x196E, 57),
pair<WORD, int>(0x2134, 76),
pair<WORD, int>(0x1C19, 58),
pair<WORD, int>(0x1B6A, 49),
pair<WORD, int>(0x2CDF, 65),
pair<WORD, int>(0x45BE, 109),
//Zenyatta:
pair<WORD, int>(0x1603, 149),
pair<WORD, int>(0x18BA, 149),
pair<WORD, int>(0x1604, 149),
pair<WORD, int>(0x18BB, 149),
pair<WORD, int>(0x15FB, 52),
pair<WORD, int>(0x1966, 99),
pair<WORD, int>(0x1605, 149),
pair<WORD, int>(0x1606, 149),
pair<WORD, int>(0x1C40, 149),
pair<WORD, int>(0x18BC, 149),
pair<WORD, int>(0x19A3, 149),
pair<WORD, int>(0x15FC, 52),
pair<WORD, int>(0x15FF, 98),
pair<WORD, int>(0x1600, 98),
pair<WORD, int>(0x1E2D, 89),
pair<WORD, int>(0x1BE7, 128),
pair<WORD, int>(0x1960, 110),
pair<WORD, int>(0x21D1, 93),
//Training Bots:
pair<WORD, int>(0x1AFC, 37),
pair<WORD, int>(0x1AF8, 37),
pair<WORD, int>(0x1AFA, 37),
};

int GetNameTagIndex() //다중인식
{
	vector<DWORD64>tempNAMETagPTR = EntityPTR;
	int MI = 0, Result = -1;
	vector<float> Dist;

	for (int i = 0; i < tempNAMETagPTR.size(); i++)
	{
		Vector3 Screen = GetAngle(MyAngle, viewMatrix.GetCameraVec(), Entitys[i].Location);
		if (Entitys[i].Alive && Entitys[i].Enemy)
		{
			Dist.push_back(MyAngle.Distance(Screen));
		}
		else
		{
			Dist.push_back(0xFFFFFFFF);
		}
	}

	for (int i = 0; i < Dist.size(); i++)
	{
		if (Dist[MI] > Dist[i])
		{
			MI = i;
		}
	}

	if (Dist[MI] != 0xFFFFFFFF)
	{
		Result = MI;
	}

	return Result;
}

void Taimbot() //에임봇 쓰레드
{
	while (TRUE)
	{
		//uint64_t pAngle = Config::Get().RPM<uint64_t>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
		bool shooted = false;
		BOOL IsMouse = FALSE;

		if (Config::Get().TPAimbot)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.01f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, Config::Get().AimSpeed, Config::Get().AimSpeed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
					}
				}
				this_thread::sleep_for(1ms);
			}
		}

		if (Config::Get().HanzoAimbot)
		{
			Config::Get().GravityBool = true;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				Config::Get().PreditLevel = 110.f;
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.01f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
						if (currentAngle.Distance(angle) * (180.f / M_PI) <= 15.f / viewMatrix.GetCameraVec().Distance(world))
						{
							Sendinput::SendVKcodesUp(0x4C);
							shooted = true;
						}
					}
				}
				this_thread::sleep_for(2ms);
			}
		}

		if (Config::Get().AnaSkill)
		{
			Config::Get().GravityBool = false;
			Config::Get().PreditLevel = 60.f;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.02f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 0x8);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
					}
				}
				this_thread::sleep_for(2ms);
			}
		}

		if (Config::Get().ESkill)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(0x45))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.02f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
						if (currentAngle.Distance(angle) * (180.f / M_PI) <= 10.f / viewMatrix.GetCameraVec().Distance(world))
						{
							shooted = true;
						}
					}
				}
				this_thread::sleep_for(2ms);
			}
		}

		if (Config::Get().Roadhog)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.02f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 0x8);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
						if (currentAngle.Distance(angle) * (180.f / M_PI) <= 10.f / viewMatrix.GetCameraVec().Distance(world))
						{
							shooted = true;
						}
					}
				}
				this_thread::sleep_for(2ms);
			}
		}
		
		if (Config::Get().DomPredit)
		{
			Config::Get().GravityBool = false;
			Config::Get().PreditLevel = 45.f;
			while (GetAsyncKeyState(VK_RBUTTON))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3Predit();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.02f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
					}
				}
				this_thread::sleep_for(2ms);
			}
		}
		
		if (Config::Get().자동평타)
		{
			Config::Get().GravityBool = false;
			Vector3 world1 = GetVector3123123Health1();
			while (viewMatrix.GetCameraVec().Distance(world1) <= 2.5f)
			{
				if (!shooted)
				{
					Vector3 world = GetVector3123123Health1();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.02f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);
						Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 0x800);
						if (currentAngle.Distance(angle) * (180.f / M_PI) <= 20.f / viewMatrix.GetCameraVec().Distance(world))
						{
							shooted = true;
						}
					}
				}
				this_thread::sleep_for(2ms);
			}
		}

		if (Config::Get().SAimbot)
		{
			Config::Get().GravityBool = false;
			//Config::PreditLevel = 50.f;
			while (Config::Get().RPM<BYTE>(AnglePTR + OFFSET_GenjiQ) == 0xB6)
			{
				Vector3 world = GetVector3123123();
				Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
				Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));

				if (viewMatrix.GetCameraVec().Distance(world) < 15.f && GetAsyncKeyState(VK_LBUTTON))
				{
					Vector3 smoothed = SmoothAngle(currentAngle, angle, Config::Get().AimSpeed, Config::Get().AimSpeed);
					Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);

					if (currentAngle.Distance(angle) * (180.f / M_PI) <= 30.f / viewMatrix.GetCameraVec().Distance(world))
					{
						if (viewMatrix.GetCameraVec().Distance(world) > 5.f)
							Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 0x8);
						else
							Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 1);
					}

				}
				this_thread::sleep_for(2ms);
			}
		}

		if (Config::Get().GENJISHIFT)
		{
			Config::Get().GravityBool = false;
			Config::Get().PreditLevel = 60.f;
			Vector3 world = GetVector3123123Health();
			Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
			Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));

			if (viewMatrix.GetCameraVec().Distance(world) <= 15.f && GetAsyncKeyState(VK_RBUTTON))
			{
				Vector3 smoothed = SmoothAngle(currentAngle, angle, Config::Get().AimSpeed, Config::Get().AimSpeed);
				Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);

				if (currentAngle.Distance(angle) * (180.f / M_PI) <= 50.f / viewMatrix.GetCameraVec().Distance(world))
				{
					Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 0x8);
				}

			}
			this_thread::sleep_for(2ms);
		}

		if (Config::Get().TAimbot)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_LBUTTON))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.01f;
						Vector3 smooted = SmoothAngle(currentAngle, angle, Config::Get().AimSpeed, Config::Get().AimSpeed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smooted);
						shooted = false;
					}
				}
				this_thread::sleep_for(1ms);
			}
		}

		if (Config::Get().FAimbot)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						float speed = (Config::Get().AimSpeed / currentAngle.Distance(angle)) * 0.01f;
						Vector3 smoothed = SmoothAngle(currentAngle, angle, speed, speed);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, smoothed);

						if (currentAngle.Distance(angle) * (180.f / M_PI) <= 15.f / viewMatrix.GetCameraVec().Distance(world))
						{
							Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 1);
							shooted = true;
						}
					}
				}
				this_thread::sleep_for(2ms);
			}
		}
		
		if (Config::Get().Silent)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_XBUTTON2))
			{
				if (!shooted)
				{
					Vector3 world = GetVector3();
					Vector3 currentAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
					Vector3 angle = CalcAngle(viewMatrix.GetCameraVec(), world, viewMatrix.GetCameraVec().Distance(world));
					if (currentAngle.Distance(angle) * (180.f / M_PI) <= Config::Get().Fov / viewMatrix.GetCameraVec().Distance(world))
					{
						Vector3 SaveAngle = Config::Get().RPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, angle);
						Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 1);
						Sleep(10);
						Config::Get().WPM<Vector3>(AnglePTR + OFFSET_PLAYER_CONTROLLER_ROTATION, SaveAngle);
						shooted = true;
					}
				}
				this_thread::sleep_for(0ms);
			}
		}

		if (Config::Get().TriggerBot)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_MBUTTON))
			{
				Vector3 world = GetVector3();
				XMFLOAT3 min{ -0.1f, -0.05f, -0.1f }, max{ 0.1f, 0.17f, 0.1f };
				XMFLOAT3 min2{ -0.16f, -0.3f, -0.16f }, max2{ 0.16f, 0.23f, 0.16f };
				XMFLOAT3 XMEnPos = XMFLOAT3(world.x, world.y, world.z);

				if (viewMatrix.IntersectRayWithAABB(viewMatrixTo, viewMatrixTo.GetCameraVec(), MyXMAngle, GetAsyncKeyState(VK_XBUTTON2) ? min : min2, GetAsyncKeyState(VK_XBUTTON2) ? max : max2, Config::Get().m_TriggerScale / 100.f, XMEnPos, GetAsyncKeyState(VK_XBUTTON2)))
				{
					Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 1);
				}

				this_thread::sleep_for(0ms);
			}
		}

		if (Config::Get().Mecry)
		{
			Config::Get().GravityBool = false;
			while (GetAsyncKeyState(VK_MBUTTON))
			{
				Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 1);
				this_thread::sleep_for(2ms);
				Config::Get().WPM<BYTE>(AnglePTR + OFFSET_PLAYER_CONTROLLER_KEY, 2);
				this_thread::sleep_for(2ms);
			}
		}
		this_thread::sleep_for(25ms);
	}
}

LONG WINAPI Exc2pt10nHand1111er(EXCEPTION_POINTERS* e) //veh 훅 (메인)
{
	std::string EncryptEA = encrypt((UINT64)(e->ExceptionRecord->ExceptionAddress));
	auto ctx = e->ContextRecord;

	if (EncryptEA == EnFovHook)
	{
		DWORD Filter = *(DWORD*)(*(DWORD64*)(ctx->Rsp + 0x28) - 0xD); //mov -> 0x28, xor -> 0x38 //패치
		if (Filter == 0x8E8B49A5) //필터
		{
			if (*(float*)(&ctx->Xmm7) == 0.01f || *(float*)(&ctx->Xmm8) == 0.01f)
			{
				Vector3 MyPos = *(Vector3*)(ctx->Rsp + 0x3B0);
				EnPos = *(Vector3*)(ctx->Rsp + 0x330); //적좌표 //패치

				for (int i = 0; i < EntityPTR.size(); i++)
				{
					if (abs(EnPos.x - Entitys[i].Location.x) <= 1.5f && abs(EnPos.z - Entitys[i].Location.z) <= 1.5f) // 좌표를 비교하여 적에대한 정보를 얻기
					{
						//printf("\nEnPos: %f %f %f\n", EnPos.x, EnPos.y, EnPos.z);
						//printf("Entitys[%d]: %f %f %f\n\n", i, Entitys[i].Location.x, Entitys[i].Location.y, Entitys[i].Location.z);

						Entitys[i].eParent = *(uint64_t*)(ctx->R14 + 0x48); // 적의 정보
						Entitys[i].eHealthComponent = GetComponent(Entitys[i].eParent, eComponentType::TYPE_HEALTH); //적 체력
						if (Entitys[i].eHealthComponent)
						{
							Entitys[i].Health = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_HEALTH);// 적체력
							Entitys[i].HealthMax = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_HEALTHMAX);// 적 최대체력
							Entitys[i].ARMOR = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_ARMOR);
							Entitys[i].ARMORMAX = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_ARMORMAX);
							Entitys[i].BARRIER = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_BARRIER);
							Entitys[i].BARRIERMAX = Config::Get().RPM<float>(Entitys[i].eHealthComponent + OFFSET_HEALTHPTR_BARRIERMAX);
						}

						//if (abs(EnPos.x - Entitys[i].Location.x) <= 1.0f && abs(EnPos.z - Entitys[i].Location.z) <= 1.0f)

						uint64_t eParentPTR = *(uint64_t*)(ctx->R14 + 0x48); // 적 정보
						uint64_t mParentPTR = *(uint64_t*)(ctx->R14 + 0x40); // 내 정보

						uint64_t eVelocityComponent = GetComponent(eParentPTR, eComponentType::TYPE_VELOCITY); // 벨로시티 컴포넌트 불러오기

						uint64_t eHealthComponent = GetComponent(eParentPTR, eComponentType::TYPE_HEALTH);// 체력 컴포넌트 불러오기
						uint64_t eVisComponent = GetComponent(eParentPTR, eComponentType::TYPE_P_VISIBILITY);// 벽구분 컴포넌트 불러오기
						uint64_t eRotationComponent = GetComponent(eParentPTR, eComponentType::TYPE_ROTATION);// 로테이션 컴포넌트 불러오기

						Entitys[i].Velocity = Config::Get().RPM<Vector3>(eVelocityComponent + OFFSET_VELOCITYPTR_VELOCITY);

						Entitys[i].SkinID = Config::Get().RPM<WORD>(eParentPTR + 0x48); //스킨 ID
						float p1 = Config::Get().RPM<float>(eHealthComponent + OFFSET_HEALTHPTR_HEALTH);
						float p2 = Config::Get().RPM<float>(eHealthComponent + OFFSET_HEALTHPTR_ARMOR);
						float p3 = Config::Get().RPM<float>(eHealthComponent + OFFSET_HEALTHPTR_BARRIER);
						Entitys[i].PlayerHealth = p1 + p2 + p3;

						uint64_t pBoneData = Config::Get().RPM<uint64_t>(eVelocityComponent + OFFSET_VELOCITYPTR_BONEDATA);
						if (pBoneData)
						{
							int boneIndex = HEROID2YPITCH(Skin2Hero(Entitys[i].SkinID));
							uint64_t bonesBase = Config::Get().RPM<uint64_t>(pBoneData + OFFSET_BONEDATA_BONEBASE);
							if (bonesBase)
							{
								DirectX::XMFLOAT3 currentBone = Config::Get().RPM<DirectX::XMFLOAT3>(bonesBase + OFFSET_BONE_SIZE * boneIndex + OFFSET_BONE_LOCATION);
								DirectX::XMFLOAT3 Result{};
								XMMATRIX rotMatrix = XMMatrixRotationY(Config::Get().RPM<float>(Config::Get().RPM<uint64_t>(eRotationComponent + 0x748) + 0xA7C)); //로테이션 //패치
								DirectX::XMStoreFloat3(&Result, XMVector3Transform(XMLoadFloat3(&currentBone), rotMatrix));
								Entitys[i].BonePos = Vector3(Result.x, Result.y, Result.z) + EnPos - Vector3(0, 0, 0);
							}
						}
					}
				}
			}
		}
		ctx->Rax ^= ctx->R9;
		ctx->Rip += 0x3;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	else if (EncryptEA == EnAngleHook) // 앵글훅
	{
		MyAngle = *(Vector3*)(ctx->Rsp + 0x20);
		AnglePTR = ctx->Rdi;
		*(Vector3*)(&ctx->Xmm0) = *(Vector3*)(ctx->Rsp + 0x20);
		ctx->Rip += 0x5;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	else if (EncryptEA == EnWallHook) //월핵
	{
		ctx->Rcx = ctx->Rsi;
		if (Config::Get().UseGlowESP)
		{
			ctx->Rdx = 0xFF;
			/*for (int i = 0; i < EntityPTR.size(); i++)
			{
				if (abs(RootPos.x - Entitys[i].Location.x) <= 0.5f && abs(RootPos.z - Entitys[i].Location.z) <= 0.5f)
				{
					ctx->R8 = RGBA2ARGB(Config::Get().E2SPColor.x * 255, Config::Get().E2SPColor.y * 255, Config::Get().E2SPColor.z * 255, Config::Get().E2SPColor.w * 255);
				}
			}*/
		}
		ctx->Rip += 0x3;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	else if (EncryptEA == EnVisHook) //벽구분
	{
		*(uint64_t*)(ctx->Rbp + 0x28) = ctx->Rcx;
		GetMyPos = *(Vector3*)(ctx->Rbp + 0x2E0);
		ctx->Rip += 0x4;
		return EXCEPTION_CONTINUE_EXECUTION;
	}
	

	return EXCEPTION_CONTINUE_SEARCH;
}

void DrawQuad(const ImVec2& pos1, const ImVec2& pos2, const ImVec2& pos3, const ImVec2& pos4, ImU32 color, float thickness, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddQuad(pos1, pos2, pos3, pos4, color, thickness);
}

void DrawQuadFilled(const ImVec2& pos1, const ImVec2& pos2, const ImVec2& pos3, const ImVec2& pos4, ImU32 color, float thickness, float rounding)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	window->DrawList->AddQuadFilled(pos1, pos2, pos3, pos4, color);
}

void DrawHealthBar(int width, const ImVec2& from, int currentHealth, int maxHealth, int currentArmor, int maxArmor, int currentBarrier, int maxBarrier)
{
	int height = width * 0.1;
	int indent = width * 0.02;
	int blockSize = width * 0.06;
	int space = width * 0.01;
	int nbBlock = width / blockSize;
	int nbBlockArmor = 0;
	if (maxArmor != 0 && currentArmor != 0) { //maxArmor != 0 &&
		int nbBlockArmorMax = nbBlock * 0.25;
		nbBlockArmor = (currentArmor * nbBlockArmorMax) / maxArmor;
	}
	int nbBlockBarrier = 0;
	if (maxBarrier != 0 && currentBarrier != 0) { //maxBarrier != 0 &&
		int nbBlockBarrierMax = nbBlock * 0.25;
		nbBlockBarrier = (currentBarrier * nbBlockBarrierMax) / maxBarrier;
	}
	int nbBlockHealthFull = (currentHealth * (nbBlock - nbBlockArmor - nbBlockBarrier)) / maxHealth;

	for (int i = 0; i < nbBlock; i++) {
		int pos1X = from.x + (i * (blockSize + space));
		int pos1Y = from.y;
		ImVec2 pos1(pos1X, pos1Y);

		int pos2X = from.x + blockSize + (i * (blockSize + space));
		int pos2Y = from.y;
		ImVec2 pos2(pos2X, pos2Y);

		int pos3X = from.x + indent + blockSize + (i * (blockSize + space));
		int pos3Y = from.y - height;
		ImVec2 pos3(pos3X, pos3Y);

		int pos4X = from.x + indent + (i * (blockSize + space));
		int pos4Y = from.y - height;
		ImVec2 pos4(pos4X, pos4Y);

		if (i < nbBlockHealthFull) {
			DrawQuadFilled(pos4, pos3, pos2, pos1, ImColor(255, 0, 0, 255), 0, 0);
		}
		else {
			if (nbBlockArmor > 0 && i < (nbBlockHealthFull + nbBlockArmor)) {
				DrawQuadFilled(pos4, pos3, pos2, pos1, ImColor(255, 220, 49, 255), 0, 0);
			}
			else {
				if (nbBlockBarrier > 0 && i < (nbBlockHealthFull + nbBlockArmor + nbBlockBarrier)) {
					DrawQuadFilled(pos4, pos3, pos2, pos1, ImColor(114, 189, 234, 255), 0, 0);
				}
				else {
					DrawQuadFilled(pos4, pos3, pos2, pos1, ImColor(80, 80, 80, 125), 0, 0);
				}
			}
		}
	}
}

void Menu::esp() // esp 기능 
{
	ImGuiWindow* window = ImGui::GetCurrentWindow(); // 임구이 오버레이

	for (int i = 0; i < EntityPTR.size(); i++)
	{
		viewMatrix = Config::Get().RPM<Matrix>(viewMatrixPtr);
		Vector2 output{}, output2{};
		if (Entitys[i].Alive && Entitys[i].Enemy)
		{
			Vector3 Vec3 = Entitys[i].Location;

			if (viewMatrix.WorldToScreen(Vector3(Vec3.x, Vec3.y - 2.f, Vec3.z), 1920, 1080, output) && viewMatrix.WorldToScreen(Vector3(Vec3.x, Vec3.y - 0.f, Vec3.z), 1920, 1080, output2))
			{
				float Size = abs(output.y - output2.y) / 2.0f;
				float Size2 = abs(output.y - output2.y) / 20.0f;
				float xpos = (output.x + output2.x) / 2;
				float ypos = output.y + Size / 5;
				int barSize = (ypos - ypos) * 1.5;
				int TextOffset = 0;

				string dist = to_string((int)viewMatrix.GetCameraVec().Distance(Vec3)) + "M";

				if (Config::Get().HealthESP)
				{
					if (Entitys[i].eHealthComponent)
					{
						if (barSize <= 60) {
							barSize = 60;
							TextOffset += 10;
						}
						if (barSize > 60 && barSize <= 160) {
							barSize = 120;
							TextOffset += 20;
						}

						if (barSize > 160) {
							barSize = 200;
							TextOffset += 30;
						}

						ImVec2 TextSize = ImGui::CalcTextSize(dist.c_str());
						window->DrawList->AddText(ImVec2(xpos - TextSize.x / 2.0f, output.y - TextSize.y / 2.0f), ImGui::GetColorU32(Config::Get().ESPColor3), dist.c_str()); // 거리 색변경 
						DrawHealthBar(barSize, ImVec2(xpos - (barSize / 2), ypos + (barSize * 0.10)), Entitys[i].Health, Entitys[i].HealthMax, Entitys[i].ARMOR, Entitys[i].ARMORMAX, Entitys[i].BARRIER, Entitys[i].BARRIERMAX);// 체력바
					}
				}
			}
		}
	}
}

#pragma endregion

#pragma region initsk
void SettingBreakPoints()
{
	HANDLE hMainThread = HW1BP->G2tMa1nThre2d();
	srand(GetTickCount64());
	PVOID pHandler = AddVectoredExceptionHandler(1, Exc2pt10nHand1111er);
	CONTEXT c{};
	c.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	SuspendThread(hMainThread);
	c.Dr0 = Config::Get().BaseAddress + offset::AngleHook;
	c.Dr1 = Config::Get().BaseAddress + offset::BorderLine;
	c.Dr2 = Config::Get().BaseAddress + offset::FovHook;
	//c.Dr3 = Config::Get().BaseAddress + offset::CompoenetHook;
	//c.Dr3 = Config::Get().BaseAddress + offset::Bone;
	c.Dr7 = (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6);
	SetThreadContext(hMainThread, &c);
	ResumeThread(hMainThread);

	_beginthread((_beginthread_proc_type)Pointer, 0, nullptr);
	_beginthread((_beginthread_proc_type)StructT, 0, nullptr);
	_beginthread((_beginthread_proc_type)Taimbot, 0, nullptr);
	//_beginthread((_beginthread_proc_type)GetEntities, 0, nullptr);
}

void BaseSettings()
{
	Config::Get().hProcess = GetCurrentProcess();
	Config::Get().BaseAddress = (DWORD64)GetModuleHandleA(("Overwatch.exe"));
	auto wndStr = "TankWindowClass";
	Config::Get().hWindow = FindWindowA(wndStr, NULL);
}
#pragma endregion

#pragma region Dllmain
DWORD WINAPI IMGUILOGIN(LPVOID lpParam) // 메인 쓰레드
{
	VMProtectBeginUltra("IMGUI");

	if (AllocConsole()) {

		freopen("CONIN$", "rb", stdin);

		freopen("CONOUT$", "wb", stdout);

		freopen("CONOUT$", "wb", stderr);
	}

	InputSys::Get().Initialize();
	D3dHook::AttachHook();
	ImGuiStyle* style = &ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.Fonts->AddFontFromFileTTF("C:\\Cannabis.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesKorean()); //폰트 설정

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildWindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.60f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.57f, 0.57f, 0.57f, 0.70f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.80f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_Button] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_Header] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
	colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.54f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);


	while (!Config::Get().IsLogin)
	{

	}

	BaseSettings();
	SaveEncrypted();
	SettingBreakPoints();

	while (true)
	{

	}

	FreeLibraryAndExitThread((HMODULE)g_Module, 0);
	return 0;
	VMProtectEnd();
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		g_Module = hInst;
		DisableThreadLibraryCalls(g_Module);
		RemovePeHeader(g_Module);
		//Aeternum::get_retaddr();
		////////////////// Thread //////////////////
		CloseHandle(CreateThread(nullptr, 0, IMGUILOGIN, (LPVOID)hInst, 0, nullptr));
		////////////////// Thread //////////////////
	}
	return TRUE;
}
#pragma endregion 