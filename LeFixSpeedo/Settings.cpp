#include "Settings.h"

#include <simpleini/SimpleIni.h>

std::string Settings::_filepath;

////CONSTANT////
const float Settings::hudSizeConst = 0.2f;
const float Settings::ledSizeConst = 0.07f;
const float Settings::needleRealisticDPS = 1.57f; //Radians per second

//// NOT CONSTANT ////

// General
bool Settings::isActive = true;
int Settings::currentSetting = 0;
bool Settings::iktCompatible = false;
bool Settings::isDebugMode = false;

// Unit
int Settings::unit[numVEHDOMAIN] = { KMH, KMH, KMH, KNO, KMH };

// Visual
float Settings::alphaFrontMax = 1.0f;
float Settings::alphaBackMax = 0.5f;
bool Settings::usePrimaryColor = true;
float Settings::needleStandardCol[numRGB] = { 1.0f, 0.1f, 0.1f };
float Settings::alphaBrand = 0.5f;

float Settings::dmgIntensityFactor = 0.5f;
float Settings::dmgFrequency = 0.5f;

// Features
int Settings::featRpm = RPM_ANLG; 
bool Settings::featGear = true;
bool Settings::featDmg = true;
bool Settings::featVertical = true;
bool Settings::featBrand = true;
bool Settings::featHBrake = true;
bool Settings::featFuel = true;

bool Settings::isShownInFpv = true;
bool Settings::isShownAsPassenger = true;
bool Settings::useHighSpeedoByDefault = true;

// Toggle
bool Settings::isEnabledInVehDomain[numVEHDOMAIN] = { true, true, true, true, true };

// Speedfactor
float Settings::speedFactor[numVEHDOMAIN] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

// Placement
bool Settings::isExtraGaugeLeft = true;
int Settings::ledsOffsetXInt = 0;
int Settings::ledsOffsetYInt = 70;
float Settings::hudSizeExp = 1.0f;
float Settings::mainOffsetX = 0.9f;
float Settings::mainOffsetY = 0.92f;

void Settings::SetFile(const std::string &filepath) {
	_filepath = filepath;
}

void Settings::SaveGlobal() {
	std::string path = _filepath + "General.ini";
	//Open File
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(path.c_str());

	// General
	settings.SetBoolValue("GENERAL", "isActive", isActive);
	settings.SetLongValue("GENERAL", "currentSetting", currentSetting);
	settings.SetBoolValue("GENERAL", "manualTransmissionSupport", iktCompatible);
	settings.SetBoolValue("GENERAL", "debugMode", isDebugMode);

	// Speedfactor
	settings.SetDoubleValue("SPEEDFACTOR", "road", speedFactor[VD_ROAD]);
	settings.SetDoubleValue("SPEEDFACTOR", "air", speedFactor[VD_AIR]);
	settings.SetDoubleValue("SPEEDFACTOR", "velo", speedFactor[VD_VELO]);
	settings.SetDoubleValue("SPEEDFACTOR", "rail", speedFactor[VD_RAIL]);
	settings.SetDoubleValue("SPEEDFACTOR", "water", speedFactor[VD_WATER]);

	// Units
	settings.SetLongValue("UNIT", "road", unit[VD_ROAD]);
	settings.SetLongValue("UNIT", "air", unit[VD_AIR]);
	settings.SetLongValue("UNIT", "velo", unit[VD_VELO]);
	settings.SetLongValue("UNIT", "rail", unit[VD_RAIL]);
	settings.SetLongValue("UNIT", "water", unit[VD_WATER]);

	// Toggle
	settings.SetBoolValue("TOGGLE", "road", isEnabledInVehDomain[VD_ROAD]);
	settings.SetBoolValue("TOGGLE", "air", isEnabledInVehDomain[VD_AIR]);
	settings.SetBoolValue("TOGGLE", "water", isEnabledInVehDomain[VD_WATER]);
	settings.SetBoolValue("TOGGLE", "velo", isEnabledInVehDomain[VD_VELO]);
	settings.SetBoolValue("TOGGLE", "rail", isEnabledInVehDomain[VD_RAIL]);

	//Write to File
	settings.SaveFile(path.c_str());
}

void Settings::SaveLocal(int x) {
	std::string path = _filepath + std::to_string(x) + ".ini";
	//Open Mode File
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(path.c_str());

	// Visual
	settings.SetDoubleValue("VISUAL", "alphaFront", alphaFrontMax);
	settings.SetDoubleValue("VISUAL", "alphaBack", alphaBackMax);
	settings.SetBoolValue("VISUAL", "usePrimaryColor", usePrimaryColor);
	settings.SetDoubleValue("VISUAL", "needleStandardR", needleStandardCol[R]);
	settings.SetDoubleValue("VISUAL", "needleStandardG", needleStandardCol[G]);
	settings.SetDoubleValue("VISUAL", "needleStandardB", needleStandardCol[B]);
	settings.SetDoubleValue("VISUAL", "alphaBrand", alphaBrand);
	settings.SetDoubleValue("VISUAL", "dmgIntensity", dmgIntensityFactor);
	settings.SetDoubleValue("VISUAL", "dmgFrequency", dmgFrequency);

	// Placement
	settings.SetBoolValue("PLACEMENT", "isSecondGaugeLeft", isExtraGaugeLeft);
	settings.SetLongValue("PLACEMENT", "ledsOffsetX", ledsOffsetXInt);
	settings.SetLongValue("PLACEMENT", "ledsOffsetY", ledsOffsetYInt);
	settings.SetDoubleValue("PLACEMENT", "hudSize", hudSizeExp);
	settings.SetDoubleValue("PLACEMENT", "hudOffsetX", mainOffsetX);
	settings.SetDoubleValue("PLACEMENT", "hudOffsetY", mainOffsetY);

	//Features
	settings.SetLongValue("FEATURES", "rpm", featRpm);
	settings.SetBoolValue("FEATURES", "gear", featGear);
	settings.SetBoolValue("FEATURES", "dmg", featDmg);
	settings.SetBoolValue("FEATURES", "vertical", featVertical);
	settings.SetBoolValue("FEATURES", "brand", featBrand);
	settings.SetBoolValue("FEATURES", "hbrake", featHBrake);
	settings.SetBoolValue("FEATURES", "fuel", featFuel);

	settings.SetBoolValue("FEATURES", "fpv", isShownInFpv);
	settings.SetBoolValue("FEATURES", "passenger", isShownAsPassenger);
	settings.SetBoolValue("FEATURES", "highspeedo", useHighSpeedoByDefault);

	//Write to Mode File
	settings.SaveFile(path.c_str());
}

void Settings::LoadGlobal() {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...

	std::string path = _filepath + "General.ini";
	//Open File
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(path.c_str());

	// General
	isActive = settings.GetBoolValue("GENERAL", "isActive", true);
	currentSetting = settings.GetLongValue("GENERAL", "currentSetting", 0);
	iktCompatible = settings.GetBoolValue("GENERAL", "manualTransmissionSupport", false);
	isDebugMode = settings.GetBoolValue("GENERAL", "debugMode", false);

	// Speedfactor
	speedFactor[VD_ROAD] = settings.GetDoubleValue("SPEEDFACTOR", "road", 1.0);
	speedFactor[VD_AIR] = settings.GetDoubleValue("SPEEDFACTOR", "air", 1.0);
	speedFactor[VD_VELO] = settings.GetDoubleValue("SPEEDFACTOR", "velo", 1.0);
	speedFactor[VD_RAIL] = settings.GetDoubleValue("SPEEDFACTOR", "rail", 1.0);
	speedFactor[VD_WATER] = settings.GetDoubleValue("SPEEDFACTOR", "water", 1.0);

	// Units
	unit[VD_ROAD] = settings.GetLongValue("UNIT", "road", KMH);
	unit[VD_AIR] = settings.GetLongValue("UNIT", "air", KMH);
	unit[VD_VELO] = settings.GetLongValue("UNIT", "velo", KMH);
	unit[VD_RAIL] = settings.GetLongValue("UNIT", "rail", KMH);
	unit[VD_WATER] = settings.GetLongValue("UNIT", "water", KNO);

	// Toggle
	isEnabledInVehDomain[VD_ROAD] = settings.GetBoolValue("TOGGLE", "road", true);
	isEnabledInVehDomain[VD_AIR] = settings.GetBoolValue("TOGGLE", "air", true);
	isEnabledInVehDomain[VD_WATER] = settings.GetBoolValue("TOGGLE", "water", true);
	isEnabledInVehDomain[VD_VELO] = settings.GetBoolValue("TOGGLE", "velo", true);
	isEnabledInVehDomain[VD_RAIL] = settings.GetBoolValue("TOGGLE", "rail", true);
}

void Settings::LoadLocal(int x) {
#pragma warning(push)
#pragma warning(disable: 4244) // Make everything doubles later...

	std::string path = _filepath + std::to_string(x) + ".ini";
	//Open Mode File
	CSimpleIniA settings;
	settings.SetUnicode();
	settings.LoadFile(path.c_str());

	// Visual
	alphaFrontMax = settings.GetDoubleValue("VISUAL", "alphaFront", 1.0);
	alphaBackMax = settings.GetDoubleValue("VISUAL", "alphaBack", 0.3);
	usePrimaryColor = settings.GetBoolValue("VISUAL", "usePrimaryColor", true);
	needleStandardCol[R] = settings.GetDoubleValue("VISUAL", "needleStandardR", 1.0);
	needleStandardCol[G] = settings.GetDoubleValue("VISUAL", "needleStandardG", 0.1);
	needleStandardCol[B] = settings.GetDoubleValue("VISUAL", "needleStandardB", 0.1);
	alphaBrand = settings.GetDoubleValue("VISUAL", "alphaBrand", 0.5);
	dmgIntensityFactor = settings.GetDoubleValue("VISUAL", "dmgIntensity", 1.0);
	dmgFrequency = settings.GetDoubleValue("VISUAL", "dmgFrequency", 1.0);

	// Placement
	isExtraGaugeLeft = settings.GetBoolValue("PLACEMENT", "isSecondGaugeLeft", true);
	ledsOffsetXInt = settings.GetLongValue("PLACEMENT", "ledsOffsetX", 0);
	ledsOffsetYInt = settings.GetLongValue("PLACEMENT", "ledsOffsetY", 70);
	hudSizeExp = settings.GetDoubleValue("PLACEMENT", "hudSize", 1.0);
	mainOffsetX = settings.GetDoubleValue("PLACEMENT", "hudOffsetX", 0.9);
	mainOffsetY = settings.GetDoubleValue("PLACEMENT", "hudOffsetY", 0.92);

	//Features
	featRpm = settings.GetLongValue("FEATURES", "rpm", RPM_ANLG);
	featGear = settings.GetBoolValue("FEATURES", "gear", true);
	featDmg = settings.GetBoolValue("FEATURES", "dmg", true);
	featVertical = settings.GetBoolValue("FEATURES", "vertical", true);
	featBrand = settings.GetBoolValue("FEATURES", "brand", true);
	featHBrake = settings.GetBoolValue("FEATURES", "hbrake", false);
	featFuel = settings.GetBoolValue("FEATURES", "fuel", true);

	isShownInFpv = settings.GetBoolValue("FEATURES", "fpv", false);
	isShownAsPassenger = settings.GetBoolValue("FEATURES", "passenger", false);
	useHighSpeedoByDefault = settings.GetBoolValue("FEATURES", "highspeedo", false);
}