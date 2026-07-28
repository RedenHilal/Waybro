#ifndef WBRO_BLUEZ_MODULE
#define WBRO_MODULE_BLUEZ

#include <sdbus-c++/sdbus-c++.h>
#include <array>
#include <list>
#include <map>
#include <memory>
#include <string>

class BlueZState {

public:

	struct BlueZDevice {
		std::unique_ptr<sdbus::IProxy> proxy;
		std::string path;
		std::string alias;
		std::string address;
		bool connected;
	};


	/*
	 * buffer for text data
	 */
	std::array<char, 128> text_;


	explicit BlueZState(std::unique_ptr<sdbus::IConnection> conn);

	void setUp();

	int getFd();

	void getManagedObjects();

	void subSignals();

	void processEvent();

	[[nodiscard]] bool isStateChanged();

	[[nodiscard]] const std::string& getAlias();

	[[nodiscard]] const std::string& getAddress();

	[[nodiscard]] const std::list<BlueZDevice>& getDevices();

	[[nodiscard]] int getConnectedCount();

	[[nodiscard]] bool isPowered();


private:

	using PropertyMap = std::map<std::string, sdbus::Variant>;
	using InterfaceMap = std::map<std::string, PropertyMap>;
	using ObjectMap = std::map<sdbus::ObjectPath, InterfaceMap>;
	

	std::unique_ptr<sdbus::IConnection> conn_;
	std::list<BlueZDevice> devices_ = {};

	std::unique_ptr<sdbus::IProxy> rootProxy_;
	std::unique_ptr<sdbus::IProxy> managerProxy_;

	std::string alias_;
	std::string address_;

	bool powered_ = false;
	bool shouldUpdate_ = false;

	/*
	 * currently not used
	 */
	int conCount_ = 0;

	sdbus::Variant getDeviceProperty(const std::string& path, const std::string& prop);

	void getAdapterProperties(const PropertyMap& properties);

	void getDeviceProperties(const PropertyMap& properties, struct BlueZDevice& dev);

	void processAdapterInterfaces(const InterfaceMap& interfaces);

	void processDeviceInterfaces(const InterfaceMap& interfaces, const std::string& path);

	void subDeviceSignal(struct BlueZDevice& dev);

	void subRootSignal();

	void subManagerSignal();

	bool isBaseDevice(const std::string& objectPath);


};

#endif
