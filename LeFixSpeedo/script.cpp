#include "script.h"

#include "VehicleData.h"
#include "speedoEnums.h"
#include "Settings.h"
#include "Util\INIutils.hpp"
#include "Util\UIutils.hpp"
#include "menu.h"
#include "Math.h"

#include <menu.h>
#include "Util/Logger.hpp"
#include "Util/Versions.h"

#include "Compatibility.h"

Player player;				//Reference to actual player
Ped playerPed;				//Reference to actual ped

NativeMenu::Menu menu;
VehicleData vehData;

bool isInMenu = false;
bool isPassenger = false;
int addFast = 0;				//Vehicle faster than expected, next Speedo
float screenCorrect = 1.0f;
float hudSize = 1.0f;

//Velocity Values Picture Based (Unit...)
float velPicMax[3][2][3];
float velPicCarFast[2][2][3];
float velPicCarSlow[2][2][3];
float velPicLinFast[3][2][3];
float velPicLinSlow[3][2][3];
//Velocity Values Vehicle Based (Vehicle Type, Unit, isFast, Degree/Velocity, Segment)(References to the values above)
float velMatrix[numVEHDOMAIN][numUNIT][3][2][3];

//Textures
int texMatrix[5][4][3];		//Texture references: Vehicle Type, Unit, Fast
int idUnit[3];				//Unit: Text
int idGear[10];				//Gear Nums: R, N, 1 - 8
int idGearShiftDown, idGearShiftUp;
int idPlaVert;				//Vertical Velocity Scale+Text
int idRpmText;				//RPM: Text
int idRpm3, idRpm5, idRpm6, idRpm8, idRpm9, idRpm10, idRpm12;	//RPM: Scales Max 3,5,6,8,9,10,12
int idRpmLedOn, idRpmLedOff;									//RPM: LED
int idShadowMain, idShadowLeft, idShadowAbove, idDmg;			//Background: Shadow, Carbon, etc.
int idNeedleBigA, idNeedleBigB, idNeedleSmallA, idNeedleSmallB;	//Needles
int idFuelScale, idExtFuel[3];		//Fuel
int idBrandLogo;

//Dash Data
float velRot, vertRot, rpmRot, fuelRot;

//OPTICS
float aTimeAdjust, fade;
bool isPrimColorValid = false;
float needlePrimCol[3];
float *needleActCol[3];

float aDmg;

//PLACEMENT
float mainPosX, mainPosY;
float extraPosX, extraPosY;
float extraPosX2, extraPosY2;
float rpmLedX, rpmLedY, rpmGap;

////EXTENDED////

static float ledCol[numRGB][9]; //[RGB][SEG]

//-------------------------------------------------------------------------------------

//                                      METHODS                                        

//-------------------------------------------------------------------------------------

void drawTexture(int id, int index, int level, float size, float centerX, float centerY, float posX, float posY, float rotation, float r, float g, float b, float a)
{
	drawTexture(id, index, level, 150, hudSize*Settings::hudSizeConst*size, hudSize*Settings::hudSizeConst*size, centerX, centerY, posX, posY, rotation, screenCorrect, r, g, b, a*fade);
}
void drawTexture(int id, int index, int level, float size, float posX, float posY, float rotation, float r, float g, float b, float a)
{
	drawTexture(id, index, level, size, 0.5f, 0.5f, posX, posY, rotation, r, g, b, a);
}
void drawTexture(int id, int index, int level, float posX, float posY, float a)
{
	drawTexture(id, index, level, 1.0f, posX, posY, 0.0f, 1.0f, 1.0f, 1.0f, a);
}
void stopTexture(int id, int index)
{
	drawTexture(id, index, -9999, 0, 0, 0);
}

//Setup Velocity Parameters and Textures
int create_texture(std::string file)
{
	std::string path = GetCurrentModulePath(); // includes trailing slash
	return createTexture((path + "LeFixSpeedo\\" + file).c_str());
}
int getIsFast(){
	int fast = 0;
	if (vehData.vDomain == VD_ROAD)
	{
		if (Settings::useHighSpeedoByDefault)
		{
			switch (vehData.vClass)
			{
			case VC_SPORT: case VC_SUPER: fast = 2; break;
			case VC_COMPACT: case VC_SEDAN: case VC_SUV: case VC_COUPE: case VC_MUSCLE: case VC_SPORTCLASSIC: case VC_MOTORCYCLE: case VC_EMERGENCY: fast = (1 + addFast); break;
			default: fast = (0 + addFast); break;
			}
		}
		else
		{
			switch (vehData.vClass)
			{
			case VC_SEDAN: case VC_COUPE: case VC_MUSCLE: case VC_SPORTCLASSIC: case VC_SPORT: case VC_SUPER: case VC_MOTORCYCLE: fast = (1 + addFast); break;
			default: fast = (0 + addFast); break;
			}
		}
	}
	if (vehData.vDomain == VD_WATER && Settings::useHighSpeedoByDefault) fast = 1;
	if (vehData.vDomain == VD_RAIL) fast = (0 + addFast);
	return fast;
}
void checkIsFasterThanExpected()
{
	if (vehData.vDomain == VD_ROAD)
	{
		if (getIsFast() < 2)
		{
			if (vehData.gearAcc > 0.1f && velRot > velMatrix[vehData.vDomain][Settings::unit[vehData.vDomain]][getIsFast()][0][2] - 0.01f)
			{
				addFast++;
			}
		}
	}
	if (vehData.vDomain == VD_RAIL)
	{
		if (getIsFast() < 1)
		{
			if (velRot > velMatrix[vehData.vDomain][Settings::unit[vehData.vDomain]][getIsFast()][0][2] - 0.05f)
			{
				addFast++;
			}
		}
	}
}
void setupVelMatrix()
{
	//vDomain, Unit, isFast, Degree/Velocity, Number

	//Bicycle

	//MPH SLOW
	velMatrix[VD_VELO][MPH][0][0][0] = velPicLinSlow[MPH][0][0];
	velMatrix[VD_VELO][MPH][0][0][1] = velPicLinSlow[MPH][0][1];
	velMatrix[VD_VELO][MPH][0][0][2] = velPicLinSlow[MPH][0][2];
	velMatrix[VD_VELO][MPH][0][1][0] = velPicLinSlow[MPH][1][0];
	velMatrix[VD_VELO][MPH][0][1][1] = velPicLinSlow[MPH][1][1];
	velMatrix[VD_VELO][MPH][0][1][2] = velPicLinSlow[MPH][1][2];
	//KMH SLOW
	velMatrix[VD_VELO][KMH][0][0][0] = velPicLinSlow[KMH][0][0];
	velMatrix[VD_VELO][KMH][0][0][1] = velPicLinSlow[KMH][0][1];
	velMatrix[VD_VELO][KMH][0][0][2] = velPicLinSlow[KMH][0][2];
	velMatrix[VD_VELO][KMH][0][1][0] = velPicLinSlow[KMH][1][0];
	velMatrix[VD_VELO][KMH][0][1][1] = velPicLinSlow[KMH][1][1];
	velMatrix[VD_VELO][KMH][0][1][2] = velPicLinSlow[KMH][1][2];

	//Boat

	//MPH FAST
	velMatrix[VD_WATER][MPH][1][0][0] = velPicLinFast[MPH][0][0];
	velMatrix[VD_WATER][MPH][1][0][1] = velPicLinFast[MPH][0][1];
	velMatrix[VD_WATER][MPH][1][0][2] = velPicLinFast[MPH][0][2];
	velMatrix[VD_WATER][MPH][1][1][0] = velPicLinFast[MPH][1][0];
	velMatrix[VD_WATER][MPH][1][1][1] = velPicLinFast[MPH][1][1];
	velMatrix[VD_WATER][MPH][1][1][2] = velPicLinFast[MPH][1][2];
	//KMH FAST
	velMatrix[VD_WATER][KMH][1][0][0] = velPicLinFast[KMH][0][0];
	velMatrix[VD_WATER][KMH][1][0][1] = velPicLinFast[KMH][0][1];
	velMatrix[VD_WATER][KMH][1][0][2] = velPicLinFast[KMH][0][2];
	velMatrix[VD_WATER][KMH][1][1][0] = velPicLinFast[KMH][1][0];
	velMatrix[VD_WATER][KMH][1][1][1] = velPicLinFast[KMH][1][1];
	velMatrix[VD_WATER][KMH][1][1][2] = velPicLinFast[KMH][1][2];
	//KNO FAST
	velMatrix[VD_WATER][KNO][1][0][0] = velPicLinFast[KNO][0][0];
	velMatrix[VD_WATER][KNO][1][0][1] = velPicLinFast[KNO][0][1];
	velMatrix[VD_WATER][KNO][1][0][2] = velPicLinFast[KNO][0][2];
	velMatrix[VD_WATER][KNO][1][1][0] = velPicLinFast[KNO][1][0];
	velMatrix[VD_WATER][KNO][1][1][1] = velPicLinFast[KNO][1][1];
	velMatrix[VD_WATER][KNO][1][1][2] = velPicLinFast[KNO][1][2];

	//MPH SLOW
	velMatrix[VD_WATER][MPH][0][0][0] = velPicLinSlow[MPH][0][0];
	velMatrix[VD_WATER][MPH][0][0][1] = velPicLinSlow[MPH][0][1];
	velMatrix[VD_WATER][MPH][0][0][2] = velPicLinSlow[MPH][0][2];
	velMatrix[VD_WATER][MPH][0][1][0] = velPicLinSlow[MPH][1][0];
	velMatrix[VD_WATER][MPH][0][1][1] = velPicLinSlow[MPH][1][1];
	velMatrix[VD_WATER][MPH][0][1][2] = velPicLinSlow[MPH][1][2];
	//KMH SLOW
	velMatrix[VD_WATER][KMH][0][0][0] = velPicLinSlow[KMH][0][0];
	velMatrix[VD_WATER][KMH][0][0][1] = velPicLinSlow[KMH][0][1];
	velMatrix[VD_WATER][KMH][0][0][2] = velPicLinSlow[KMH][0][2];
	velMatrix[VD_WATER][KMH][0][1][0] = velPicLinSlow[KMH][1][0];
	velMatrix[VD_WATER][KMH][0][1][1] = velPicLinSlow[KMH][1][1];
	velMatrix[VD_WATER][KMH][0][1][2] = velPicLinSlow[KMH][1][2];
	//KNO SLOW
	velMatrix[VD_WATER][KNO][0][0][0] = velPicLinSlow[KNO][0][0];
	velMatrix[VD_WATER][KNO][0][0][1] = velPicLinSlow[KNO][0][1];
	velMatrix[VD_WATER][KNO][0][0][2] = velPicLinSlow[KNO][0][2];
	velMatrix[VD_WATER][KNO][0][1][0] = velPicLinSlow[KNO][1][0];
	velMatrix[VD_WATER][KNO][0][1][1] = velPicLinSlow[KNO][1][1];
	velMatrix[VD_WATER][KNO][0][1][2] = velPicLinSlow[KNO][1][2];

	//Car

	//MPH FAST
	velMatrix[VD_ROAD][MPH][2][0][0] = velPicCarFast[MPH][0][0];
	velMatrix[VD_ROAD][MPH][2][0][1] = velPicCarFast[MPH][0][1];
	velMatrix[VD_ROAD][MPH][2][0][2] = velPicCarFast[MPH][0][2];
	velMatrix[VD_ROAD][MPH][2][1][0] = velPicCarFast[MPH][1][0];
	velMatrix[VD_ROAD][MPH][2][1][1] = velPicCarFast[MPH][1][1];
	velMatrix[VD_ROAD][MPH][2][1][2] = velPicCarFast[MPH][1][2];
	//KMH FAST
	velMatrix[VD_ROAD][KMH][2][0][0] = velPicCarFast[KMH][0][0];
	velMatrix[VD_ROAD][KMH][2][0][1] = velPicCarFast[KMH][0][1];
	velMatrix[VD_ROAD][KMH][2][0][2] = velPicCarFast[KMH][0][2];
	velMatrix[VD_ROAD][KMH][2][1][0] = velPicCarFast[KMH][1][0];
	velMatrix[VD_ROAD][KMH][2][1][1] = velPicCarFast[KMH][1][1];
	velMatrix[VD_ROAD][KMH][2][1][2] = velPicCarFast[KMH][1][2];

	//MPH MED
	velMatrix[VD_ROAD][MPH][1][0][0] = velPicCarSlow[MPH][0][0];
	velMatrix[VD_ROAD][MPH][1][0][1] = velPicCarSlow[MPH][0][1];
	velMatrix[VD_ROAD][MPH][1][0][2] = velPicCarSlow[MPH][0][2];
	velMatrix[VD_ROAD][MPH][1][1][0] = velPicCarSlow[MPH][1][0];
	velMatrix[VD_ROAD][MPH][1][1][1] = velPicCarSlow[MPH][1][1];
	velMatrix[VD_ROAD][MPH][1][1][2] = velPicCarSlow[MPH][1][2];
	//KMH MED
	velMatrix[VD_ROAD][KMH][1][0][0] = velPicCarSlow[KMH][0][0];
	velMatrix[VD_ROAD][KMH][1][0][1] = velPicCarSlow[KMH][0][1];
	velMatrix[VD_ROAD][KMH][1][0][2] = velPicCarSlow[KMH][0][2];
	velMatrix[VD_ROAD][KMH][1][1][0] = velPicCarSlow[KMH][1][0];
	velMatrix[VD_ROAD][KMH][1][1][1] = velPicCarSlow[KMH][1][1];
	velMatrix[VD_ROAD][KMH][1][1][2] = velPicCarSlow[KMH][1][2];

	//MPH SLOW
	velMatrix[VD_ROAD][MPH][0][0][0] = velPicLinFast[MPH][0][0];
	velMatrix[VD_ROAD][MPH][0][0][1] = velPicLinFast[MPH][0][1];
	velMatrix[VD_ROAD][MPH][0][0][2] = velPicLinFast[MPH][0][2];
	velMatrix[VD_ROAD][MPH][0][1][0] = velPicLinFast[MPH][1][0];
	velMatrix[VD_ROAD][MPH][0][1][1] = velPicLinFast[MPH][1][1];
	velMatrix[VD_ROAD][MPH][0][1][2] = velPicLinFast[MPH][1][2];
	//KMH SLOW
	velMatrix[VD_ROAD][KMH][0][0][0] = velPicLinFast[KMH][0][0];
	velMatrix[VD_ROAD][KMH][0][0][1] = velPicLinFast[KMH][0][1];
	velMatrix[VD_ROAD][KMH][0][0][2] = velPicLinFast[KMH][0][2];
	velMatrix[VD_ROAD][KMH][0][1][0] = velPicLinFast[KMH][1][0];
	velMatrix[VD_ROAD][KMH][0][1][1] = velPicLinFast[KMH][1][1];
	velMatrix[VD_ROAD][KMH][0][1][2] = velPicLinFast[KMH][1][2];

	//Plane

	//MPH
	velMatrix[VD_AIR][MPH][0][0][0] = velPicMax[MPH][0][0];
	velMatrix[VD_AIR][MPH][0][0][1] = velPicMax[MPH][0][1];
	velMatrix[VD_AIR][MPH][0][0][2] = velPicMax[MPH][0][2];
	velMatrix[VD_AIR][MPH][0][1][0] = velPicMax[MPH][1][0];
	velMatrix[VD_AIR][MPH][0][1][1] = velPicMax[MPH][1][1];
	velMatrix[VD_AIR][MPH][0][1][2] = velPicMax[MPH][1][2];
	//KMH
	velMatrix[VD_AIR][KMH][0][0][0] = velPicMax[KMH][0][0];
	velMatrix[VD_AIR][KMH][0][0][1] = velPicMax[KMH][0][1];
	velMatrix[VD_AIR][KMH][0][0][2] = velPicMax[KMH][0][2];
	velMatrix[VD_AIR][KMH][0][1][0] = velPicMax[KMH][1][0];
	velMatrix[VD_AIR][KMH][0][1][1] = velPicMax[KMH][1][1];
	velMatrix[VD_AIR][KMH][0][1][2] = velPicMax[KMH][1][2];
	//KNO
	velMatrix[VD_AIR][KNO][0][0][0] = velPicMax[KNO][0][0];
	velMatrix[VD_AIR][KNO][0][0][1] = velPicMax[KNO][0][1];
	velMatrix[VD_AIR][KNO][0][0][2] = velPicMax[KNO][0][2];
	velMatrix[VD_AIR][KNO][0][1][0] = velPicMax[KNO][1][0];
	velMatrix[VD_AIR][KNO][0][1][1] = velPicMax[KNO][1][1];
	velMatrix[VD_AIR][KNO][0][1][2] = velPicMax[KNO][1][2];

	//Train

	//MPH FAST
	velMatrix[VD_RAIL][MPH][1][0][0] = velPicCarSlow[MPH][0][0];
	velMatrix[VD_RAIL][MPH][1][0][1] = velPicCarSlow[MPH][0][1];
	velMatrix[VD_RAIL][MPH][1][0][2] = velPicCarSlow[MPH][0][2];
	velMatrix[VD_RAIL][MPH][1][1][0] = velPicCarSlow[MPH][1][0];
	velMatrix[VD_RAIL][MPH][1][1][1] = velPicCarSlow[MPH][1][1];
	velMatrix[VD_RAIL][MPH][1][1][2] = velPicCarSlow[MPH][1][2];
	//KMH FAST
	velMatrix[VD_RAIL][KMH][1][0][0] = velPicCarSlow[KMH][0][0];
	velMatrix[VD_RAIL][KMH][1][0][1] = velPicCarSlow[KMH][0][1];
	velMatrix[VD_RAIL][KMH][1][0][2] = velPicCarSlow[KMH][0][2];
	velMatrix[VD_RAIL][KMH][1][1][0] = velPicCarSlow[KMH][1][0];
	velMatrix[VD_RAIL][KMH][1][1][1] = velPicCarSlow[KMH][1][1];
	velMatrix[VD_RAIL][KMH][1][1][2] = velPicCarSlow[KMH][1][2];

	//MPH SLOW
	velMatrix[VD_RAIL][MPH][0][0][0] = velPicLinFast[MPH][0][0];
	velMatrix[VD_RAIL][MPH][0][0][1] = velPicLinFast[MPH][0][1];
	velMatrix[VD_RAIL][MPH][0][0][2] = velPicLinFast[MPH][0][2];
	velMatrix[VD_RAIL][MPH][0][1][0] = velPicLinFast[MPH][1][0];
	velMatrix[VD_RAIL][MPH][0][1][1] = velPicLinFast[MPH][1][1];
	velMatrix[VD_RAIL][MPH][0][1][2] = velPicLinFast[MPH][1][2];
	//KMH SLOW
	velMatrix[VD_RAIL][KMH][0][0][0] = velPicLinFast[KMH][0][0];
	velMatrix[VD_RAIL][KMH][0][0][1] = velPicLinFast[KMH][0][1];
	velMatrix[VD_RAIL][KMH][0][0][2] = velPicLinFast[KMH][0][2];
	velMatrix[VD_RAIL][KMH][0][1][0] = velPicLinFast[KMH][1][0];
	velMatrix[VD_RAIL][KMH][0][1][1] = velPicLinFast[KMH][1][1];
	velMatrix[VD_RAIL][KMH][0][1][2] = velPicLinFast[KMH][1][2];
}
void setupTextures(){

	//Shadow
	idShadowMain = create_texture("Shadow\\ShadowMain.png");
	idShadowLeft = create_texture("Shadow\\ShadowLeft.png");
	idShadowAbove = create_texture("Shadow\\ShadowAbove.png");

	//Rpm
	idRpm3 =  create_texture("Rpm\\Rpm3.png");
	idRpm5 =  create_texture("Rpm\\Rpm5.png");
	idRpm6 =  create_texture("Rpm\\Rpm6.png");
	idRpm8 =  create_texture("Rpm\\Rpm8.png");
	idRpm9 =  create_texture("Rpm\\Rpm9.png");
	idRpm10 = create_texture("Rpm\\Rpm10.png");
	idRpm12 = create_texture("Rpm\\Rpm12.png");
	idRpmText = create_texture("Rpm\\RpmText.png");
	idRpmLedOn = create_texture("Rpm\\LedOn.png");
	idRpmLedOff = create_texture("Rpm\\LedOff.png");

	//Velocity Pictures, then referring them
	int idMax[3];		//Textures Max(Mph/Kmh/Kno).png
	int idCarFast[2];	//Textures CarFast(Mph/Kmh).png
	int idCarSlow[2];	//Textures CarSlow(Mph/Kmh).png
	int idLinFast[3];	//Textures LinFast(Mph/Kmh/Kno).png
	int idLinSlow[3];	//Textures LinSlow(Mph/Kmh/Kno).png

	idMax[MPH] = create_texture("Velocity\\MaxMph.png");
	idMax[KMH] = create_texture("Velocity\\MaxKmh.png");
	idMax[KNO] = create_texture("Velocity\\MaxKno.png");
	idCarFast[MPH] = create_texture("Velocity\\CarMphFast.png");
	idCarFast[KMH] = create_texture("Velocity\\CarKmhFast.png");
	idCarSlow[MPH] = create_texture("Velocity\\CarMphSlow.png");
	idCarSlow[KMH] = create_texture("Velocity\\CarKmhSlow.png");
	idLinFast[MPH] = create_texture("Velocity\\LinMphFast.png");
	idLinFast[KMH] = create_texture("Velocity\\LinKmhFast.png");
	idLinFast[KNO] = create_texture("Velocity\\LinKnoFast.png");
	idLinSlow[MPH] = create_texture("Velocity\\LinMphSlow.png");
	idLinSlow[KMH] = create_texture("Velocity\\LinKmhSlow.png");
	idLinSlow[KNO] = create_texture("Velocity\\LinKnoSlow.png");

	// Bicycle
	texMatrix[VD_VELO][MPH][0] = idLinSlow[MPH];
	texMatrix[VD_VELO][KMH][0] = idLinSlow[KMH];
	//Boat
	texMatrix[VD_WATER][MPH][1] = idLinFast[MPH];
	texMatrix[VD_WATER][KMH][1] = idLinFast[KMH];
	texMatrix[VD_WATER][KNO][1] = idLinFast[KNO];
	texMatrix[VD_WATER][MPH][0] = idLinSlow[MPH];
	texMatrix[VD_WATER][KMH][0] = idLinSlow[KMH];
	texMatrix[VD_WATER][KNO][0] = idLinSlow[KNO];
	//Car
	texMatrix[VD_ROAD][MPH][2] = idCarFast[MPH];
	texMatrix[VD_ROAD][KMH][2] = idCarFast[KMH];
	texMatrix[VD_ROAD][KNO][2] = idCarFast[KNO];
	texMatrix[VD_ROAD][MPH][1] = idCarSlow[MPH];
	texMatrix[VD_ROAD][KMH][1] = idCarSlow[KMH];
	texMatrix[VD_ROAD][KNO][1] = idCarSlow[KNO];
	texMatrix[VD_ROAD][MPH][0] = idLinFast[MPH];
	texMatrix[VD_ROAD][KMH][0] = idLinFast[KMH];
	texMatrix[VD_ROAD][KNO][0] = idLinFast[KNO];
	//Plane
	texMatrix[VD_AIR][MPH][0] = idMax[MPH];
	texMatrix[VD_AIR][KMH][0] = idMax[KMH];
	texMatrix[VD_AIR][KNO][0] = idMax[KNO];
	//Train
	texMatrix[VD_RAIL][MPH][1] = idCarSlow[MPH];
	texMatrix[VD_RAIL][KMH][1] = idCarSlow[KMH];
	texMatrix[VD_RAIL][MPH][0] = idLinFast[MPH];
	texMatrix[VD_RAIL][KMH][0] = idLinFast[KMH];
	
	//Unit Text
	idUnit[MPH] = create_texture("Velocity\\TextMph.png");
	idUnit[KMH] = create_texture("Velocity\\TextKmh.png");
	idUnit[KNO] = create_texture("Velocity\\TextKno.png");

	//Gear
	idGear[0] = create_texture("GearNumbers\\GearR.png");
	idGear[1] = create_texture("GearNumbers\\GearN.png");
	for (int x = 1; x < 9; x++){
		idGear[x + 1] = create_texture("GearNumbers\\Gear" + std::to_string(x) + ".png");
	}
	idGearShiftDown = create_texture("GearNumbers\\GearShiftDown.png");
	idGearShiftUp = create_texture("GearNumbers\\GearShiftUp.png");

	//Needles
	idNeedleBigA = create_texture("Needles\\NeedleBigA.png");
	idNeedleBigB = create_texture("Needles\\NeedleBigB.png");
	idNeedleSmallA = create_texture("Needles\\NeedleSmallA.png");
	idNeedleSmallB = create_texture("Needles\\NeedleSmallB.png");

	//Damage
	idDmg = create_texture("Damage.png");

	//Pla VSpeed
	idPlaVert = create_texture("PlaVertical.png");

	//Fuel
	idFuelScale  = create_texture("Fuel\\FuelScale.png");
	idExtFuel[0] = create_texture("Fuel\\FuelLogo.png");
	idExtFuel[1] = create_texture("Fuel\\GearLogo.png");
	idExtFuel[2] = create_texture("Fuel\\EngineLogo.png");
}

//Placement of elemets which are relative to others
void relativePlacement(){

	//Hud Size
	hudSize = exp2f(Settings::hudSizeExp - 1.0f);

	//Size correction, static right bottom corner while changing hudsize
	mainPosX = Settings::mainOffsetX - 0.098f*(hudSize - 1.0f);
	mainPosY = Settings::mainOffsetY - 0.091f*(hudSize - 1.0f) / screenCorrect;

	//Extra gauge
	extraPosX  = mainPosX + hudSize * Settings::hudSizeConst * (381.0f / 1024.0f);
	extraPosY  = mainPosY - hudSize * Settings::hudSizeConst * (496.0f / 1024.0f) * screenCorrect;
	extraPosX2 = mainPosX - hudSize * Settings::hudSizeConst * (557.0f / 1024.0f);
	extraPosY2 = mainPosY + hudSize * Settings::hudSizeConst * (178.0f / 1024.0f) * screenCorrect;

	if (Settings::isExtraGaugeLeft)
	{
		float helpPosX = extraPosX;
		extraPosX = extraPosX2;
		extraPosX2 = helpPosX;
		float helpPosY = extraPosY;
		extraPosY = extraPosY2;
		extraPosY2 = helpPosY;
	}

	//Led RPM
	rpmGap = hudSize * Settings::hudSizeConst * ( 50.0f / 1024.0f);
	rpmLedX = mainPosX + hudSize * Settings::hudSizeConst * (Settings::ledsOffsetXInt*2.0f / 1024.0f) - 4 * rpmGap;
	rpmLedY = mainPosY + hudSize * Settings::hudSizeConst * (Settings::ledsOffsetYInt*2.0f / 1024.0f) * screenCorrect;
}

//Load and Save to INI Methods
void read_ini_extended()
{
	std::string path = GetCurrentModulePath(); // includes trailing slash
	path = path + "LeFixSpeedo\\settingsExtended.ini";

	//Constants

	ledCol[R][0] = ((float)readInt(path, "CONSTANTS", "led1R",   0,   0, 255)) / 255.0f;
	ledCol[G][0] = ((float)readInt(path, "CONSTANTS", "led1G", 255,   0, 255)) / 255.0f;
	ledCol[B][0] = ((float)readInt(path, "CONSTANTS", "led1B",   0,   0, 255)) / 255.0f;
	ledCol[R][1] = ((float)readInt(path, "CONSTANTS", "led2R",   0,   0, 255)) / 255.0f;
	ledCol[G][1] = ((float)readInt(path, "CONSTANTS", "led2G", 255,   0, 255)) / 255.0f;
	ledCol[B][1] = ((float)readInt(path, "CONSTANTS", "led2B",   0,   0, 255)) / 255.0f;
	ledCol[R][2] = ((float)readInt(path, "CONSTANTS", "led3R",   0,   0, 255)) / 255.0f;
	ledCol[G][2] = ((float)readInt(path, "CONSTANTS", "led3G", 255,   0, 255)) / 255.0f;
	ledCol[B][2] = ((float)readInt(path, "CONSTANTS", "led3B",   0,   0, 255)) / 255.0f;
	ledCol[R][3] = ((float)readInt(path, "CONSTANTS", "led4R", 255,   0, 255)) / 255.0f;
	ledCol[G][3] = ((float)readInt(path, "CONSTANTS", "led4G", 255,   0, 255)) / 255.0f;
	ledCol[B][3] = ((float)readInt(path, "CONSTANTS", "led4B",   0,   0, 255)) / 255.0f;
	ledCol[R][4] = ((float)readInt(path, "CONSTANTS", "led5R", 255,   0, 255)) / 255.0f;
	ledCol[G][4] = ((float)readInt(path, "CONSTANTS", "led5G", 255,   0, 255)) / 255.0f;
	ledCol[B][4] = ((float)readInt(path, "CONSTANTS", "led5B",   0,   0, 255)) / 255.0f;
	ledCol[R][5] = ((float)readInt(path, "CONSTANTS", "led6R", 255,   0, 255)) / 255.0f;
	ledCol[G][5] = ((float)readInt(path, "CONSTANTS", "led6G", 255,   0, 255)) / 255.0f;
	ledCol[B][5] = ((float)readInt(path, "CONSTANTS", "led6B",   0,   0, 255)) / 255.0f;
	ledCol[R][6] = ((float)readInt(path, "CONSTANTS", "led7R", 255,   0, 255)) / 255.0f;
	ledCol[G][6] = ((float)readInt(path, "CONSTANTS", "led7G",   0,   0, 255)) / 255.0f;
	ledCol[B][6] = ((float)readInt(path, "CONSTANTS", "led7B",   0,   0, 255)) / 255.0f;
	ledCol[R][7] = ((float)readInt(path, "CONSTANTS", "led8R", 255,   0, 255)) / 255.0f;
	ledCol[G][7] = ((float)readInt(path, "CONSTANTS", "led8G",   0,   0, 255)) / 255.0f;
	ledCol[B][7] = ((float)readInt(path, "CONSTANTS", "led8B",   0,   0, 255)) / 255.0f;
	ledCol[R][8] = ((float)readInt(path, "CONSTANTS", "led9R", 255,   0, 255)) / 255.0f;
	ledCol[G][8] = ((float)readInt(path, "CONSTANTS", "led9G",   0,   0, 255)) / 255.0f;
	ledCol[B][8] = ((float)readInt(path, "CONSTANTS", "led9B",   0,   0, 255)) / 255.0f;

	//PIC MAX MPH
	velPicMax[MPH][0][0] = ((float)readInt(path, "CONSTANTS", "picMaxMphDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicMax[MPH][0][1] = ((float)readInt(path, "CONSTANTS", "picMaxMphDeg1", 3375, -7200, 7200)) / 3600.0f;
	velPicMax[MPH][0][2] = ((float)readInt(path, "CONSTANTS", "picMaxMphDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicMax[MPH][1][0] = ((float)readInt(path, "CONSTANTS", "picMaxMphVel0", 0, 0, 10000));
	velPicMax[MPH][1][1] = ((float)readInt(path, "CONSTANTS", "picMaxMphVel1", 100, 0, 10000));
	velPicMax[MPH][1][2] = ((float)readInt(path, "CONSTANTS", "picMaxMphVel2", 350, 0, 10000));
	//PIC MAX KMH
	velPicMax[KMH][0][0] = ((float)readInt(path, "CONSTANTS", "picMaxKmhDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicMax[KMH][0][1] = ((float)readInt(path, "CONSTANTS", "picMaxKmhDeg1", 3375, -7200, 7200)) / 3600.0f;
	velPicMax[KMH][0][2] = ((float)readInt(path, "CONSTANTS", "picMaxKmhDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicMax[KMH][1][0] = ((float)readInt(path, "CONSTANTS", "picMaxKmhVel0", 0, 0, 10000));
	velPicMax[KMH][1][1] = ((float)readInt(path, "CONSTANTS", "picMaxKmhVel1", 160, 0, 10000));
	velPicMax[KMH][1][2] = ((float)readInt(path, "CONSTANTS", "picMaxKmhVel2", 560, 0, 10000));
	//PIC MAX KNO
	velPicMax[KNO][0][0] = ((float)readInt(path, "CONSTANTS", "picMaxKnoDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicMax[KNO][0][1] = ((float)readInt(path, "CONSTANTS", "picMaxKnoDeg1", 3375, -7200, 7200)) / 3600.0f;
	velPicMax[KNO][0][2] = ((float)readInt(path, "CONSTANTS", "picMaxKnoDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicMax[KNO][1][0] = ((float)readInt(path, "CONSTANTS", "picMaxKnoVel0", 0, 0, 10000));
	velPicMax[KNO][1][1] = ((float)readInt(path, "CONSTANTS", "picMaxKnoVel1", 80, 0, 10000));
	velPicMax[KNO][1][2] = ((float)readInt(path, "CONSTANTS", "picMaxKnoVel2", 330, 0, 10000));
	//PIC ROAD FAST MPH
	velPicCarFast[MPH][0][0] = ((float)readInt(path, "CONSTANTS", "picCarFastMphDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicCarFast[MPH][0][1] = ((float)readInt(path, "CONSTANTS", "picCarFastMphDeg1", 3375, -7200, 7200)) / 3600.0f;
	velPicCarFast[MPH][0][2] = ((float)readInt(path, "CONSTANTS", "picCarFastMphDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicCarFast[MPH][1][0] = ((float)readInt(path, "CONSTANTS", "picCarFastMphVel0", 20, 0, 10000));
	velPicCarFast[MPH][1][1] = ((float)readInt(path, "CONSTANTS", "picCarFastMphVel1", 80, 0, 10000));
	velPicCarFast[MPH][1][2] = ((float)readInt(path, "CONSTANTS", "picCarFastMphVel2", 280, 0, 10000));
	//PIC ROAD FAST KMH		  
	velPicCarFast[KMH][0][0] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicCarFast[KMH][0][1] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhDeg1", 3375, -7200, 7200)) / 3600.0f;
	velPicCarFast[KMH][0][2] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicCarFast[KMH][1][0] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhVel0", 20, 0, 10000));
	velPicCarFast[KMH][1][1] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhVel1", 100, 0, 10000));
	velPicCarFast[KMH][1][2] = ((float)readInt(path, "CONSTANTS", "picCarFastKmhVel2", 400, 0, 10000));
	//PIC ROAD SLOW MPH		  
	velPicCarSlow[MPH][0][0] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicCarSlow[MPH][0][1] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphDeg1", 3825, -7200, 7200)) / 3600.0f;
	velPicCarSlow[MPH][0][2] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicCarSlow[MPH][1][0] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphVel0", 0, 0, 10000));
	velPicCarSlow[MPH][1][1] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphVel1", 90, 0, 10000));
	velPicCarSlow[MPH][1][2] = ((float)readInt(path, "CONSTANTS", "picCarSlowMphVel2", 180, 0, 10000));
	//PIC ROAD SLOW KMH		  
	velPicCarSlow[KMH][0][0] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicCarSlow[KMH][0][1] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhDeg1", 3825, -7200, 7200)) / 3600.0f;
	velPicCarSlow[KMH][0][2] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicCarSlow[KMH][1][0] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhVel0", 0, 0, 10000));
	velPicCarSlow[KMH][1][1] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhVel1", 120, 0, 10000));
	velPicCarSlow[KMH][1][2] = ((float)readInt(path, "CONSTANTS", "picCarSlowKmhVel2", 300, 0, 10000));
	//PIC LIN FAST MPH		  
	velPicLinFast[MPH][0][0] = ((float)readInt(path, "CONSTANTS", "picLinFastMphDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinFast[MPH][0][1] = ((float)readInt(path, "CONSTANTS", "picLinFastMphDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[MPH][0][2] = ((float)readInt(path, "CONSTANTS", "picLinFastMphDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[MPH][1][0] = ((float)readInt(path, "CONSTANTS", "picLinFastMphVel0", 0, 0, 10000));
	velPicLinFast[MPH][1][1] = ((float)readInt(path, "CONSTANTS", "picLinFastMphVel1", 120, 0, 10000));
	velPicLinFast[MPH][1][2] = ((float)readInt(path, "CONSTANTS", "picLinFastMphVel2", 120, 0, 10000));
	//PIC LIN FAST KMH		  
	velPicLinFast[KMH][0][0] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinFast[KMH][0][1] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[KMH][0][2] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[KMH][1][0] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhVel0", 0, 0, 10000));
	velPicLinFast[KMH][1][1] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhVel1", 180, 0, 10000));
	velPicLinFast[KMH][1][2] = ((float)readInt(path, "CONSTANTS", "picLinFastKmhVel2", 180, 0, 10000));
	//PIC LIN FAST KNO		  
	velPicLinFast[KNO][0][0] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinFast[KNO][0][1] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[KNO][0][2] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinFast[KNO][1][0] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoVel0", 0, 0, 10000));
	velPicLinFast[KNO][1][1] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoVel1", 100, 0, 10000));
	velPicLinFast[KNO][1][2] = ((float)readInt(path, "CONSTANTS", "picLinFastKnoVel2", 100, 0, 10000));
	//PIC LIN SLOW MPH		  
	velPicLinSlow[MPH][0][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinSlow[MPH][0][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[MPH][0][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[MPH][1][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphVel0", 0, 0, 10000));
	velPicLinSlow[MPH][1][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphVel1", 60, 0, 10000));
	velPicLinSlow[MPH][1][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowMphVel2", 60, 0, 10000));
	//PIC LIN SLOW KMH		  
	velPicLinSlow[KMH][0][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KMH][0][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KMH][0][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KMH][1][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhVel0", 0, 0, 10000));
	velPicLinSlow[KMH][1][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhVel1", 90, 0, 10000));
	velPicLinSlow[KMH][1][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowKmhVel2", 90, 0, 10000));
	//PIC LIN SLOW KNO		  
	velPicLinSlow[KNO][0][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoDeg0", 2475, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KNO][0][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoDeg1", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KNO][0][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoDeg2", 4500, -7200, 7200)) / 3600.0f;
	velPicLinSlow[KNO][1][0] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoVel0", 0, 0, 10000));
	velPicLinSlow[KNO][1][1] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoVel1", 60, 0, 10000));
	velPicLinSlow[KNO][1][2] = ((float)readInt(path, "CONSTANTS", "picLinSlowKnoVel2", 60, 0, 10000));

	setupVelMatrix();
}

//Draw und Rot Befehle
int levelBack	= -9999;
int levelUnit	= -9997;
int levelBrand	= -9998;
int levelDmg	= -9996;
int levelVel	= -9995;
int levelRpmLed	= -9994;
int levelGear	= -9993;
int levelNeedle = -9992;

void updateRotation(float &currentRot, float aimedRot)
{
	//Realism
	if (aimedRot < currentRot - Settings::needleRealisticDPS*MISC::GET_FRAME_TIME())
	{
		currentRot = currentRot - Settings::needleRealisticDPS*MISC::GET_FRAME_TIME();
	}
	else if (aimedRot > currentRot + Settings::needleRealisticDPS*MISC::GET_FRAME_TIME())
	{
		currentRot = currentRot + Settings::needleRealisticDPS*MISC::GET_FRAME_TIME();
	}
	else
	{
		currentRot = aimedRot;
	}
}

void set_needle_col()
{
	if (isPrimColorValid && Settings::usePrimaryColor)
	{
		for (int x = 0; x < 3; x++)
		{
			needleActCol[x] = &needlePrimCol[x];
		}
	}
	else
	{
		for (int x = 0; x < numRGB; x++)
		{
			needleActCol[x] = &Settings::needleStandardCol[x];
		}
	}
}
void set_car_rpm_rot(){

	//Rotation which shows the actual speed.
	float aimRot;
	float relativeRot;

	// vvv Same Block in Draw Rpm vvv
	if (vehData.rpmMax < 2.6f) {
		relativeRot = vehData.rpm * 2.5f / 3.0f;
	} else if (vehData.rpmMax < 4.6f) {
		relativeRot = vehData.rpm * 4.5f / 5.0f;
	} else if (vehData.rpmMax < 5.2f) {
		relativeRot = vehData.rpm * 5.1f / 6.0f;
	} else if (vehData.rpmMax < 7.1f) {
		relativeRot = vehData.rpm * 7.0f / 8.0f;
	} else if (vehData.rpmMax < 8.1f) {
		relativeRot = vehData.rpm * 8.0f / 9.0f;
	} else if (vehData.rpmMax < 9.1f) {
		relativeRot = vehData.rpm * 9.0f / 10.0f;
	} else {
		relativeRot = vehData.rpm * 11.5f / 12.0f;
	}
	aimRot = 0.75f + 0.295f*relativeRot;
	updateRotation(rpmRot, aimRot);
}
void draw_car_rpm_anlg()
{
	//Passender Drehzahlbereich zeichnen
	int back;

	// vvv Same Block in Set Rpm Rot vvv
	if (vehData.rpmMax < 2.5f) { back = idRpm3; }
	else if (vehData.rpmMax < 4.6f) { back = idRpm5; }
	else if (vehData.rpmMax < 5.6f) { back = idRpm6; }
	else if (vehData.rpmMax < 7.1f) { back = idRpm8; }
	else if (vehData.rpmMax < 8.1f) { back = idRpm9; }
	else if (vehData.rpmMax < 9.1f) { back = idRpm10; }
	else { back = idRpm12; }

	drawTexture(back,      0, levelBack, extraPosX, extraPosY, Settings::alphaFrontMax*aTimeAdjust);
	
	//Text zeichnen
	drawTexture(idRpmText, 0, levelUnit, extraPosX, extraPosY, Settings::alphaFrontMax*aTimeAdjust);
	
	int needle;
	if (Settings::featGear) {
		needle = idNeedleSmallB;
	}
	else {
		needle = idNeedleSmallA;
	}
	drawTexture(needle, 0, levelNeedle, 1.0f, extraPosX, extraPosY, rpmRot, *needleActCol[R], *needleActCol[G], *needleActCol[B], Settings::alphaFrontMax*aTimeAdjust);
	
	//Hintergrund zeichnen
	int shadow;
	if (Settings::isExtraGaugeLeft) {
		shadow = idShadowLeft;
	}
	else {
		shadow = idShadowAbove;
	}
	drawTexture(shadow, 0, levelBack, extraPosX, extraPosY, Settings::alphaBackMax*aTimeAdjust);
}
void draw_car_rpm_led()
{
	float avgRpm = 0.0f;
	if (vehData.rpm > 0.1f)
	{
		avgRpm = 1.2f + 0.5f * vehData.rpm + 2.2f * vehData.rpm * vehData.rpm + 5.2f * vehData.rpm * vehData.rpm * vehData.rpm;
	}

	//Flashing for shifting up
	if (Settings::featGear && (vehData.shiftUp|| vehData.shiftDown))
	{
		if ((GetTickCount() % 200) < 100) avgRpm = 0.0f;
	}

	for (int x = 0; x < 9; x++)
	{
		if (avgRpm > x)
		{
			stopTexture(idRpmLedOff, x);
			drawTexture(idRpmLedOn, x, levelRpmLed, Settings::ledSizeConst, rpmLedX + x*rpmGap, rpmLedY, 0.0f, ledCol[R][x], ledCol[G][x], ledCol[B][x], Settings::alphaFrontMax*aTimeAdjust);
		}
		else
		{
			stopTexture(idRpmLedOn, x);
			drawTexture(idRpmLedOff, x, levelRpmLed, Settings::ledSizeConst, rpmLedX + x*rpmGap, rpmLedY, 0.0f, ledCol[R][x], ledCol[G][x], ledCol[B][x], Settings::alphaFrontMax*aTimeAdjust);
		}
	}
}
void set_pla_vert_rot()
{
	//Rotation which shows the actual speed.
	float aimRot;
	float vspeed = vehData.velocityVertical;
	int sink = 1;
	if (vspeed < 0.0f)
	{
		vspeed = -1.0f*vspeed;
		sink = -1;
	}
	if (vspeed < 1.0f)
	{
		aimRot = 0.895f + vspeed*0.029f * sink;
	}
	else if (vspeed < 100.0f)
	{	
		aimRot = 0.895f + (0.029f + log10f(vspeed) * 0.116f * 0.5f) * sink;
	}
	else
	{
		aimRot = 0.895f + 0.145f * sink;
	}

	//Realism
	if (aimRot < vertRot - MISC::GET_FRAME_TIME())
	{
		vertRot = vertRot - MISC::GET_FRAME_TIME();
	}
	else if (aimRot > vertRot + MISC::GET_FRAME_TIME())
	{
		vertRot = vertRot + MISC::GET_FRAME_TIME();
	}
	else
	{
		vertRot = aimRot;
	}
}
void draw_pla_vert()
{
	//Nadel zeichnen
	drawTexture(idNeedleSmallA, 0, levelNeedle, 1.0f, extraPosX, extraPosY, vertRot, *needleActCol[R], *needleActCol[G], *needleActCol[B], Settings::alphaFrontMax*aTimeAdjust);
	//Skalazeichnen
	drawTexture(idPlaVert, 0, levelVel, extraPosX, extraPosY, Settings::alphaFrontMax*aTimeAdjust);
	//Hintergrund zeichnen
	int shadow;
	if (Settings::isExtraGaugeLeft) {
		shadow = idShadowLeft;
	}
	else {
		shadow = idShadowAbove;
	}
	drawTexture(shadow, 0, levelBack, extraPosX, extraPosY, Settings::alphaBackMax*aTimeAdjust);
}
void set_vel_rot()
{
	eVEHDOMAIN d = vehData.vDomain;

	//vDomain, Unit, isFast, Degree/Velocity, Number
	int fast = getIsFast();
	//Rotation which shows the actual speed.
	float aimRot;

	//Calculate Speed which will be shown in the right unit.
	float realSpeed;
	realSpeed = Settings::speedFactor[d] * unitFactor[Settings::unit[d]] * vehData.velocity;

	//Calculating the roation
	if (realSpeed < velMatrix[d][Settings::unit[d]][fast][1][0])
	{
		//Under minimum
		aimRot = velMatrix[d][Settings::unit[d]][fast][0][0];
	}
	else if (realSpeed < velMatrix[d][Settings::unit[d]][fast][1][1])
	{
		//First segment
		float velPart = (realSpeed - velMatrix[d][Settings::unit[d]][fast][1][0]) /	(velMatrix[d][Settings::unit[d]][fast][1][1] - velMatrix[d][Settings::unit[d]][fast][1][0]);
		aimRot = velMatrix[d][Settings::unit[d]][fast][0][0] + velPart *			(velMatrix[d][Settings::unit[d]][fast][0][1] - velMatrix[d][Settings::unit[d]][fast][0][0]);
	}
	else if (realSpeed < velMatrix[d][Settings::unit[d]][fast][1][2])
	{
		//Second segment
		float velPart = (realSpeed - velMatrix[d][Settings::unit[d]][fast][1][1]) /	(velMatrix[d][Settings::unit[d]][fast][1][2] - velMatrix[d][Settings::unit[d]][fast][1][1]);
		aimRot = velMatrix[d][Settings::unit[d]][fast][0][1] + velPart *				(velMatrix[d][Settings::unit[d]][fast][0][2] - velMatrix[d][Settings::unit[d]][fast][0][1]);
	}
	else
	{
		//Over maximum
		aimRot = velMatrix[d][Settings::unit[d]][fast][0][2];
	}

	updateRotation(velRot, aimRot);
}
void draw_veh_vel(){
	eVEHDOMAIN d = vehData.vDomain;
	//Hintergrund, Damage, Skala, Einheit
	drawTexture(idShadowMain,									0, levelBack, mainPosX, mainPosY, Settings::alphaBackMax *aTimeAdjust);
	drawTexture(texMatrix[d][Settings::unit[d]][getIsFast()],	0, levelVel,  mainPosX, mainPosY, Settings::alphaFrontMax*aTimeAdjust);
	drawTexture(idUnit[Settings::unit[d]],						0, levelUnit, mainPosX, mainPosY, Settings::alphaFrontMax*aTimeAdjust);

	//Nadelbild
	int needle;
	if (Settings::featGear && Settings::featRpm != RPM_ANLG && d == VD_ROAD)
		needle = idNeedleBigB;
	else
		needle = idNeedleBigA;

	//Nadel
	drawTexture(needle, 0, levelNeedle, 1.0f, mainPosX, mainPosY, velRot, *needleActCol[R], *needleActCol[G], *needleActCol[B], Settings::alphaFrontMax*aTimeAdjust);
}
void draw_car_gear(){
	static int prevGear = 0;
	static DWORD lastShiftIndicator = 0;

	float size, posX, posY, centerX, centerY;
	if (Settings::featRpm == RPM_ANLG)
	{
		// RPM Analog (Gear on extra gauge)
		size = 0.35f;
		posX = extraPosX;
		posY = extraPosY;
		centerX = 0.6f;
		centerY = 0.65f;

		// Flash Gear if shifting is necessary (RPM flashes if LED)
		if (vehData.shiftUp || vehData.shiftDown) lastShiftIndicator = GetTickCount();
	}
	else
	{
		// RPM LEDs or no RPM at all (Gear on main gauge)
		size = 0.4f;
		posX = mainPosX;
		posY = mainPosY;
		centerX = 0.5f;
		centerY = 0.5f;
	}

	//Gear change
	if (prevGear != vehData.gear)
	{
		stopTexture(idGear[prevGear + 1], 0);
		prevGear = vehData.gear;
	}

	//If rpm is displayed analogue, and shift up/down indicator is on, the gear will flash
	if (false)//lastShiftIndicator > GetTickCount()-400 && Settings::featRpm == RPM_ANLG && (GetTickCount() % 400) < 220)
	{
		stopTexture(idGear[vehData.gear + 1], 0); //TODO
	}
	else
	{
		float antiRed = 1.0f;
		if (vehData.gearMax == vehData.gear) antiRed = 0.5f;

		drawTexture(idGear[vehData.gear + 1], 0, levelGear, size, centerX, centerY, posX, posY, 0.0f, 1.0f, antiRed, antiRed, Settings::alphaFrontMax*aTimeAdjust);
	}		

	if (vehData.shiftDown)	drawTexture(idGearShiftDown,  0, levelGear, size, centerX, centerY, posX, posY, 0.0f, 1.0f, 1.0f, 1.0f, Settings::alphaFrontMax*aTimeAdjust);
	if (vehData.shiftUp)	drawTexture(idGearShiftUp,    0, levelGear, size, centerX, centerY, posX, posY, 0.0f, 1.0f, 1.0f, 1.0f, Settings::alphaFrontMax*aTimeAdjust);
}
void draw_car_dmg()
{
	drawTexture(idDmg, 0, levelDmg, 1.0f, mainPosX, mainPosY, 0.0f, 1.0f, 0.0f, 0.0f, Settings::alphaFrontMax*aTimeAdjust*aDmg*Settings::dmgIntensityFactor);
}
void draw_car_brand()
{
	static std::string prevBrand = "";
	if (prevBrand != vehData.brand)
	{
		std::string path = "Brands\\" + vehData.brand + ".png";
		idBrandLogo = create_texture(path.c_str());
		prevBrand = vehData.brand;
	}
	drawTexture(idBrandLogo, 0, levelBrand, mainPosX, mainPosY, Settings::alphaBrand*Settings::alphaFrontMax*aTimeAdjust);
}
void set_fuel_rot()
{
	float aimRot;
	if(vehData.isDashboardOn) {
		aimRot = 0.75f + 0.295f*vehData.fuel;
	} else {
		aimRot = 0.75f;
	}
	updateRotation(fuelRot, aimRot);
}
void draw_fuel()
{
	//Set color of symbols and draw
	float extFuelCol[numRGB][3]; //[RGBA][1,2,3]
	for (int ext = 0; ext < 3; ext++) {
		for (int col = 0; col < 3; col++) {
			extFuelCol[col][ext] = 1.0f;

			if (col == G || col == B)
			{
				if (vehData.ext2 < 0.5f)
				{
					extFuelCol[col][ext] = 2 * vehData.ext2;
				}
			}
		}
		drawTexture(idExtFuel[ext], 0, levelBack, 1.0f, extraPosX2, extraPosY2, 0.0f, extFuelCol[R][ext], extFuelCol[G][ext], extFuelCol[B][ext], Settings::alphaFrontMax*aTimeAdjust);
	}

	drawTexture(idFuelScale, 0, levelBack, extraPosX2, extraPosY2, Settings::alphaFrontMax*aTimeAdjust);
	drawTexture(idNeedleSmallA, 1, levelNeedle, 1.0f, extraPosX2, extraPosY2, fuelRot, *needleActCol[R], *needleActCol[G], *needleActCol[B], Settings::alphaFrontMax*aTimeAdjust);

	//Hintergrund zeichnen
	int shadow;
	if (!Settings::isExtraGaugeLeft) {
		shadow = idShadowLeft;
	}
	else {
		shadow = idShadowAbove;
	}
	drawTexture(shadow, 0, levelBack, extraPosX2, extraPosY2, Settings::alphaBackMax*aTimeAdjust);
}

//Alpha Fade
void fadeOut()
{
	if (fade > 0.0f)
	{
		//Fadout out speed 1.0f
		fade -= 2.0f*MISC::GET_FRAME_TIME();
		//Valid Borders
		if (fade < 0.0f)
		{
			fade = 0.0f;
		}
		if (fade > 1.0f)
		{
			fade = 1.0f;
		}
	}
}
void fadeIn()
{
	if (fade < 1.0f)
	{
		//Fade in speed 0.5
		fade += 0.5f*MISC::GET_FRAME_TIME();
		//Valid Borders
		if (fade < 0.0f)
		{
			fade = 0.0f;
		}
		if (fade > 1.0f)
		{
			fade = 1.0f;
		}
	}
}
bool isFadeOut()
{
	// check if vehicle dashboard is on
	//if (!vehData.isDashboardOn) return true;
	// check if enable in vehicle domain
	if (!Settings::isEnabledInVehDomain[vehData.vDomain]) return true;
	// check if player is in any vehicle
	if (!PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE)) return true;
	// check if player ped exists and control is on (e.g. not in a cutscene)
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player)) return true;
	// check for player ped death and player arrest
	if (ENTITY::IS_ENTITY_DEAD(playerPed, false) || PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE)) return true;
	// check if player's using mobilphone
	if (PED::IS_PED_RUNNING_MOBILE_PHONE_TASK(playerPed)) return true;
	// check if vehicle name isn't being displayed
	if (!isInMenu && HUD::IS_HUD_COMPONENT_ACTIVE(eHudComponent::HudComponentVehicleName)) return true;
	// check if player is surfing on the internet (workaround: is vehicle roll disabled and not using mousesteering)
	if (!PAD::IS_CONTROL_ENABLED(2, eControl::ControlVehicleFlyRollLeftRight) && !PAD::IS_CONTROL_PRESSED(2, eControl::ControlVehicleMouseControlOverride)) return true;
	// check for first person view
	if (CAM::GET_FOLLOW_VEHICLE_CAM_VIEW_MODE() == 4 && !Settings::isShownInFpv) return true;
	// check if player is passenger
	if (isPassenger && !Settings::isShownAsPassenger) return true;
	//check if character wheel is shown (button pressed and money shown, avoid dpad bugs)
	if (PAD::IS_CONTROL_PRESSED(2, eControl::ControlCharacterWheel) && HUD::IS_HUD_COMPONENT_ACTIVE(eHudComponent::HudComponentCash)) return true;
	return false;
}
//Alpha Day/Night
void calculateAlphaTime(){
	int hour = CLOCK::GET_CLOCK_HOURS();
	int min = CLOCK::GET_CLOCK_MINUTES();
	float time = hour + min / 60.0f;
	if (time < 4.0f)
	{
		aTimeAdjust = 0.8f;
	}
	else if (time < 8.0f)
	{
		aTimeAdjust = 0.8f + (time - 4.0f) / 4.0f*0.3f;
	}
	else if (time < 18.0f)
	{
		aTimeAdjust = 1.10f;
	}
	else if (time < 22.0f)
	{
		aTimeAdjust = 1.10f - (time - 18.0f) / 4.0f*0.3f;
	}
	else
	{
		aTimeAdjust = 0.8f;
	}
}
//Alpha Damage
void calculateAlphaDmgFade()
{
	static bool isInc = true;

	float aDmgMax = (vehData.damage - 0.2f)*1.0f / 0.8f;
	float aDmgMin = aDmgMax - 0.5f;

	//Difference between Min and Max
	float diff = aDmgMax - aDmgMin;
	if (diff < 0.01f || Settings::dmgFrequency == 0.0f)
	{
		aDmg = aDmgMax;
	}
	else
	{
		//Increase Red
		if (isInc)
		{
			aDmg = aDmg + MISC::GET_FRAME_TIME() * diff * Settings::dmgFrequency;
			//Switch if Max is reached
			isInc = aDmg < aDmgMax;
		}
		//Decrease Red
		else
		{
			aDmg = aDmg - MISC::GET_FRAME_TIME() * diff * Settings::dmgFrequency;
			//Switch if Min is reached
			isInc = aDmg < aDmgMin;
		}
	}
}
//Color Needles
void calculateNeedleColor()
{
	if (vehData.veh == 0 || !ENTITY::DOES_ENTITY_EXIST(vehData.veh))
	{
		return;
	}

	isPrimColorValid = false;
	if (vehData.vClass < 10)
	{
		//RGB values of the primary color of the car as integers [0-255]
		int col[] = { 0, 0, 0 };
		VEHICLE::GET_VEHICLE_CUSTOM_PRIMARY_COLOUR(vehData.veh, col+R, col+G, col+B);
		//Brightest Color
		float max = 0.0f;
		int maxColInd = 0;
		//Color 2.8f multiplier to match the color better (Method returns colors which are too dark)
		for (int x = 0; x < 3; x++)
		{
			needlePrimCol[x] = ((float)col[x]) / 255.0f * 2.8f;
			if (needlePrimCol[x] > max)
			{
				max = needlePrimCol[x];
				maxColInd = x;
			}
		}
		//No values above 1.0f
		if (max > 1.0f)
		{
			for (int x = 0; x < 3; x++)
			{
				needlePrimCol[x] = needlePrimCol[x] / max;
			}
		}

		//Determine luminance
		float luminance = (needlePrimCol[R] + needlePrimCol[G] + needlePrimCol[B]) / 3.0f;

		//Is primary color valid to override the standard needle color?
		if (luminance > 0.25f)
		{
			if( (needlePrimCol[maxColInd] > needlePrimCol[(maxColInd + 1) % 3] * (3.5f - luminance * 2.0f)) ||
				(needlePrimCol[maxColInd] > needlePrimCol[(maxColInd + 2) % 3] * (3.5f - luminance * 2.0f)) )
			{
				isPrimColorValid = true;
			}
		}
	}
}

void onMenuEnter()
{
	isInMenu = true;
}
void onMenuExit()
{
	Settings::SaveGlobal();
	Settings::SaveLocal(Settings::currentSetting);
	isInMenu = false;
}
void initialize() {
	logger.SetFile(GetCurrentModulePath() + "LeFixSpeedo\\LeFixSpeedo.log");
	logger.Clear();
	logger.Write("LeFixSpeedo " + std::string(DISPLAY_VERSION));
	logger.Write("Game version " + eGameVersionToString(getGameVersion()));
	vehData.init();
	std::string path = GetCurrentModulePath(); // includes trailing slash
	path = path + "LeFixSpeedo\\settings";

	//Menu
	menu.SetFiles(path + "Menu.ini");
	menu.RegisterOnMain(std::bind(onMenuEnter));
	menu.RegisterOnExit(std::bind(onMenuExit));
	menu.ReadSettings();
	menu.Initialize();

	Settings::SetFile(path);
	Settings::LoadGlobal();
	Settings::LoadLocal(Settings::currentSetting);

	read_ini_extended();

	setupTextures();

	setupCompatibility();

	logger.Write("initialize() Completed!");
}

void update()
{
	//Referenzen aktualisieren
	player = PLAYER::PLAYER_ID();
	playerPed = PLAYER::PLAYER_PED_ID();
	vehData.update(PED::GET_VEHICLE_PED_IS_USING(playerPed));

	static int rare = 0;
	rare++;
	if (rare == 100)
	{
		rare = 0;

		//Alphakorrektur berechnen (Tag/Nacht)
		calculateAlphaTime();

		//Screen Resolution
		screenCorrect = GRAPHICS::_GET_ASPECT_RATIO(FALSE);
		relativePlacement();
	}

	if (rare % 10 == 0)
	{
			//Needle Color
			if (Settings::usePrimaryColor) calculateNeedleColor();

			// Passenger?
			isPassenger = !(VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehData.veh, -1, false) == playerPed);

			// Enable high speedo for modded cars replaced for slow cras
			checkIsFasterThanExpected();
	}
	
	//Anzeige ein- und ausfaden
	isFadeOut() ? fadeOut() : fadeIn();

	//HUD invisible -> dont draw
	if (fade > 0.0f)
	{
		// Needle Color
		set_needle_col();

		// Velocity
		set_vel_rot();
		draw_veh_vel();

		// Fuel stuff
		if (vehData.hasFuel && Settings::featFuel)
		{
			set_fuel_rot();
			draw_fuel();
		}

		// Damage
		if (Settings::featDmg)
		{
			calculateAlphaDmgFade();
			draw_car_dmg();
		}

		// Brand
		if (Settings::featBrand) draw_car_brand();

		if (vehData.vDomain == VD_ROAD)
		{
			// RPM
			switch (Settings::featRpm) {
			case RPM_NO: break;
			case RPM_ANLG: set_car_rpm_rot(); draw_car_rpm_anlg(); break;
			case RPM_LED:  draw_car_rpm_led(); break;
			}
			// Gear
			if (Settings::featGear) draw_car_gear();
		}
		//Vertical Velocity
		if (vehData.vDomain == VD_AIR  && Settings::featVertical)
		{
			set_pla_vert_rot();
			draw_pla_vert();
		}
	}

}

void updateMenu()
{
	menu.CheckKeys();

	if (menu.CurrentMenu("mainmenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle(std::string(DISPLAY_VERSION) + " by LeFix");

		menu.BoolOption("Mod Enabled", Settings::isActive, { "Enable/Disable the entire mod." });
		menu.MenuOption("Units", "unitsmenu", { "Set the units displayed on the different HUDs." });
		menu.MenuOption("Speedfactor", "speedfactormenu", { "Distort displayed velocity by factors, may be used if displayed velocity 'feels' wrong or while screen capturing." });
		menu.MenuOption("Toggle HUD", "togglemenu", { "Toggle HUD visibility for different type of vehicles." });
		int oldSetting = Settings::currentSetting;
		menu.StringArray("Current Setting", { "A", "B", "C" }, Settings::currentSetting, { "This mod saves three different settings. (However the settings above are 'global' settings.)" });
		if (oldSetting != Settings::currentSetting)
		{
			Settings::SaveLocal(oldSetting);
			Settings::LoadLocal(Settings::currentSetting);
			relativePlacement();
		}
		menu.MenuOption("Features", "featuremenu", { "Toggle multiple optional features." });
		menu.MenuOption("Visual", "visualmenu", { "Color and alpha values." });
		menu.MenuOption("Placement", "placementmenu", { "All settings related to size and screen position." });
		menu.BoolOption("Manual Transmission Support", Settings::iktCompatible, { "Mod by ikt@gta5mods, if both mods enable compatibility mode, they will pass additional vehicle stats." });
	}

	if (menu.CurrentMenu("speedfactormenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Speedfactor");

		menu.FloatOption("Road",	Settings::speedFactor[VD_ROAD],		0.5f, 2.0f, 0.1f, { "Velocity will be multiplied by this factor for all road vehicles." });
		menu.FloatOption("Air",		Settings::speedFactor[VD_AIR],		0.5f, 2.0f, 0.1f, { "Velocity will be multiplied by this factor for all aircrafts." });
		menu.FloatOption("Water",	Settings::speedFactor[VD_WATER],	0.5f, 2.0f, 0.1f, { "Velocity will be multiplied by this factor for all watercrafts." });
		menu.FloatOption("Rail",	Settings::speedFactor[VD_RAIL],		0.5f, 2.0f, 0.1f, { "Velocity will be multiplied by this factor for all trains." });
		menu.FloatOption("Bicycle",	Settings::speedFactor[VD_VELO],		0.5f, 2.0f, 0.1f, { "Velocity will be multiplied by this factor for all bicycles." });
	}

	if (menu.CurrentMenu("unitsmenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Units");

		menu.StringArray("Road",	{"MPH", "KMH", },			Settings::unit[VD_ROAD],	{ "Unit for all road vehicles." });
		menu.StringArray("Air",		{"MPH", "KMH", "KNOTS"},	Settings::unit[VD_AIR],		{ "Unit for all aircrafts." });
		menu.StringArray("Water",	{"MPH", "KMH", "KNOTS"},	Settings::unit[VD_WATER],	{ "Unit for all watercrafts." });
		menu.StringArray("Rail",	{"MPH", "KMH", },			Settings::unit[VD_RAIL],	{ "Unit for all trains." });
		menu.StringArray("Bicycle",	{"MPH", "KMH", },			Settings::unit[VD_VELO],	{ "Unit for all bicycles." });
	}

	if (menu.CurrentMenu("togglemenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Toggle HUDs");

		menu.BoolOption("Road",		Settings::isEnabledInVehDomain[VD_ROAD],	{ "Toggle HUD for all road vehicles." });
		menu.BoolOption("Air",		Settings::isEnabledInVehDomain[VD_AIR],		{ "Toggle HUD for all aircrafts." });
		menu.BoolOption("Water",	Settings::isEnabledInVehDomain[VD_WATER],	{ "Toggle HUD for all watercrafts." });
		menu.BoolOption("Rail",		Settings::isEnabledInVehDomain[VD_RAIL],	{ "Toggle HUD for all trains." });
		menu.BoolOption("Bicycle",	Settings::isEnabledInVehDomain[VD_VELO],	{ "Toggle HUD for all bicycles." });
	}

	if (menu.CurrentMenu("featuremenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Features");

		menu.BoolOption("Gear", Settings::featGear, { "Gear display for all road vehicles, explicitly supports Manual Transmission mod by ikt." });
		menu.StringArray("RPM", { "Off", "Analog", "LEDs" }, Settings::featRpm, { "RPM display either analog or with LEDs.", "Color is defined in ...Extended.ini" });
		menu.BoolOption("Fuel", Settings::featFuel, { "Enables fuel gauge while driving road vehicles with a fuel tank. Intended for 'Simple Fuel' or any other mod change the native fuel level." });
		menu.BoolOption("Vertical Speed", Settings::featVertical, { "Displays vertical velocity in meters per second on a logarithmic scale for all aircrafts." });
		menu.BoolOption("Brand", Settings::featBrand, { "Optional brand logo for all vehicles. Brands are registered using the livery name and real life brands can be added easily in ...Brands.ini" });
		menu.BoolOption("Show Handbrake", Settings::featHBrake, { "Display 'N' on gear display while using handbrake." });
		menu.BoolOption("Show in FPV", Settings::isShownInFpv, { "Keep HUD visible in First Person View." });
		menu.BoolOption("Show as Passenger", Settings::isShownAsPassenger, { "Show HUD while using a vehicle even as passenger." });
		menu.BoolOption("Use high speedo backgrounds", Settings::useHighSpeedoByDefault, { "The default speedo background (maximum velocity) fits for vanilla GTA V but with modified handling it requires high velocity backgrounds by default. However it automatically switches to the next picture if the maximum velocity is reached." });
	}

	if (menu.CurrentMenu("visualmenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Features");

		menu.BoolOption("Use Primary Color", Settings::usePrimaryColor, { "If primary vehicle color is bright enough the needles will change their color." });
		menu.FloatOption("Needle R", Settings::needleStandardCol[R], 0.0f, 1.0f, 0.05, { "Default color of all needles." });
		menu.FloatOption("Needle G", Settings::needleStandardCol[G], 0.0f, 1.0f, 0.05, { "Default color of all needles." });
		menu.FloatOption("Needle B", Settings::needleStandardCol[B], 0.0f, 1.0f, 0.05, { "Default color of all needles." });
		menu.FloatOption("Alpha Foreground", Settings::alphaFrontMax, 0.5f, 1.0f, 0.1f, { "Alpha value for all forgorund textures." });
		menu.FloatOption("Alpha Background", Settings::alphaBackMax,  0.0f, 1.0f, 0.1f, { "Alpha value for all background textures." });
		menu.FloatOption("Alpha Brand", Settings::alphaBrand, 0.5f, 1.0f, 0.1f, { "Alpha value for optional brand logo." });
		menu.FloatOption("Damage Intensity", Settings::dmgIntensityFactor, 0.5f, 1.0f, 0.1f, { "Max alpha Value of damage feature." });
		menu.FloatOption("Damage Frequency", Settings::dmgFrequency, 0.5f, 2.0f, 0.1f, { "Frequency of damage feature." });
	}

	if (menu.CurrentMenu("placementmenu"))
	{
		menu.Title("Speedometer");
		menu.Subtitle("Placement");

		bool b = false;

		b = b || menu.BoolOption("Extra Gauge Left", Settings::isExtraGaugeLeft, { "Choose whether the small gauge is on left or on top of the main gauge." });
		b = b || menu.FloatOption("HUD Size", Settings::hudSizeExp, 0.2f, 1.8f, 0.1f, { "HUD size. Affects every component." });
		b = b || menu.FloatOption("HUD Positon X", Settings::mainOffsetX, 0.0f, 1.0f, 0.005f, { "HUD Positon X:", "0 left", "1 right" });
		b = b || menu.FloatOption("HUD Positon Y", Settings::mainOffsetY, 0.0f, 1.0f, 0.005f, { "HUD Positon X:", "0 top", "1 bottom" });
		b = b || menu.IntOption("LED Offset X", Settings::ledsOffsetXInt, -1000, 1000, 2, { "LEDs offset from Main HUD center." });
		b = b || menu.IntOption("LED Offset Y", Settings::ledsOffsetYInt, -1000, 1000, 2, { "LEDs offset from Main HUD center." });

		if (b) relativePlacement();
	}

	menu.EndMenu();
}

//-------------------------------------------------------------------------------------

//                                    MAIN METHOD                                      

//-------------------------------------------------------------------------------------

void ScriptMain()
{
	initialize();
	while (true) {
		if (Settings::isActive) update();
		updateMenu();
		WAIT(0);
	}
}
