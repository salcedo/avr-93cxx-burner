/*
 * 93Cxx EEPROM programmer - Tested on 93C56.
 * Arduino sketch written for Mega.
 *
 * Pin assignments may be changed.
 */

// Push button to toggle between read / write modes.
#define MODE_BUTTON     47

// Status LEDs
#define LED_DETECTED    53
#define LED_WRITE       49
#define LED_READ        51

// EEPROM pins
#define EEPROM_CS       46
#define EEPROM_DO       52
#define EEPROM_DI       50
#define EEPROM_SK       48

// needed EEPROM op codes
#define OP_READ      0x004
#define OP_READ_LEN      3
#define OP_EWEN      0x180
#define OP_EWEN_LEN     11
#define OP_WRITE     0x002
#define OP_WRITE_LEN     3
#define OP_ERAL      0x100
#define OP_ERAL_LEN     11
#define OP_EWDS      0x000
#define OP_EWDS_LEN     11

// size we're working with.
#define EEPROM_SIZE    256

byte program_data[EEPROM_SIZE];

// one clock cycle
void eeprom_clock(void)
{
	digitalWrite(EEPROM_SK, HIGH);
	delay(1);
	digitalWrite(EEPROM_SK, LOW);
	delay(1);
}

// send start condition
void eeprom_begin_instruction(void)
{
	digitalWrite(EEPROM_DI, HIGH);
	digitalWrite(EEPROM_CS, HIGH);
	delay(1);
	
	eeprom_clock();
}

// lower CS.
void eeprom_end_instruction(void)
{
	digitalWrite(EEPROM_CS, LOW);
	delay(2);
}

// send opcode bits
void eeprom_send_opcode(int opcode, int len)
{
	int i, dbit;
	
	for(i = len-1; i >= 0; i--) {
		dbit = (opcode >> i) & 1;
		digitalWrite(EEPROM_DI, dbit);
		delay(1);
		
		eeprom_clock();
	}
}

// send address or data byte
void eeprom_send_address(int address)
{
	int i, dbit;
	for(i = 0; i < 8; i++) {
		dbit = (address >> (7 - i)) & 1;
		digitalWrite(EEPROM_DI, dbit);
		delay(1);
		
		eeprom_clock();
	}
}

// returns a byte read from DO
int eeprom_read_byte(void)
{
	int i, dbits = 0, dbit;
	
	for(i = 0; i < 8; i++) {
		digitalWrite(EEPROM_SK, HIGH);
		delay(1);
		dbit = digitalRead(EEPROM_DO);
		delay(1);
		digitalWrite(EEPROM_SK, LOW);
		delay(1);
		
		dbits |= dbit << (7-i);
	}
	
	return dbits;
}

// read a byte at specified address
int eeprom_read_data(int address)
{
	int dbits = 0;
	
	eeprom_begin_instruction();
	eeprom_send_opcode(OP_READ, OP_READ_LEN);
	eeprom_send_address(address);
	dbits = eeprom_read_byte();
	eeprom_end_instruction();
	
	return dbits;
}

// write byte to address
void eeprom_write_data(int address, int data)
{
	eeprom_begin_instruction();
	eeprom_send_opcode(OP_WRITE, OP_WRITE_LEN);
	eeprom_send_address(address);
	eeprom_send_address(data);
	eeprom_end_instruction();

	delay(10);
}

// enable writing
void eeprom_write_enable(void)
{
	eeprom_begin_instruction();
	eeprom_send_opcode(OP_EWEN, OP_EWEN_LEN);
	eeprom_end_instruction();
}

// disable writing
void eeprom_write_disable(void)
{
	eeprom_begin_instruction();
	eeprom_send_opcode(OP_EWDS, OP_EWDS_LEN);
	eeprom_end_instruction();
}

// erase entire chip
void eeprom_erase_chip(void)
{
	eeprom_begin_instruction();
	eeprom_send_opcode(OP_ERAL, OP_ERAL_LEN);
	eeprom_end_instruction();
}

// this could be dangerous but it works.
int detect_eeprom(void)
{
	int odbits = 0, dbits = 0;

	digitalWrite(LED_DETECTED, HIGH);
	odbits = eeprom_read_data(0);
	
	eeprom_write_enable();
	eeprom_write_data(0, 0x42);
	dbits = eeprom_read_data(0);
	eeprom_write_data(0, odbits);
	eeprom_write_disable();
	
	if(dbits == 0x42) {
		return 1;
	}
	
	digitalWrite(LED_DETECTED, LOW);
	return 0;
}

void led_bling(void)
{
	int i;
	
	for(i = 0; i < 5; i++) {
		digitalWrite(LED_DETECTED, HIGH);
		delay(100);
		digitalWrite(LED_DETECTED, LOW);
		digitalWrite(LED_WRITE, HIGH);
		delay(100);
		digitalWrite(LED_WRITE, LOW);
		digitalWrite(LED_READ, HIGH);
		delay(100);
		digitalWrite(LED_READ, LOW);
	}
}

#define STATE_READING 0
#define STATE_WRITING 1

int current_state = STATE_WRITING;
int ran_state = 0;
int eeprom_detected = 0;
int mode_button;

void run_state(int state)
{
	int i = 0;
	
	eeprom_detected = detect_eeprom();
	
	if(eeprom_detected == 0)
		return;
	
	switch(state) {
		case STATE_READING:
			digitalWrite(LED_READ, HIGH);
		
			for(i = 0; i < EEPROM_SIZE; i++) {
				program_data[i] = eeprom_read_data(i);
				Serial.write(program_data[i]);
			}
		
			Serial.flush();
		
			digitalWrite(LED_READ, LOW);
			break;
		
		case STATE_WRITING:
			digitalWrite(LED_WRITE, HIGH);
			digitalWrite(LED_READ, HIGH);
			
			Serial.print("OK");
			Serial.flush();
			
			i = 0;
			do {
				if(Serial.available()) {
					program_data[i++] = Serial.read();
				}
			} while(i < EEPROM_SIZE);
			
			Serial.flush();
			digitalWrite(LED_READ, LOW);
			
			eeprom_write_enable();
			eeprom_erase_chip();
			eeprom_write_enable();
			
			for(i = 0; i < EEPROM_SIZE; i++) {
				eeprom_write_data(i, program_data[i]);
			}
			
			eeprom_write_disable();

			digitalWrite(LED_WRITE, LOW);
			Serial.print("OK");
			
			break;
	}
	
	ran_state = 1;
}

void enter_write_state(void)
{
	int i = 0;

	current_state = STATE_WRITING;
		
	for(i = 0; i < 5; i++) {
		digitalWrite(LED_WRITE, HIGH);
		delay(50);
		digitalWrite(LED_WRITE, LOW);
		delay(50);
	}
}

void setup()
{
	pinMode(MODE_BUTTON, INPUT);
	pinMode(LED_DETECTED, OUTPUT);
	pinMode(LED_WRITE, OUTPUT);
	pinMode(LED_READ, OUTPUT);
	
	pinMode(EEPROM_CS, OUTPUT);
	pinMode(EEPROM_DO, INPUT);
	pinMode(EEPROM_DI, OUTPUT);
	pinMode(EEPROM_SK, OUTPUT);
	
	digitalWrite(EEPROM_CS, LOW);
	digitalWrite(EEPROM_DI, LOW);
	digitalWrite(EEPROM_SK, LOW);
	
	delay(3000);
	Serial.begin(9600);
	
	led_bling();
	current_state = STATE_READING;	
}

int probe_time = 0;

void loop()
{
	mode_button = digitalRead(MODE_BUTTON);
	
	if(eeprom_detected == 0 && mode_button == HIGH && current_state != STATE_WRITING) {
		enter_write_state();
		delay(500);
	}

	if(mode_button == LOW && eeprom_detected == 0 && probe_time >= 4000) {
		eeprom_detected = detect_eeprom();
		probe_time = 0;
	} else {
		probe_time++;
	}
	
	if(eeprom_detected == 1) {
		if(ran_state == 0)
			run_state(current_state);
	} else {
		delay(1);
	}
}
