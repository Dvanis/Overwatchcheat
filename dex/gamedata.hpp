#pragma once

#include "singleton.h"
#include <Windows.h>
#include <vector>
#include <mutex>
#include <map>
#include "config.h"
#include  <time.h>
#include "Vector3.h"
#include <array>
#include "load.h"
using namespace std;
#define InRange(val) (val >= Config::Get().BaseAddress && val <= Config::Get().BaseAddress + 0x9000000)


enum eComponentType
{
	TYPE_ERROR = -1,
	TYPE_VELOCITY = 4,
	TYPE_TEAM = 26,
	TYPE_ROTATION = 39,
	TYPE_ID = 43,
	TYPE_LINK = 44,
	TYPE_P_VISIBILITY = 45,
	TYPE_SKILL = 47,
	TYPE_HEALTH = 51,
	TYPE_P_HEROID = 75,
	TYPE_OUTLINE = 83,
};

struct Component
{
	uint64_t ComponentBase;
	uint64_t ComponentParent;
	uint8_t ComponentType;

	Component(uint64_t ComponentBase, uint64_t ComponentParent, uint8_t ComponentType) : ComponentBase(ComponentBase), ComponentParent(ComponentParent), ComponentType(ComponentType) {};
	Component() : ComponentBase(NULL), ComponentParent(NULL), ComponentType(eComponentType::TYPE_ERROR) {};

	BOOL IsValid()
	{
		if (InRange(Config::Get().RPM<uint64_t>(ComponentBase)))
		{
			if (Config::Get().RPM<uint8_t>(ComponentBase + 0x10) == ComponentType)
			{
				return TRUE;
			}
		}

		uint64_t CompBase = FnDecryptParnet(ComponentParent, ComponentType);
		if (InRange(Config::Get().RPM<uint64_t>(CompBase)))
		{
			if (Config::Get().RPM<uint8_t>(CompBase + 0x10) == ComponentType)
			{
				ComponentBase = CompBase;
				return TRUE;
			}
		}
		return FALSE;
	}
};


class OWENTITY
{
public:
	struct basePointers
	{
		DWORD_PTR entityBoneBaseLocationPtr;
		DWORD_PTR entityHealthPtr;
		DWORD_PTR entityRotationPtr;
		DWORD_PTR entityVelocityPtr;
		DWORD_PTR entityVisPtr;
		DWORD_PTR entityHeroIDPtr;
	} basePointers;

	struct valuesPointers
	{
		float* health;
		float* maxhealth;
		float* armor;
		float* maxarmor;
		float* barrier;
		float* maxbarrier;
		BYTE* isMyTeam;
		BYTE* isVisible;
		DWORD64* heroID;
		DWORD64* skinID;
		Vector3* location;
		Vector3* velocity;
		Vector3* boneLocation;
		//Vector3* boneRotation;
		float* rotation;
		//WORD* bonesCount;
	} valuesPointers;
	DWORD64 entityTag;
	OWENTITY() {}
};
struct HeroID_Bone
{
	const char* heroName = "";
	DWORD64 heroID = 0x0;
	float heroHeadPos;
	int defaultHeadBoneIndex;
	HeroID_Bone() {}
	HeroID_Bone(const char* name, DWORD64 id, int dhbi) : heroName(name), heroID(id), defaultHeadBoneIndex(dhbi) {}
};
	class BOUCENTITY
	{
	public:
		BOOLEAN isValid;
		DWORD64 entityTag;
		DWORD_PTR pointer;
		float health;
		float maxhealth;
		float armor;
		float maxarmor;
		float barrier;
		float maxbarrier;
		BYTE isMyTeam;
		BYTE isVisible;
		uint32_t PlayerID;
		DWORD64 heroID;
		DWORD64 skinID;
		Vector3 location;
		Vector3 velocity;
		Vector3 boneLocation;
		//Vector3 boneRotation;
		Vector3 boneBaseLocation;
		float rotation;
		float stamina;
		float maxstamina;
		Vector3 entityHeadLocationByBones;
		Vector3 entityHeadLocationByHeroes;
		WORD bonesCount;

		DWORD64 debug1, debug2, debug3, debug4, debug5, debug6;
		BOUCENTITY() {}
	};

class GameData
		: public Singleton<GameData>
	{
		friend class Singleton<GameData>;

	public:
		bool askReset = false;

		HWND owWindow;
		HMODULE owModule;
		HMODULE overwatchBase;
		DWORD mainThreadID;
		bool running = true;

		Vector2 screenSize = Vector2(1920.0f, 1080.0f);
		uint32_t UniqueID;
		DWORD_PTR viewMatrixPtr;
		Matrix viewMatrix;
		MatrixTo viewMatrixXM;
		Vector3 localPlayerPosition;
		Vector3 localPlayerRotation;
		DWORD64 localPlayerHeroID;
		std::vector<Component> healthComponentList;
		std::vector<Component> velocityComponentList;
		std::vector<Component> heroIDComponentList;
		std::vector<Component> PlayerIDComponentList;
		std::vector<Component> visibilityComponentList;
		std::vector<Component> skinIDComponentList;
		std::vector<Component> boneBaseComponentList;
		std::vector<Component> rotationComponentList;
		std::vector<Component> LinkComponentList;
		std::vector<Component> SkillComponentList;
		std::vector<Component> BorderComponentList;
		std::vector<OWENTITY> OwEntityList;
		BYTE* widowTriggerAddress = NULL;
		clock_t lastPtrUpdate = NULL;
		std::array<BOUCENTITY, 20> BoucEntityList;
		std::mutex entityListMutex;


		mutex triggerMutex;
		BOOLEAN triggerActivate = FALSE;
		BOOLEAN triggerAuto = FALSE;
		clock_t triggerAutoRefreshShoot = NULL;
		DWORD triggerHanzoState = NULL;
		clock_t triggerLastShoot = NULL;
		clock_t triggerStartShoot = NULL;
		SHORT triggerKey = NULL;

		Vector3 lastMouse;

		char cfgPath[MAX_PATH];
		char clientName[MAX_PATH];


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

		std::array<pair<DWORD_PTR, int>, 544> Head_SkinIDs
		{
			//Ana:
		pair<DWORD64, int>(0x0A50000000001921,67),
		pair<DWORD64, int>(0x0A50000000001924,67),
		pair<DWORD64, int>(0x0A50000000001925,67),
		pair<DWORD64, int>(0x0A50000000001922,67),
		pair<DWORD64, int>(0x0A50000000001926,67),
		pair<DWORD64, int>(0x0A5000000000194B,68),
		pair<DWORD64, int>(0x0A50000000001923,67),
		pair<DWORD64, int>(0x0A50000000001927,67),
		pair<DWORD64, int>(0x0A5000000000199C,67),
		pair<DWORD64, int>(0x0A50000000001B11,67),
		pair<DWORD64, int>(0x0A5000000000194A,68),
		pair<DWORD64, int>(0x0A50000000001950,68),
		pair<DWORD64, int>(0x0A50000000001951,68),
		pair<DWORD64, int>(0x0A50000000001BB8,64),
		pair<DWORD64, int>(0x0A50000000001B0F,75),
		pair<DWORD64, int>(0x0A50000000001C0F,68),
		pair<DWORD64, int>(0x0A50000000004570, 117),
		//Ashe:
		pair<DWORD64, int>(0x0A50000000001ED3,125),
		pair<DWORD64, int>(0x0A500000000021CF,111),
		pair<DWORD64, int>(0x0A50000000002A02,125),
		pair<DWORD64, int>(0x0A50000000002A01,125),
		pair<DWORD64, int>(0x0A50000000002A04,125),
		pair<DWORD64, int>(0x0A50000000002A03,125),
		pair<DWORD64, int>(0x0A50000000002A06,125),
		pair<DWORD64, int>(0x0A50000000002A05,125),
		pair<DWORD64, int>(0x0A50000000002A10,125),
		pair<DWORD64, int>(0x0A50000000002A07,111),
		pair<DWORD64, int>(0x0A50000000002A09,108),
		pair<DWORD64, int>(0x0A500000000021D2,108),
		pair<DWORD64, int>(0x0A50000000002A13,122),
		//Baptiste:
		pair<DWORD64, int>(0x0A50000000002188,66),
		pair<DWORD64, int>(0x0A50000000002C8A,66),
		pair<DWORD64, int>(0x0A50000000002C87,66),
		pair<DWORD64, int>(0x0A50000000002C89,66),
		pair<DWORD64, int>(0x0A50000000002C88,66),
		pair<DWORD64, int>(0x0A50000000002C92,66),
		pair<DWORD64, int>(0x0A50000000002C90,69),
		pair<DWORD64, int>(0x0A50000000002CDD,50),
		pair<DWORD64, int>(0x0A50000000002C77,50),
		pair<DWORD64, int>(0x0A50000000002C7F,52),
		pair<DWORD64, int>(0x0A50000000002CDB,52),
		pair<DWORD64, int>(0x0A50000000002C7A,166),
		//Bastion:
		pair<DWORD64, int>(0x0A50000000001627,81),
		pair<DWORD64, int>(0x0A50000000001628,81),
		pair<DWORD64, int>(0x0A50000000001629,81),
		pair<DWORD64, int>(0x0A500000000018CC,81),
		pair<DWORD64, int>(0x0A500000000018CD,81),
		pair<DWORD64, int>(0x0A50000000002A46,81),
		pair<DWORD64, int>(0x0A50000000001799,98),
		pair<DWORD64, int>(0x0A5000000000162A,81),
		pair<DWORD64, int>(0x0A500000000018CE,81),
		pair<DWORD64, int>(0x0A5000000000199F,81),
		pair<DWORD64, int>(0x0A50000000001AE8,81),
		pair<DWORD64, int>(0x0A50000000001B93,81),
		pair<DWORD64, int>(0x0A50000000001798,98),
		pair<DWORD64, int>(0x0A50000000001795,109),
		pair<DWORD64, int>(0x0A50000000001794,109),
		pair<DWORD64, int>(0x0A50000000001C3E,99),
		pair<DWORD64, int>(0x0A50000000001B90,96),
		pair<DWORD64, int>(0x0A50000000001E97,97),
		pair<DWORD64, int>(0x0A50000000001623,106),
		pair<DWORD64, int>(0x0A50000000002A58,86),
		//Brigitte:
		pair<DWORD64, int>(0x0A50000000001BCE,77),
		pair<DWORD64, int>(0x0A50000000001EAB,77),
		pair<DWORD64, int>(0x0A50000000001EAA,77),
		pair<DWORD64, int>(0x0A50000000001EA8,77),
		pair<DWORD64, int>(0x0A50000000001EA9,77),
		pair<DWORD64, int>(0x0A50000000001EA5,77),
		pair<DWORD64, int>(0x0A50000000001EAD,75),
		pair<DWORD64, int>(0x0A50000000002131,77),
		pair<DWORD64, int>(0x0A50000000002A54,121),
		pair<DWORD64, int>(0x0A50000000001EC2,70),
		pair<DWORD64, int>(0x0A50000000001E80,70),
		pair<DWORD64, int>(0x0A50000000001E81,73),
		pair<DWORD64, int>(0x0A50000000001EC3,73),
		pair<DWORD64, int>(0x0A50000000001EBC,61),
		//Doomfist:
		pair<DWORD64, int>(0x0A5000000000160F,83),
		pair<DWORD64, int>(0x0A50000000001610,83),
		pair<DWORD64, int>(0x0A500000000018C3,83),
		pair<DWORD64, int>(0x0A50000000001611,83),
		pair<DWORD64, int>(0x0A50000000001612,83),
		pair<DWORD64, int>(0x0A500000000018C5,83),
		pair<DWORD64, int>(0x0A500000000018C4,83),
		pair<DWORD64, int>(0x0A50000000001C14,64),
		pair<DWORD64, int>(0x0A50000000001BE3,65),
		pair<DWORD64, int>(0x0A50000000001BD9,65),
		pair<DWORD64, int>(0x0A50000000001BE4,81),
		//Dva:

		pair<DWORD64, int>(0x0A50000000001613, 151),
		pair<DWORD64, int>(0x0A50000000001616,151),
		pair<DWORD64, int>(0x0A500000000018C6,151),
		pair<DWORD64, int>(0x0A5000000000197A,151),
		pair<DWORD64, int>(0x0A50000000001764,72),
		pair<DWORD64, int>(0x0A50000000001615, 151),
		pair<DWORD64, int>(0x0A50000000001614, 151),
		pair<DWORD64, int>(0x0A500000000018C7, 151),
		pair<DWORD64, int>(0x0A500000000018C8, 151),
		pair<DWORD64, int>(0x0A500000000017A0, 75),
		pair<DWORD64, int>(0x0A500000000017A1, 75),
		pair<DWORD64, int>(0x0A50000000001765, 72),
		pair<DWORD64, int>(0x0A50000000001C3D, 88),
		pair<DWORD64, int>(0x0A50000000001BB0, 71),
		pair<DWORD64, int>(0x0A50000000001EA2, 44),
		pair<DWORD64, int>(0x0A50000000001A19, 92),
		pair<DWORD64, int>(0x0A5000000000196C, 71),

		// Echo
		pair<DWORD64, int>(0x0A500000000020E0, 42),
		pair<DWORD64, int>(0x0A500000000046C3, 42),
		pair<DWORD64, int>(0x0A500000000046C6, 42),
		pair<DWORD64, int>(0x0A500000000046C4, 42),
		pair<DWORD64, int>(0x0A500000000046C5, 42),
		pair<DWORD64, int>(0x0A500000000046C7, 42),
		pair<DWORD64, int>(0x0A500000000046C8, 42),

		pair<DWORD64, int>(0x0A500000000046B7, 53),
		pair<DWORD64, int>(0x0A500000000046E4, 53),
		pair<DWORD64, int>(0x0A500000000046BC, 43),
		pair<DWORD64, int>(0x0A500000000046D4, 43),
		//Genji:
		pair<DWORD64, int>(0x0A500000000016D7, 53),
		pair<DWORD64, int>(0x0A500000000016D8, 53),
		pair<DWORD64, int>(0x0A500000000018EA, 53),
		pair<DWORD64, int>(0x0A500000000016D9, 53),
		pair<DWORD64, int>(0x0A500000000016DA, 53),
		pair<DWORD64, int>(0x0A500000000018EB, 53),
		pair<DWORD64, int>(0x0A50000000001785, 55),
		pair<DWORD64, int>(0x0A50000000001789, 60),
		pair<DWORD64, int>(0x0A50000000001994, 47),
		pair<DWORD64, int>(0x0A500000000018EC, 53),
		pair<DWORD64, int>(0x0A5000000000197C, 53),
		pair<DWORD64, int>(0x0A50000000001784, 55),
		pair<DWORD64, int>(0x0A50000000001788, 60),
		pair<DWORD64, int>(0x0A50000000001C1D, 54),
		pair<DWORD64, int>(0x0A50000000001B7D, 42),
		pair<DWORD64, int>(0x0A50000000001B91, 57),
		//hanzo:
		pair<DWORD64, int>(0x0A500000000015EB, 38),
		pair<DWORD64, int>(0x0A500000000015ED, 38),
		pair<DWORD64, int>(0x0A500000000018B4, 38),
		pair<DWORD64, int>(0x0A500000000015EC, 38),
		pair<DWORD64, int>(0x0A500000000015EE, 38),
		pair<DWORD64, int>(0x0A500000000018B6, 38),
		pair<DWORD64, int>(0x0A500000000018B5, 38),
		pair<DWORD64, int>(0x0A500000000019A7, 38),
		pair<DWORD64, int>(0x0A500000000019B4, 34),
		pair<DWORD64, int>(0x0A500000000015E3, 40),
		pair<DWORD64, int>(0x0A500000000015E4, 40),
		pair<DWORD64, int>(0x0A500000000015E8, 55),
		pair<DWORD64, int>(0x0A500000000015E7, 55),
		pair<DWORD64, int>(0x0A500000000019B2, 46),
		pair<DWORD64, int>(0x0A50000000002129, 42),
		pair<DWORD64, int>(0x0A50000000001C28, 33),
		pair<DWORD64, int>(0x0A500000000019B0, 33),
		//Junkrat:
		pair<DWORD64, int>(0x0A50000000001607, 100),
		pair<DWORD64, int>(0x0A50000000001608, 100),
		pair<DWORD64, int>(0x0A50000000001609, 100),
		pair<DWORD64, int>(0x0A500000000018BD, 100),
		pair<DWORD64, int>(0x0A50000000001A2D, 106),
		pair<DWORD64, int>(0x0A5000000000179C, 149),
		pair<DWORD64, int>(0x0A50000000001962, 137),
		pair<DWORD64, int>(0x0A5000000000160A, 100),
		pair<DWORD64, int>(0x0A50000000001C4D, 100),
		pair<DWORD64, int>(0x0A500000000018BE, 100),
		pair<DWORD64, int>(0x0A500000000018BF, 100),
		pair<DWORD64, int>(0x0A5000000000179D, 149),
		pair<DWORD64, int>(0x0A5000000000177D, 84),
		pair<DWORD64, int>(0x0A5000000000177C, 84),
		pair<DWORD64, int>(0x0A50000000001BC1, 128),
		pair<DWORD64, int>(0x0A500000000020E7, 146),
		pair<DWORD64, int>(0x0A50000000001C13, 99),
		pair<DWORD64, int>(0x0A50000000001C73, 155),
		pair<DWORD64, int>(0x0A50000000002CE5, 100),
		pair<DWORD64, int>(0x0A500000000041F5, 108),
		//Lucio:
		pair<DWORD64, int>(0x0A5000000000160B, 52),
		pair<DWORD64, int>(0x0A500000000018C0, 52),
		pair<DWORD64, int>(0x0A5000000000160C, 52),
		pair<DWORD64, int>(0x0A5000000000160E, 52),
		pair<DWORD64, int>(0x0A500000000018C2, 52),
		pair<DWORD64, int>(0x0A500000000018C1, 52),
		pair<DWORD64, int>(0x0A50000000001778, 52),
		pair<DWORD64, int>(0x0A5000000000160D, 52),
		pair<DWORD64, int>(0x0A50000000001A0B, 52),
		pair<DWORD64, int>(0x0A500000000020EC, 52),
		pair<DWORD64, int>(0x0A50000000001BC2, 65),
		pair<DWORD64, int>(0x0A50000000001779, 52),
		pair<DWORD64, int>(0x0A50000000001911, 46),
		pair<DWORD64, int>(0x0A50000000001910, 46),
		pair<DWORD64, int>(0x0A50000000001947, 51),
		pair<DWORD64, int>(0x0A50000000001946, 51),
		pair<DWORD64, int>(0x0A5000000000257D, 54),
		pair<DWORD64, int>(0x0A50000000001E2F, 47),
		pair<DWORD64, int>(0x0A50000000001B94, 51),
		//Mccree:
		pair<DWORD64, int>(0x0A500000000015F7, 53),
		pair<DWORD64, int>(0x0A500000000015FA, 53),
		pair<DWORD64, int>(0x0A500000000018B7, 53),
		pair<DWORD64, int>(0x0A500000000015F9, 53),
		pair<DWORD64, int>(0x0A500000000018B9, 53),
		pair<DWORD64, int>(0x0A500000000015F3, 37),
		pair<DWORD64, int>(0x0A500000000015F0, 57),
		pair<DWORD64, int>(0x0A500000000015F8, 53),
		pair<DWORD64, int>(0x0A500000000018B8, 53),
		pair<DWORD64, int>(0x0A50000000001C4C, 53),
		pair<DWORD64, int>(0x0A50000000001976, 53),
		pair<DWORD64, int>(0x0A500000000021CD, 53),
		pair<DWORD64, int>(0x0A50000000001A12, 56),
		pair<DWORD64, int>(0x0A500000000015F4, 37),
		pair<DWORD64, int>(0x0A500000000015EF, 57),
		pair<DWORD64, int>(0x0A50000000001BCD, 94),
		pair<DWORD64, int>(0x0A50000000001BCA, 85),
		pair<DWORD64, int>(0x0A50000000001C12, 125),
		pair<DWORD64, int>(0x0A500000000019B6, 94),
		pair<DWORD64, int>(0x0A50000000001E0C, 97),
		pair<DWORD64, int>(0x0A50000000002A08, 54),
		pair<DWORD64, int>(0x0A50000000002CD5, 54),
		//Mei:
		pair<DWORD64, int>(0x0A5000000000161F, 50),
		pair<DWORD64, int>(0x0A50000000001620, 50),
		pair<DWORD64, int>(0x0A500000000018C9, 50),
		pair<DWORD64, int>(0x0A50000000001621, 50),
		pair<DWORD64, int>(0x0A50000000001982, 72),
		pair<DWORD64, int>(0x0A50000000001B67, 58),
		pair<DWORD64, int>(0x0A50000000001622, 50),
		pair<DWORD64, int>(0x0A500000000018CB, 50),
		pair<DWORD64, int>(0x0A500000000018CA, 50),
		pair<DWORD64, int>(0x0A500000000021AC, 50),
		pair<DWORD64, int>(0x0A50000000001BE1, 58),
		pair<DWORD64, int>(0x0A500000000017A5, 55),
		pair<DWORD64, int>(0x0A500000000017A4, 55),
		pair<DWORD64, int>(0x0A50000000001915, 106),
		pair<DWORD64, int>(0x0A50000000001914, 106),
		pair<DWORD64, int>(0x0A50000000001BEC, 112),
		pair<DWORD64, int>(0x0A50000000001A10, 50),
		pair<DWORD64, int>(0x0A50000000001981, 86),
		pair<DWORD64, int>(0x0A500000000046A2, 115),
		//Mercy:
		pair<DWORD64, int>(0x0A500000000015DB, 150),
		pair<DWORD64, int>(0x0A500000000018AE, 150),
		pair<DWORD64, int>(0x0A500000000015DC, 150),
		pair<DWORD64, int>(0x0A500000000015DE, 150),
		pair<DWORD64, int>(0x0A500000000015D4, 149),
		pair<DWORD64, int>(0x0A500000000015D8, 157),
		pair<DWORD64, int>(0x0A500000000015DD, 168),
		pair<DWORD64, int>(0x0A500000000018B0, 168),
		pair<DWORD64, int>(0x0A500000000018AF, 168),
		pair<DWORD64, int>(0x0A50000000001979, 168),
		pair<DWORD64, int>(0x0A50000000001A2A, 168),
		pair<DWORD64, int>(0x0A500000000015D3, 149),
		pair<DWORD64, int>(0x0A500000000015D7, 157),
		pair<DWORD64, int>(0x0A50000000001BC6, 171),
		pair<DWORD64, int>(0x0A5000000000196A, 157),
		pair<DWORD64, int>(0x0A50000000001F71, 171),
		pair<DWORD64, int>(0x0A50000000001C11, 170),
		pair<DWORD64, int>(0x0A50000000001B7B, 152),
		//Moira:
		pair<DWORD64, int>(0x0A50000000001BE8, 64),
		pair<DWORD64, int>(0x0A50000000001C5E, 64),
		pair<DWORD64, int>(0x0A50000000001C5F, 64),
		pair<DWORD64, int>(0x0A50000000001C61, 64),
		pair<DWORD64, int>(0x0A50000000001C60, 64),
		pair<DWORD64, int>(0x0A50000000001C62, 64),
		pair<DWORD64, int>(0x0A50000000001C63, 64),
		pair<DWORD64, int>(0x0A50000000002136, 64),
		pair<DWORD64, int>(0x0A50000000001C6A, 69),
		pair<DWORD64, int>(0x0A50000000001C42, 69),
		pair<DWORD64, int>(0x0A50000000001C45, 114),
		pair<DWORD64, int>(0x0A50000000001C79, 114),
		pair<DWORD64, int>(0x0A5000000000212A, 51),
		pair<DWORD64, int>(0x0A50000000001C75, 42),
		pair<DWORD64, int>(0x0A50000000002C72, 95),
		//Orisa:
		pair<DWORD64, int>(0x0A50000000001933, 50),
		pair<DWORD64, int>(0x0A50000000001935, 50),
		pair<DWORD64, int>(0x0A50000000001934, 50),
		pair<DWORD64, int>(0x0A50000000001937, 50),
		pair<DWORD64, int>(0x0A50000000001936, 50),
		pair<DWORD64, int>(0x0A50000000001938, 50),
		pair<DWORD64, int>(0x0A50000000001B95, 49),
		pair<DWORD64, int>(0x0A50000000002A11, 58),
		pair<DWORD64, int>(0x0A50000000001C10, 58),
		pair<DWORD64, int>(0x0A50000000001B62, 59),
		pair<DWORD64, int>(0x0A50000000001B65, 59),
		pair<DWORD64, int>(0x0A50000000001B64, 55),
		pair<DWORD64, int>(0x0A50000000001B63, 55),
		pair<DWORD64, int>(0x0A50000000001B61, 54),
		pair<DWORD64, int>(0x0A50000000001C6F, 56),
		//Pharah:
		pair<DWORD64, int>(0x0A500000000016BF, 38),
		pair<DWORD64, int>(0x0A500000000016C2, 38),
		pair<DWORD64, int>(0x0A50000000001768, 49),
		pair<DWORD64, int>(0x0A500000000016C1, 38),
		pair<DWORD64, int>(0x0A500000000018E1, 38),
		pair<DWORD64, int>(0x0A500000000016C0, 38),
		pair<DWORD64, int>(0x0A50000000001929, 42),
		pair<DWORD64, int>(0x0A50000000001928, 42),
		pair<DWORD64, int>(0x0A500000000019A0, 38),
		pair<DWORD64, int>(0x0A50000000001A16, 42),
		pair<DWORD64, int>(0x0A500000000019EA, 92),
		pair<DWORD64, int>(0x0A50000000001769, 49),
		pair<DWORD64, int>(0x0A5000000000192A, 112),
		pair<DWORD64, int>(0x0A5000000000192B, 112),
		pair<DWORD64, int>(0x0A500000000020E6, 5), // voir 6 ou 87
		pair<DWORD64, int>(0x0A50000000001C0B, 38),
		pair<DWORD64, int>(0x0A500000000019E6, 111),
		pair<DWORD64, int>(0x0A500000000016A7, 105),
		pair<DWORD64, int>(0x0A50000000002D7C, 108),
		//Reaper:
		pair<DWORD64, int>(0x0A50000000001697, 36),
		pair<DWORD64, int>(0x0A50000000001699, 36),
		pair<DWORD64, int>(0x0A500000000018DB, 36),
		pair<DWORD64, int>(0x0A500000000018DD, 36),
		pair<DWORD64, int>(0x0A5000000000167C, 47),
		pair<DWORD64, int>(0x0A5000000000169A, 36),
		pair<DWORD64, int>(0x0A50000000001698, 36),
		pair<DWORD64, int>(0x0A500000000018DC, 36),
		pair<DWORD64, int>(0x0A50000000001C3C, 36),
		pair<DWORD64, int>(0x0A50000000001A07, 36),
		pair<DWORD64, int>(0x0A50000000001770, 49),
		pair<DWORD64, int>(0x0A50000000001771, 49),
		pair<DWORD64, int>(0x0A5000000000167B, 47),
		pair<DWORD64, int>(0x0A50000000001BC0, 45),
		pair<DWORD64, int>(0x0A5000000000195A, 58),
		pair<DWORD64, int>(0x0A50000000001BEB, 41),
		pair<DWORD64, int>(0x0A500000000021C1, 38),
		pair<DWORD64, int>(0x0A50000000001C74, 34),
		pair<DWORD64, int>(0x0A5000000000168B, 50),
		//Reinhardt:
		pair<DWORD64, int>(0x0A5000000000165F, 41),
		pair<DWORD64, int>(0x0A50000000001660, 41),
		pair<DWORD64, int>(0x0A50000000001662, 41),
		pair<DWORD64, int>(0x0A500000000018D6, 41),
		pair<DWORD64, int>(0x0A5000000000163F, 41),
		pair<DWORD64, int>(0x0A50000000001954, 53),
		pair<DWORD64, int>(0x0A500000000018D5, 41),
		pair<DWORD64, int>(0x0A50000000001661, 41),
		pair<DWORD64, int>(0x0A500000000017B1, 41),
		pair<DWORD64, int>(0x0A500000000019A1, 41),
		pair<DWORD64, int>(0x0A50000000002A0C, 41),
		pair<DWORD64, int>(0x0A50000000001B87, 41),
		pair<DWORD64, int>(0x0A50000000001B7E, 37),
		pair<DWORD64, int>(0x0A50000000001640, 41),
		pair<DWORD64, int>(0x0A5000000000163B, 39),
		pair<DWORD64, int>(0x0A5000000000163C, 39),
		pair<DWORD64, int>(0x0A50000000001955, 53),
		pair<DWORD64, int>(0x0A50000000001EB6, 37),
		pair<DWORD64, int>(0x0A500000000021BB, 38),
		pair<DWORD64, int>(0x0A50000000001991, 44),
		//Roadhog:
		pair<DWORD64, int>(0x0A500000000016A3, 61),
		pair<DWORD64, int>(0x0A500000000018DE, 61),
		pair<DWORD64, int>(0x0A500000000016A4, 61),
		pair<DWORD64, int>(0x0A50000000001A14, 61),
		pair<DWORD64, int>(0x0A50000000001C1C, 80),
		pair<DWORD64, int>(0x0A500000000016A5, 61),
		pair<DWORD64, int>(0x0A500000000016A6, 61),
		pair<DWORD64, int>(0x0A500000000018E0, 61),
		pair<DWORD64, int>(0x0A500000000018DF, 61),
		pair<DWORD64, int>(0x0A50000000001C1B, 74),
		pair<DWORD64, int>(0x0A500000000016A0, 53),
		pair<DWORD64, int>(0x0A5000000000169F, 53),
		pair<DWORD64, int>(0x0A5000000000169C, 65),
		pair<DWORD64, int>(0x0A5000000000169B, 65),
		pair<DWORD64, int>(0x0A500000000020E9, 112),
		pair<DWORD64, int>(0x0A5000000000195C, 62),
		pair<DWORD64, int>(0x0A5000000000197D, 53),
		pair<DWORD64, int>(0x0A50000000002D76, 78),
		//Soldier:
		pair<DWORD64, int>(0x0A500000000016C7, 38),
		pair<DWORD64, int>(0x0A500000000016C9, 38),
		pair<DWORD64, int>(0x0A500000000016CA, 38),
		pair<DWORD64, int>(0x0A500000000018E4, 38),
		pair<DWORD64, int>(0x0A500000000016C8, 38),
		pair<DWORD64, int>(0x0A500000000018E6, 38),
		pair<DWORD64, int>(0x0A500000000018E5, 38),
		pair<DWORD64, int>(0x0A500000000019A6, 38),
		pair<DWORD64, int>(0x0A500000000020E8, 38),
		pair<DWORD64, int>(0x0A5000000000178C, 55),
		pair<DWORD64, int>(0x0A5000000000178D, 55),
		pair<DWORD64, int>(0x0A50000000001791, 54),
		pair<DWORD64, int>(0x0A50000000001790, 54),
		pair<DWORD64, int>(0x0A50000000001BC8, 41),
		pair<DWORD64, int>(0x0A50000000001EBE, 47),
		pair<DWORD64, int>(0x0A50000000001C1E, 45),
		pair<DWORD64, int>(0x0A50000000001A1E, 116),
		pair<DWORD64, int>(0x0A500000000016C3, 43),
		pair<DWORD64, int>(0x0A50000000002A5D, 46),
		//Sombra:
		pair<DWORD64, int>(0x0A5000000000162B, 42),
		pair<DWORD64, int>(0x0A5000000000162D, 42),
		pair<DWORD64, int>(0x0A5000000000162E, 42),
		pair<DWORD64, int>(0x0A50000000001A1D, 42),
		pair<DWORD64, int>(0x0A5000000000162C, 42),
		pair<DWORD64, int>(0x0A500000000018CF, 42),
		pair<DWORD64, int>(0x0A500000000018D0, 42),
		pair<DWORD64, int>(0x0A500000000018D1, 42),
		pair<DWORD64, int>(0x0A50000000002138, 42),
		pair<DWORD64, int>(0x0A5000000000198B, 57),
		pair<DWORD64, int>(0x0A5000000000198A, 57),
		pair<DWORD64, int>(0x0A5000000000198C, 36),
		pair<DWORD64, int>(0x0A5000000000198D, 36),
		pair<DWORD64, int>(0x0A50000000001BC7, 37),
		pair<DWORD64, int>(0x0A500000000020DB, 52),
		pair<DWORD64, int>(0x0A50000000001C16, 46),
		pair<DWORD64, int>(0x0A50000000001C17, 36),
		pair<DWORD64, int>(0x0A5000000000213B, 49),
		// SIGMA
		pair<DWORD64, int>(0x0A500000000028CF, 110),
		pair<DWORD64, int>(0x0A50000000002D65, 110),
		pair<DWORD64, int>(0x0A50000000002D64, 110),
		pair<DWORD64, int>(0x0A50000000002D62, 110),
		pair<DWORD64, int>(0x0A50000000002D66, 110),
		pair<DWORD64, int>(0x0A50000000002D67, 110),

		//Symmetra:
		pair<DWORD64, int>(0x0A500000000016E7, 89),
		pair<DWORD64, int>(0x0A500000000016E9, 89),
		pair<DWORD64, int>(0x0A50000000001998, 89),
		pair<DWORD64, int>(0x0A500000000016E4, 86),
		pair<DWORD64, int>(0x0A500000000016E3, 86),
		pair<DWORD64, int>(0x0A500000000016DB, 92),
		pair<DWORD64, int>(0x0A500000000016E8, 89),
		pair<DWORD64, int>(0x0A500000000016EA, 89),
		pair<DWORD64, int>(0x0A500000000018ED, 89),
		pair<DWORD64, int>(0x0A50000000001C44, 89),
		pair<DWORD64, int>(0x0A500000000018EE, 89),
		pair<DWORD64, int>(0x0A500000000018EF, 89),
		pair<DWORD64, int>(0x0A50000000001A27, 89),
		pair<DWORD64, int>(0x0A500000000016DC, 92),
		pair<DWORD64, int>(0x0A500000000016DF, 97),
		pair<DWORD64, int>(0x0A50000000002133, 90),
		pair<DWORD64, int>(0x0A50000000001E0A, 95),
		pair<DWORD64, int>(0x0A50000000001B92, 83),
		pair<DWORD64, int>(0x0A500000000046A3, 89),
		//Torbjorn:
		pair<DWORD64, int>(0x0A50000000001637, 45),
		pair<DWORD64, int>(0x0A50000000001638, 45),
		pair<DWORD64, int>(0x0A50000000001639, 45),
		pair<DWORD64, int>(0x0A50000000001BAD, 64),
		pair<DWORD64, int>(0x0A5000000000163A, 45),
		pair<DWORD64, int>(0x0A500000000018D2, 45),
		pair<DWORD64, int>(0x0A500000000018D4, 45),
		pair<DWORD64, int>(0x0A500000000018D3, 45),
		pair<DWORD64, int>(0x0A5000000000197B, 45),
		pair<DWORD64, int>(0x0A50000000001C18, 92),
		pair<DWORD64, int>(0x0A5000000000162F, 51),
		pair<DWORD64, int>(0x0A50000000001630, 51),
		pair<DWORD64, int>(0x0A50000000001633, 59),
		pair<DWORD64, int>(0x0A50000000001634, 59),
		pair<DWORD64, int>(0x0A50000000001BCB, 96),
		pair<DWORD64, int>(0x0A50000000001968, 92),
		pair<DWORD64, int>(0x0A500000000021AB, 55),
		pair<DWORD64, int>(0x0A50000000001B60, 121),
		pair<DWORD64, int>(0x0A50000000001C6E, 118),
		pair<DWORD64, int>(0x0A500000000046A6, 46),
		//Tracer:
		pair<DWORD64, int>(0x0A5000000000170F, 52),
		pair<DWORD64, int>(0x0A50000000001711, 52),
		pair<DWORD64, int>(0x0A50000000001712, 52),
		pair<DWORD64, int>(0x0A50000000001710, 52),
		pair<DWORD64, int>(0x0A500000000018F1, 52),
		pair<DWORD64, int>(0x0A50000000001942, 45),
		pair<DWORD64, int>(0x0A5000000000195E, 46),
		pair<DWORD64, int>(0x0A500000000018F0, 52),
		pair<DWORD64, int>(0x0A500000000018F2, 52),
		pair<DWORD64, int>(0x0A50000000001B07, 52),
		pair<DWORD64, int>(0x0A500000000020EB, 52),
		pair<DWORD64, int>(0x0A500000000016EB, 49),
		pair<DWORD64, int>(0x0A500000000016EC, 49),
		pair<DWORD64, int>(0x0A5000000000170B, 50),
		pair<DWORD64, int>(0x0A5000000000170C, 50),
		pair<DWORD64, int>(0x0A50000000001943, 45),
		pair<DWORD64, int>(0x0A500000000021F2, 58),
		pair<DWORD64, int>(0x0A50000000001B66, 41),
		pair<DWORD64, int>(0x0A50000000001B2B, 37),
		pair<DWORD64, int>(0x0A500000000016F7, 123),
		//Widowmaker:
		pair<DWORD64, int>(0x0A5000000000171B, 40),
		pair<DWORD64, int>(0x0A5000000000171C, 40),
		pair<DWORD64, int>(0x0A5000000000171E, 40),
		pair<DWORD64, int>(0x0A500000000018F3, 40),
		pair<DWORD64, int>(0x0A5000000000171D, 40),
		pair<DWORD64, int>(0x0A50000000001C21, 38),
		pair<DWORD64, int>(0x0A500000000018F4, 40),
		pair<DWORD64, int>(0x0A500000000018F5, 40),
		pair<DWORD64, int>(0x0A50000000001978, 40),
		pair<DWORD64, int>(0x0A500000000021BA, 40),
		pair<DWORD64, int>(0x0A50000000001714, 39),
		pair<DWORD64, int>(0x0A50000000001713, 39),
		pair<DWORD64, int>(0x0A50000000001919, 47),
		pair<DWORD64, int>(0x0A50000000001918, 47),
		pair<DWORD64, int>(0x0A50000000001C20, 38),
		pair<DWORD64, int>(0x0A50000000001BC9, 38),
		pair<DWORD64, int>(0x0A50000000002113, 46),
		pair<DWORD64, int>(0x0A50000000001C1F, 46),
		pair<DWORD64, int>(0x0A50000000001BAF, 38),
		pair<DWORD64, int>(0x0A50000000001717, 42), // ���Ƹ�
		//Winston:
		pair<DWORD64, int>(0x0A500000000016D3, 109),
		pair<DWORD64, int>(0x0A500000000016D6, 109),
		pair<DWORD64, int>(0x0A500000000018E7, 109),
		pair<DWORD64, int>(0x0A500000000018E9, 109),
		pair<DWORD64, int>(0x0A5000000000197F, 87),
		pair<DWORD64, int>(0x0A500000000016D5, 109),
		pair<DWORD64, int>(0x0A500000000016D4, 109),
		pair<DWORD64, int>(0x0A500000000018E8, 109),
		pair<DWORD64, int>(0x0A500000000016CF, 61),
		pair<DWORD64, int>(0x0A500000000016D0, 61),
		pair<DWORD64, int>(0x0A500000000016CB, 105),
		pair<DWORD64, int>(0x0A500000000016CC, 105),
		pair<DWORD64, int>(0x0A50000000001EAC, 61),
		pair<DWORD64, int>(0x0A50000000001964, 61),
		pair<DWORD64, int>(0x0A50000000001CC6, 56),
		//WreckingBall:
		pair<DWORD64, int>(0x0A50000000001C56, 199),
		pair<DWORD64, int>(0x0A5000000000213F, 199),
		pair<DWORD64, int>(0x0A5000000000213D, 199),
		pair<DWORD64, int>(0x0A5000000000213C, 199),
		pair<DWORD64, int>(0x0A5000000000213E, 199),
		pair<DWORD64, int>(0x0A50000000002143, 199),
		pair<DWORD64, int>(0x0A50000000002142, 199),
		pair<DWORD64, int>(0x0A5000000000214A, 145),
		pair<DWORD64, int>(0x0A50000000002128, 138),
		pair<DWORD64, int>(0x0A50000000002147, 153),
		pair<DWORD64, int>(0x0A50000000002130, 153),
		pair<DWORD64, int>(0x0A5000000000214E, 153),
		pair<DWORD64, int>(0x0A50000000002CD8, 199),
		//Zarya:
		pair<DWORD64, int>(0x0A5000000000166B, 65),
		pair<DWORD64, int>(0x0A5000000000166E, 65),
		pair<DWORD64, int>(0x0A500000000018D8, 65),
		pair<DWORD64, int>(0x0A5000000000166D, 65),
		pair<DWORD64, int>(0x0A5000000000166C, 65),
		pair<DWORD64, int>(0x0A500000000018D9, 65),
		pair<DWORD64, int>(0x0A50000000001A1B, 65),
		pair<DWORD64, int>(0x0A50000000001664, 36),
		pair<DWORD64, int>(0x0A500000000018DA, 65),
		pair<DWORD64, int>(0x0A50000000001C1A, 51),
		pair<DWORD64, int>(0x0A50000000001668, 43),
		pair<DWORD64, int>(0x0A50000000001667, 43),
		pair<DWORD64, int>(0x0A50000000001663, 36),
		pair<DWORD64, int>(0x0A50000000001944, 52),
		pair<DWORD64, int>(0x0A50000000001945, 52),
		pair<DWORD64, int>(0x0A5000000000196E, 57),
		pair<DWORD64, int>(0x0A50000000002134, 76),
		pair<DWORD64, int>(0x0A50000000001C19, 58),
		pair<DWORD64, int>(0x0A50000000001B6A, 49),
		pair<DWORD64, int>(0x0A50000000002CDF, 65),
		pair<DWORD64, int>(0x0A500000000045BE, 109),
		//Zenyatta:
		pair<DWORD64, int>(0x0A50000000001603, 149),
		pair<DWORD64, int>(0x0A500000000018BA, 149),
		pair<DWORD64, int>(0x0A50000000001604, 149),
		pair<DWORD64, int>(0x0A500000000018BB, 149),
		pair<DWORD64, int>(0x0A500000000015FB, 52),
		pair<DWORD64, int>(0x0A50000000001966, 99),
		pair<DWORD64, int>(0x0A50000000001605, 149),
		pair<DWORD64, int>(0x0A50000000001606, 149),
		pair<DWORD64, int>(0x0A50000000001C40, 149),
		pair<DWORD64, int>(0x0A500000000018BC, 149),
		pair<DWORD64, int>(0x0A500000000019A3, 149),
		pair<DWORD64, int>(0x0A500000000015FC, 52),
		pair<DWORD64, int>(0x0A500000000015FF, 98),
		pair<DWORD64, int>(0x0A50000000001600, 98),
		pair<DWORD64, int>(0x0A50000000001E2D, 89),
		pair<DWORD64, int>(0x0A50000000001BE7, 128),
		pair<DWORD64, int>(0x0A50000000001960, 110),
		pair<DWORD64, int>(0x0A500000000021D1, 93),
		//Training Bots:
		pair<DWORD64, int>(0x0A50000000001AFC, 37),
		pair<DWORD64, int>(0x0A50000000001AF8, 37),
		pair<DWORD64, int>(0x0A50000000001AFA, 37),
		};
	};
