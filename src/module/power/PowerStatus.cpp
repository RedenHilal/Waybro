#include "PowerStatus.h"
#include "sdbus-c++/sdbus-c++.h"

#include <map>
#include <vector>
#include <string>

PowerStatus::PowerStatus(std::unique_ptr<sdbus::IConnection> connection)
		: conn(std::move(connection)) {}

void PowerStatus::setUp()
{
	using Properties = std::map<std::string, sdbus::Variant>;
	sdbus::ServiceName service{"org.freedesktop.UPower"};
	sdbus::ObjectPath battObject{"/org/freedesktop/UPower/devices/battery_BAT0"};
	sdbus::ObjectPath acObject{"/org/freedesktop/UPower/devices/line_power_AC0"};

	acProxy = sdbus::createProxy(*conn, service, std::move(acObject));
	battProxy = sdbus::createProxy(*conn, std::move(service), std::move(battObject));

	acProxy->getPropertyAsync("Online")
			.onInterface("org.freedesktop.UPower.Device")
			.uponReplyInvoke([this](std::optional<sdbus::Error> err,
									const sdbus::Variant value)
		{
			this->isCharging = value.get<bool>();
		});

	battProxy->getAllPropertiesAsync()
			.onInterface("org.freedesktop.UPower.Device")
			.uponReplyInvoke([this](std::optional<sdbus::Error> err,
									std::map<sdbus::PropertyName, sdbus::Variant> properties)
		{
			this->percentage = properties
				.find(sdbus::PropertyName{"Percentage"})->second.get<double>();
			this->timeToFull = properties
				.find(sdbus::PropertyName{"TimeToFull"})->second.get<int64_t>();
			this->timeToEmpty = properties
				.find(sdbus::PropertyName{"TimeToEmpty"})->second.get<int64_t>();
		});

	acProxy->uponSignal("PropertiesChanged")
			.onInterface("org.freedesktop.DBus.Properties")
			.call([this](std::string ifaceName, Properties properties,
									std::vector<std::string>)
		{
			if (ifaceName != "org.freedesktop.UPower.Device") {
				return;
			}

			const auto& it = properties.find("Online");
			if (it != properties.end()) {
				this->isCharging = it->second.get<bool>();
				this->isUpdated = true;
			}

		});

	battProxy->uponSignal("PropertiesChanged")
			.onInterface("org.freedesktop.DBus.Properties")
			.call([this](std::string ifaceName, Properties properties,
									std::vector<std::string>)
		{
			if (ifaceName != "org.freedesktop.UPower.Device") {
				return;
			}
			
			for (const auto [key, prop] : properties) {
				if (key == "Percentage") {
					this->percentage = prop.get<double>();
				} else if (key == "TimeToFull") {
					this->timeToFull = prop.get<int64_t>();
				} else if (key == "TimeToEmpty") {
					this->timeToEmpty = prop.get<int64_t>();
				} else {
					continue;
				}

				this->isUpdated = true;
			}
		});

}

bool PowerStatus::shouldUpdate()
{
	if (isUpdated) {
		isUpdated = false;
		return true;
	}
	
	return false;
}

void PowerStatus::processEvents()
{
	while (conn->processPendingEvent());
}

int PowerStatus::getFd()
{
	return conn->getEventLoopPollData().fd;
}
