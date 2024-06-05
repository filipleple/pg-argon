#include "Particle.h"

#define LED_CUSTOM D7

#define N_POS 12

SerialLogHandler logHandler(LOG_LEVEL_INFO);
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

const size_t SCAN_RESULT_MAX = 100;
BleScanResult scanResults[SCAN_RESULT_MAX];
BleScanParams scanParams;


int count;
int espar_current_pos = 1;
float signal_strength_in_position[N_POS];
int bestRSSI = -999;

enum ARGON_MODES {
	SCANNING,
	HOLD_AND_MONITOR
};

ARGON_MODES current_mode = SCANNING; 


void logDevices(int deviceCount, String mode);
void goto_best_position();

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
	scanParams.timeout = 300;						// Change timeout to 10 seconds
	
	// Scanning for both 1 MBPS and CODED PHY simultaneously requires scanning window <= 1/2 the scanning interval.
	// We will widen the window to 2/3 of the interval so automatic override will be tested in simultaneous mode
	
	scanParams.window = (2 * scanParams.interval) / 3 ;
	BLE.setScanParameters(&scanParams);				// Set the modified scan parameters

	// serial comm. over TX/RX pins, to be connected to nrf52-DK espar handler
	Serial1.begin(115200);

	Serial1.print("rotate ");
	Serial1.println(espar_current_pos);
	delay(5000);	

}

void loop() {
	switch(current_mode){
		case SCANNING:
			scanParams.scan_phys = BLE_PHYS_1MBPS;
			BLE.setScanParameters(scanParams);
			count = BLE.scan(scanResults, SCAN_RESULT_MAX);
			digitalWrite(LED_CUSTOM, HIGH);
			//logDevices(count, "PHYS_1MBPS (standard)");
			digitalWrite(LED_CUSTOM, LOW);

			for (int i = 0; i < count; i++){
				if (scanResults[i].rssi() > bestRSSI){
					bestRSSI = scanResults[i].rssi();
				}
			}
			Log.info("Scanning position: %d, found %d devices, best RSSI: %d", espar_current_pos, count, bestRSSI);   

			signal_strength_in_position[espar_current_pos-1] = bestRSSI;
			bestRSSI = -999;

			if (espar_current_pos <= 11){
				espar_current_pos++;
				Serial1.println("rotate next");
			}
			else {
				goto_best_position();
			}			

			break;
		case HOLD_AND_MONITOR:
			Serial1.println("hold");
			break;	
	}
	
}

void goto_best_position(){
	int bestSignal = -999;
	int bestPos = -1;
	for (int i = 0; i < N_POS; i++){
		if(signal_strength_in_position[i] > bestSignal){
			bestSignal = signal_strength_in_position[i];
			bestPos = i;
		}
	}

	Serial1.print("rotate ");
	Serial1.println(bestPos);

	current_mode = HOLD_AND_MONITOR;
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
	//Serial1.println("rotate 4");
	
}