#pragma once
#include <Windows.h>
#include <iostream>

struct offset
{
	static uint64_t AngleHook;
	static uint64_t BorderLine;
	static uint64_t FovHook;
	static uint64_t EntityHook;
	static uint64_t CompoenetHook;
	static uint64_t Skill;
	static uint64_t ViewMatrixOffset;
};

uint64_t offset::AngleHook             = 0x6e78ec;   //패치
uint64_t offset::BorderLine            = 0xbab1a8;   //패치
uint64_t offset::FovHook               = 0x6e5027;   //패치
uint64_t offset::CompoenetHook         = 0x86CF99;  //안씀
uint64_t offset::ViewMatrixOffset      = 0x3000C48;  //패치

#define Compo__SKILL                     0x2F // 0x2F


#define OFFSET_BADREADPTR_SIZEDEFAULT    0x540
#define OFFSET_BADREADPTR_SIZEBONES      0xE474
#define OFFSET_BADREADPTR_SIZEROTSTRUCT  0xB00


#define OFFSET_GenjiQ								0x24B
#define OFFSET_PLAYER_CONTROLLER_ROTATION			0x1090
#define OFFSET_PLAYER_CONTROLLER_KEY				0x1034
#define OFFSET_PLAYER_CONTROLLER_DELTA				(OFFSET_PLAYER_CONTROLLER_ROTATION + 0x4C)

#define OFFSET_UNIT_VELOCITY						0x4
#define OFFSET_UNIT_COMPNENT_TO_WORLD				0x10
#define OFFSET_PLAYER_VISIBILITY					0x2D 
#define OFFSET_UNIT_HEALTH							0x33 
#define OFFSET_PLAYER_HEROIDENTITY					0x4B
#define OFFSET_UNIT_HASPLAYERID						0x2B
#define OFFSET_UNIT_ROTATIONBASE					0x27
#define OFFSET_UINT_LINK							0x2C
#define OFFSET_UINT_SKILL							0x2F
#define OFFSET_UINT_OUTLINE							0x53


#define OFFSET_HEALTHPTR_HEALTH						0xE0 // OK
#define OFFSET_HEALTHPTR_HEALTHMAX					0xDC // OK
#define OFFSET_HEALTHPTR_ARMOR						0x220 // OK
#define OFFSET_HEALTHPTR_ARMORMAX					0x21C // OK
#define OFFSET_HEALTHPTR_BARRIER					0x360 // OK
#define OFFSET_HEALTHPTR_BARRIERMAX					0x35C // OK

#define OFFSET_HEALTHPTR_TAG						0x8 // OK

#define OFFSET_UCWPIDPTR_COMPOID					0xD0 // OK
#define OFFSET_HEROIDPTR_COMPOID					0xE8// OK //0xD8
#define OFFSET_HEALTHPTR_TEAM						0x504// OK
#define OFFSET_HEROIDPTR_HEROID						0xE8 // OK
#define OFFSET_HEROIDPTR_SKINID						0xD0 // OK

#define OFFSET_VELOCITYPTR_LOCATION					0x140 // ㅇ
#define OFFSET_VELOCITYPTR_ENCRYPTED				0x80 // d
#define OFFSET_VELOCITYPTR_VELOCITY					0x50  // ㅇ
#define OFFSET_VELOCITYPTR_BONEDATA					0x6C0 // ㅇ
#define OFFSET_BONEDATA_BONEBASE					0x28 //  ㅇ
#define OFFSET_BONE_SIZE							0x30 //  ㅇ
#define OFFSET_BONE_LOCATION						0x20 // ㅇ
#define OFFSET_BONE_ROTCATION						0x0 // ㅇ

#define OFFSET_BONEDATA_BONESCOUNT					0x48 // OK?
#define OFFSET_VISIBILITYPTR_ISVISIBLE				0x98 //98

#define OFFSET_ROTATIONPTRPTR_ROTSTRUCT				0x1598 // // OK
#define OFFSET_ROTSTRUCT_ROT						0xA98 // OK
#define OFFSET__SKill								0xD0