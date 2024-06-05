#include "Particle.h"

#define LED_CUSTOM D7

#define N_POS 12
#define HOLD_MODE_INTERVAL_MS 1000
#define HOLD_MODE_DROP 6

int HOLD_RSSI = -999;

SerialLogHandler logHandler(LOG_LEVEL_INFO);
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

const size_t SCAN_RESULT_MAX = 100;
BleScanResult scanResults[SCAN_RESULT_MAX];
BleScanParams scanParams;


int count;
int espar_current_pos = 1;
int signal_strength_in_position[N_POS];
int bestRSSI = -999;

enum ARGON_MODES {
	SCANNING,
	HOLD_AND_MONITOR
};

ARGON_MODES current_mode = SCANNING; 


void logDevices(int deviceCount, String mode);
void goto_best_position();
int scan_for_best_rssi();
void rotate_espar(int position);
void espar_hold();

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
			rotate_espar(espar_current_pos);			

			bestRSSI = scan_for_best_rssi();
			Log.info("Scanning position: %d, best RSSI: %d", espar_current_pos, bestRSSI);
			signal_strength_in_position[espar_current_pos-1] = bestRSSI;
			
			// after a full rotation, pick and go to the best position
			if (espar_current_pos <= 11){
				espar_current_pos++;				
			}
			else {
				goto_best_position();
			}
			break;

		case HOLD_AND_MONITOR:
			Log.info("Hold mode; monitoring RSSI");
			espar_hold();
			bestRSSI = scan_for_best_rssi();
			if (bestRSSI < HOLD_RSSI - HOLD_MODE_DROP){
				Log.info("RSSI drop detected; rescanning");
				espar_current_pos = 1;
				rotate_espar(espar_current_pos);
				current_mode = SCANNING;
				break;
			}
			delay(HOLD_MODE_INTERVAL_MS);
			break;	
	}
	
}

int scan_for_best_rssi(){
	int bestRSSI = -999;
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
	return bestRSSI;
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
	rotate_espar(bestPos+1);

	Log.info("Best position found around dir %d\r\n", bestPos+1);
	Log.info("Going into hold mode\r\n");

	HOLD_RSSI = bestSignal;
	current_mode = HOLD_AND_MONITOR;
}

void rotate_espar(int position){
	Serial1.print("rotate ");
	Serial1.println(position);
	// get response and verify instead of delay
	delay(500);
}

void espar_hold(){
	Serial1.println("hold");
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