#ifndef WBRO_POWER_MODULE
#define WBRO_POWER_MODULE

#include <memory>
#include <array>
#include <sdbus-c++/sdbus-c++.h>
#include <cstdint>

class PowerStatus {
	std::unique_ptr<sdbus::IConnection> conn;
	std::unique_ptr<sdbus::IProxy> battProxy;
	std::unique_ptr<sdbus::IProxy> acProxy;

public:
	PowerStatus(std::unique_ptr<sdbus::IConnection> conn);
	
	bool isCharging;

	double percentage;

	int64_t timeToFull;

	int64_t timeToEmpty;

	std::array<char, 128> text;



	[[nodiscard]] int getFd();

	void setUp();

	void processEvents();

	[[nodiscard]] bool shouldUpdate();

private:
	bool isUpdated = false;

};

#endif
