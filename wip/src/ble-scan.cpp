#include "Particle.h"

#define LED_CUSTOM D7

SerialLogHandler logHandler(LOG_LEVEL_INFO);
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

const size_t SCAN_RESULT_MAX = 100;
BleScanResult scanResults[SCAN_RESULT_MAX];
BleScanParams scanParams;


void logDevices(int deviceCount, String mode);

void setup() {
	pinMode(LED_CUSTOM, OUTPUT);
	RGB.control(true);								// disable status led flicker
	delay(5000);									// Allow time for USB serial to connect

	// supposedly picks the external BT antenna
	BLE.selectAntenna(BleAntennaType::EXTERNAL);
	
	BLE.on();
	Log.info("BLE 5 PHY CODED SCAN TEST");
	scanParams.version = BLE_API_VERSION;
	scanParams.size = sizeof(BleScanParams);
	BLE.getScanParameters(&scanParams);				// Get the default scan parameters
	scanParams.timeout = 1000;						// Change timeout to 10 seconds
	
	// Scanning for both 1 MBPS and CODED PHY simultaneously requires scanning window <= 1/2 the scanning interval.
	// We will widen the window to 2/3 of the interval so automatic override will be tested in simultaneous mode
	
	scanParams.window = (2 * scanParams.interval) / 3 ;
	BLE.setScanParameters(&scanParams);				// Set the modified scan parameters

	// serial comm. over TX/RX pins, to be connected to nrf52-DK espar handler
	Serial1.begin(115200);
}

void loop() {
	int count;
	
	// First, scan and log with standard PHYS_1MBPS
	scanParams.scan_phys = BLE_PHYS_1MBPS;
	BLE.setScanParameters(scanParams);
	count = BLE.scan(scanResults, SCAN_RESULT_MAX);
	digitalWrite(LED_CUSTOM, HIGH);
	logDevices(count, "PHYS_1MBPS (standard)");
	digitalWrite(LED_CUSTOM, LOW);
}

void logDevices(int deviceCount, String mode) {
	Log.info("Scanning with %s found %d devices", mode.c_str(), deviceCount);   

	for (int i = 0; i < deviceCount; i++) {
		Log.info( 
			"Device rssi: %d dBm, name: %s",       
			scanResults[i].rssi(), 
			scanResults[i].advertisingData().deviceName().c_str()
		);
	}

	Log.info("--------------------------------------------------------");

	//Serial1.print("hello from Argon; ");
	Serial1.print("found ");
	Serial1.print(deviceCount);
	Serial1.println(" devices");
	
}