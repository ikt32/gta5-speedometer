#include "VehicleExtensions.hpp"
#include "NativeMemory.hpp"
#include "Offsets.hpp"
#include "../Util/Logger.hpp"
#include "../Util/Versions.h"

VehicleExtensions::VehicleExtensions() {
	mem::init(getGameVersion());
}

BYTE *VehicleExtensions::GetAddress(Vehicle handle) {
	return reinterpret_cast<BYTE *>(mem::GetAddressOfEntity(handle));
}

void VehicleExtensions::GetOffsets() {
	auto addr = mem::FindPattern("\x3C\x03\x0F\x85\x00\x00\x00\x00\x48\x8B\x41\x20\x48\x8B\x88",
		"xxxx????xxxxxxx");
	handlingOffset = addr == 0 ? 0 : *(int*)(addr + 0x16);
	logger.Writef("Handling Offset: 0x%X", handlingOffset);

	addr = mem::FindPattern("\x76\x03\x0F\x28\xF0\xF3\x44\x0F\x10\x93",
		"xxxxxxxxxx");
	rpmOffset = addr == 0 ? 0 : *(int*)(addr + 10);
	logger.Writef("RPM Offset: 0x%X", rpmOffset);
	
	addr = mem::FindPattern("\x74\x26\x0F\x57\xC9", "xxxxx");
	fuelLevelOffset = addr == 0 ? 0 : *(int*)(addr + 8);
	logger.Writef("Fuel Level Offset: 0x%X", fuelLevelOffset);
	
	addr = mem::FindPattern("\x48\x8D\x8F\x00\x00\x00\x00\x4C\x8B\xC3\xF3\x0F\x11\x7C\x24",
				"xxx????xxxxxxxx");
	currentGearOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 2;
	logger.Writef("Current Gear Offset: 0x%X", currentGearOffset);
	
	topGearOffset = addr == 0 ? 0 : *(int*)(addr + 3) + 6;
	logger.Writef("Top Gear Offset: 0x%X", topGearOffset);
	
	addr = mem::FindPattern("\x76\x03\x0F\x28\xF0\xF3\x44\x0F\x10\x93",
		"xxxxxxxxxx"); 
	throttleOffset = addr == 0 ? 0 : *(int*)(addr + 10) + 0x10;
	logger.Writef("Throttle Offset: 0x%X", throttleOffset);

	if (getGameVersion() >= 59) {
		addr = mem::FindPattern("\x8A\xC2\x24\x01\xC0\xE0\x04\x08\x81", "xxxxxxxxx");
		handbrakeOffset = addr == 0 ? 0 : *(int*)(addr + 19);
	}
	else {
		addr = mem::FindPattern("\x44\x88\xA3\x00\x00\x00\x00\x45\x8A\xF4", "xxx????xxx");
		handbrakeOffset = addr == 0 ? 0 : *(int*)(addr + 3);
	}
	logger.Writef("Handbrake Offset: 0x%X", handbrakeOffset);
}

float VehicleExtensions::GetCurrentRPM(Vehicle handle) {
	auto address = GetAddress(handle);
	
	if (address == nullptr || rpmOffset == 0)
		return 0.0f;
	
	return *reinterpret_cast<const float *>(address + rpmOffset);
}

float VehicleExtensions::GetFuelLevel(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || fuelLevelOffset == 0)
		return 0.0f;

	return *reinterpret_cast<float*>(address + fuelLevelOffset);
}

uint64_t VehicleExtensions::GetHandlingPtr(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || handlingOffset == 0)
		return 0;
	
	return *reinterpret_cast<uint64_t*>(address + handlingOffset);
}

float VehicleExtensions::GetPetrolTankVolume(Vehicle handle) {
	auto address = GetHandlingPtr(handle);

	if (address == 0)
		return 0.0f;
	
	return *reinterpret_cast<float *>(address + hOffsets.fPetrolTankVolume);
}

uint16_t VehicleExtensions::GetGearCurr(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || currentGearOffset == 0)
		return 0;
	
	return *reinterpret_cast<uint16_t*>(address + currentGearOffset);
}

float VehicleExtensions::GetThrottle(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || throttleOffset == 0)
		return 0.0f;
	
	return *reinterpret_cast<float*>(address + throttleOffset);
}

uint8_t VehicleExtensions::GetTopGear(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || topGearOffset == 0)
		return 0;
	
	return *reinterpret_cast<uint8_t*>(address + topGearOffset);
}

bool VehicleExtensions::GetHandbrake(Vehicle handle) {
	auto address = GetAddress(handle);

	if (address == nullptr || handbrakeOffset == 0)
		return false;
	
	return *reinterpret_cast<bool*>(address + handbrakeOffset);
}
