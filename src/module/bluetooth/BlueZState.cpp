#include "BlueZState.hpp"

#include <vector>
#include <iostream>

BlueZState::BlueZState(std::unique_ptr<sdbus::IConnection> conn) :
		conn_(std::move(conn)) {}

void BlueZState::setUp() {
	sdbus::ServiceName service{"org.bluez"};
	sdbus::ObjectPath rootPath{"/"};
	sdbus::ObjectPath managerPath{"/org/bluez/hci0"};

	rootProxy_ = sdbus::createProxy(*conn_, service, std::move(rootPath));
	managerProxy_ = sdbus::createProxy(*conn_, std::move(service),
					std::move(managerPath));
}

int BlueZState::getFd() {
	return conn_->getEventLoopPollData().fd;
}

void BlueZState::getManagedObjects() {
	constexpr std::string_view targetStr = "/org/bluez/hci0";
	int targetSize = targetStr.size();

	ObjectMap reply;
	rootProxy_->callMethod("GetManagedObjects")
			.onInterface("org.freedesktop.DBus.ObjectManager")
			.storeResultsTo(reply);

	for (const auto& [key, val] : reply) {
		if (key == targetStr) {
			processAdapterInterfaces(val);
		} else if (isBaseDevice(key)) {
			processDeviceInterfaces(val, key);
		}
	}

}

void BlueZState::subSignals() {
	this->subManagerSignal();
	this->subRootSignal();
}

void BlueZState::processEvent() {
	while (conn_->processPendingEvent());
}

[[nodiscard]] bool BlueZState::isStateChanged() {
	return shouldUpdate_ = false;
}

[[nodiscard]] const std::string& BlueZState::getAlias() {
	return alias_;
}

[[nodiscard]] const std::string& BlueZState::getAddress() {
	return address_;
}

[[nodiscard]] const std::list<BlueZState::BlueZDevice>& BlueZState::getDevices() {
	return devices_;
}

[[nodiscard]] int BlueZState::getConnectedCount() {
	int count = 0;
	for (const auto& dev : devices_) {
		if (dev.connected) {
			count++;
		}
	}
	return count;
}

[[nodiscard]] bool BlueZState::isPowered() {
	return powered_;
} 

sdbus::Variant BlueZState::getDeviceProperty(const std::string& path, const std::string& prop) {
	sdbus::ServiceName service{"org.bluez"};
	sdbus::ObjectPath devicePath{std::move(prop)};
	auto proxy = sdbus::createProxy(*conn_, service, devicePath);

	return proxy->getProperty(prop).onInterface("org.bluez.Device1");
}


void BlueZState::getAdapterProperties(const PropertyMap& properties) {
	for (auto it = properties.begin(); it != properties.end(); it++) {
		std::cout << it->first << std::endl;
		if (it->first == "Alias") {
			this->alias_ = it->second.get<std::string>();
		} else if (it->first == "Address") {
			this->address_ = it->second.get<std::string>();
		} else if (it->first == "Powered") {
			this->powered_ = it->second.get<bool>();
		}
	}
}

void BlueZState::getDeviceProperties(const PropertyMap& properties, BlueZState::BlueZDevice& dev) {

	for (auto it = properties.begin(); it != properties.end(); it++) {
			std::cout << it->first << std::endl;
		if (it->first == "Alias") {
			dev.alias = it->second.get<std::string>();
		} else if (it->first == "Address") {
			dev.address = it->second.get<std::string>();
		} else if (it->first == "Connected") {
			dev.connected = it->second.get<bool>();
		}
	}
	
}

void BlueZState::processAdapterInterfaces(const InterfaceMap& interfaces) {
	for (const auto& [interfaceKey, props] : interfaces) {
		if (interfaceKey == "org.bluez.Adapter1") {
			getAdapterProperties(props);
		}
	}
}

void BlueZState::processDeviceInterfaces(const InterfaceMap& interfaces, const std::string& path) {
	for (const auto& [interfaceKey, props] : interfaces) {
		if (interfaceKey != "org.bluez.Device1") {
			continue;
		}

		BlueZState::BlueZDevice dev;
		getDeviceProperties(props, dev);

		dev.path = std::move(path);
		devices_.push_back(std::move(dev));

		auto& ldev = devices_.back();
		subDeviceSignal(ldev);
	}
}

void BlueZState::subDeviceSignal(BlueZState::BlueZDevice& dev) {
	sdbus::ServiceName service{"org.bluez"};
	sdbus::ObjectPath object{dev.path};

	dev.proxy = sdbus::createProxy(*conn_, service, object);
	/* sa{sv}as */
	dev.proxy->uponSignal("PropertiesChanged")
			.onInterface("org.freedesktop.DBus.Properties")
			.call([&dev, this](std::string& interface, std::map<std::string, sdbus::Variant> props, std::vector<std::string> strings){
		if (interface != "org.bluez.Device1") {
			return;
		}
		
		for (const auto& [key, val] : props) {
			if (key == "Connected") {
				dev.connected = val.get<bool>();
				shouldUpdate_ = true;
			}
		}
	});
}

void BlueZState::subRootSignal() {
	using Properties = std::map<std::string, sdbus::Variant>;
	using Interfaces = std::map<std::string, Properties>;

	/* oa{sa{sv}} */
	rootProxy_->uponSignal("InterfacesAdded")
			.onInterface("org.freedesktop.DBus.ObjectManager")
			.call([this](const sdbus::ObjectPath& object, const Interfaces& interfaces) {
		if (!this->isBaseDevice(object)) {
			return;
		}

		const auto& devIface = interfaces.find("org.bluez.Device1");

		if (devIface == interfaces.end()) {
			return;
		}

		struct BlueZDevice dev = {
			.path = std::move(object),
			.alias = devIface->second.find("Alias")->second.get<std::string>(),
			.address = devIface->second.find("Address")->second.get<std::string>(),
			.connected = devIface->second.find("Connected")->second.get<bool>()
		};

		devices_.push_back(std::move(dev));
		subDeviceSignal(devices_.back());
		shouldUpdate_ = true;

	});
	/* oas */
	rootProxy_->uponSignal("InterfacesRemoved")
			.onInterface("org.freedesktop.DBus.ObjectManager")
			.call([this](const sdbus::ObjectPath& object, const std::vector<std::string>& interfaces) {
		if (!this->isBaseDevice(object)) {
			return;
		}

		for (const auto& iface: interfaces) {
			if (iface == "org.bluez.Device1") {
				int removed = devices_.remove_if([object](BlueZDevice& dev) {
								return dev.path == object;});
				
				shouldUpdate_ = true;
			}
		}
	});
}

void BlueZState::subManagerSignal() {
	/* sa{sv}as */
	managerProxy_->uponSignal("PropertiesChanged")
			.onInterface("org.freedesktop.DBus.Properties")
			.call([this](const std::string interface, const std::map<std::string, sdbus::Variant>& properties, const std::vector<std::string>& args3) {
		if (interface != "org.bluez.Adapter1") {
			return;
		}

		for (const auto& [key, dict] : properties) {
			if (key == "Powered") {
				powered_ = dict.get<bool>();
				if (powered_ == false)
				this->shouldUpdate_ = true;
			}
		}
	});
}

bool BlueZState::isBaseDevice(const std::string& objectPath) {
	constexpr std::string_view prefix = "/org/bluez/hci0";
	constexpr std::string_view devStr = "dev_";
	
	const auto pos = objectPath.find(devStr, prefix.size());
	if (pos == std::string::npos) {
		return false;
	}

	return objectPath.find("/", pos) == std::string::npos;
}
