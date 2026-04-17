#include "VehicleExtensions.hpp"
#include "NativeMemory.hpp"
#include "Offsets.hpp"
#include "../Util/Logger.hpp"
#include "../Memory/Versions.h"

VehicleExtensions::VehicleExtensions() {
	mem::init();
}

BYTE *VehicleExtensions::GetAddress(Vehicle handle) {
	return reinterpret_cast<BYTE *>(mem::GetAddressOfEntity(handle));
}

void VehicleExtensions::GetOffsets() {
	uintptr_t addr = 0;
	
	if (!Versions::IsEnhanced()) {
		addr = mem::FindPattern("\x3C\x03\x0F\x85\x00\x00\x00\x00\x48\x8B\x41\x20\x48\x8B\x88",
								"xxxx????xxxxxxx");
		handlingOffset = addr == 0 ? 0 : *(int*)(addr + 0x16);
	}
	else {
		addr = mem::FindPattern("88 90 ? ? ? 00 0F B7 90 ? 00 00 00");
		handlingOffset = addr == 0 ? 0 : (*(int*)(addr + 2)) - 9;
	}
	LOGD(handlingOffset == 0 ? Warning : Debug, "Handling Offset: 0x{:X}", handlingOffset);

	if (!Versions::IsEnhanced()) {
		addr = mem::FindPattern("\x74\x26\x0F\x57\xC9", "xxxxx");
		fuelLevelOffset = addr == 0 ? 0 : *(int*)(addr + 8);
	}
	else {
		addr = mem::FindPattern(
			"83 E0 07 "
			"66 83 F8 03 "
			"74 ?? "
			"F3 0F 10 86 ?? ?? 00 00"
		);
		fuelLevelOffset = addr == 0 ? 0 : *(int*)(addr + 13);
	}
	LOGD(fuelLevelOffset == 0 ? Warning : Debug, "Fuel Level Offset: 0x{:X}", fuelLevelOffset);

	if (!Versions::IsEnhanced()) {
		addr = mem::FindPattern("\x76\x03\x0F\x28\xF0\xF3\x44\x0F\x10\x93", "xxxxxxxxxx");
		rpmOffset = addr == 0 ? 0 : *(int*)(addr + 10);
		throttleOffset = addr == 0 ? 0 : rpmOffset + 0x10;
	}
	else {
		addr = mem::FindPattern(
			"0F B7 8E ?? ?? 00 00 "
			"66 3B 8E ?? ?? 00 00 "
			"74 ??"
		);
		auto nextGearOffset = addr == 0 ? 0 : *(int*)(addr + 3);
		auto gearRatiosOffset = addr == 0 ? 0 : nextGearOffset + 12;
		auto driveForceOffset = gearRatiosOffset == 0 ? 0 : gearRatiosOffset + (11 * sizeof(float));
		auto driveMaxFlatVelOffset = driveForceOffset == 0 ? 0 : driveForceOffset + 0x08;
		rpmOffset = driveMaxFlatVelOffset == 0 ? 0 : driveMaxFlatVelOffset + sizeof(float) * 2;
		throttleOffset = driveMaxFlatVelOffset == 0 ? 0 : driveMaxFlatVelOffset + sizeof(float) * 6;
	}
	LOGD(rpmOffset == 0 ? Warning : Debug, "RPM Offset: 0x{:X}", rpmOffset);

	if (!Versions::IsEnhanced()) {
		addr = mem::FindPattern("\x48\x8D\x8F\x00\x00\x00\x00\x4C\x8B\xC3\xF3\x0F\x11\x7C\x24",
								"xxx????xxxxxxxx");
		auto nextGearOffset = addr == 0 ? 0 : *(int*)(addr + 3);
		currentGearOffset = addr == 0 ? 0 : nextGearOffset + 2;
		topGearOffset = addr == 0 ? 0 : nextGearOffset + 6;
	}
	else {
		addr = mem::FindPattern(
			"0F B7 8E ?? ?? 00 00 "
			"66 3B 8E ?? ?? 00 00 "
			"74 ??"
		);
		auto nextGearOffset = addr == 0 ? 0 : *(int*)(addr + 3);
		currentGearOffset = addr == 0 ? 0 : nextGearOffset + 2;
		topGearOffset = addr == 0 ? 0 : nextGearOffset + 6;
	}
	LOGD(currentGearOffset == 0 ? Warning : Debug, "Current Gear Offset: 0x{:X}", currentGearOffset);
	LOGD(topGearOffset == 0 ? Warning : Debug, "Top Gear Offset: 0x{:X}", topGearOffset);

	if (!Versions::IsEnhanced()) {
		if (gameVersion >= Versions::L_1_0_2060_0_STEAM) {
			addr = mem::FindPattern("8A C2 24 01 C0 E0 04 08 81");
			handbrakeOffset = addr == 0 ? 0 : *(int*)(addr + 19);
		}
		else {
			addr = mem::FindPattern("\x44\x88\xA3\x00\x00\x00\x00\x45\x8A\xF4", "xxx????xxx");
			handbrakeOffset = addr == 0 ? 0 : *(int*)(addr + 3);
		}
	}
	else {
		addr = mem::FindPattern(
			"F3 0F 11 BE ?? ?? 00 00 "
			"48 8B 86 ?? ?? 00 00 "
			"F3 0F 59 B8 ?? 00 00 00 "
			"48 89 DA"
		);
		auto steeringAngleInputOffset = addr == 0 ? 0 : *(int*)(addr + 4);
		auto steeringAngleOffset = addr == 0 ? 0 : steeringAngleInputOffset + sizeof(float) * 2;
		handbrakeOffset = steeringAngleOffset == 0 ? 0 : steeringAngleOffset + sizeof(float) * 4;
	}
	LOGD(handbrakeOffset == 0 ? Warning : Debug, "Handbrake Offset: 0x{:X}", handbrakeOffset);
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
