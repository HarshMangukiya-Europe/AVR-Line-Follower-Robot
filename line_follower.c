#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

// Motor direction pins
#define M1_DIR PB0  // Right motor direction
#define M2_DIR PB3  // Left motor direction

// P-Control parameters
#define KP         5.5
#define BASE_SPEED 150

// Calibration & EEPROM
#define CALIB_BUTTON PD2
#define LED_PIN      PB5
#define OFFSET       10

// EEPROM storage
uint16_t EEMEM eeprom_left_threshold;
uint16_t EEMEM eeprom_right_threshold;

// Runtime thresholds
uint16_t left_threshold;
uint16_t right_threshold;

void init_io() {
    // Motor direction + PWM + LED
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << LED_PIN);

    // PD4 (wire bridge) & PD2 (calibration button) as input with pull-up
    DDRD  &= ~((1 << PD4) | (1 << CALIB_BUTTON));
    PORTD |=  (1 << PD4)  | (1 << CALIB_BUTTON);
}

void init_adc() {
    ADMUX  = (1 << REFS0);               // AVcc reference
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, prescaler 128
}

void init_pwm() {
    // Timer1 Fast PWM 8-bit
    TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
    TCCR1B = (1 << WGM12)  | (1 << CS11)   | (1 << CS10); // Prescaler 64
    OCR1A  = 0;
    OCR1B  = 0;
}

uint16_t read_adc(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    _delay_us(5);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void calibrate_sensors() {
    _delay_ms(50); // Debounce

    uint16_t left  = read_adc(0); // A0
    uint16_t right = read_adc(1); // A1

    left_threshold  = left  + OFFSET;
    right_threshold = right + OFFSET;

    eeprom_write_word(&eeprom_left_threshold,  left_threshold);
    eeprom_write_word(&eeprom_right_threshold, right_threshold);

    // Blink onboard LED twice to confirm calibration
    for (uint8_t i = 0; i < 2; i++) {
        PORTB |=  (1 << LED_PIN);
        _delay_ms(200);
        PORTB &= ~(1 << LED_PIN);
        _delay_ms(200);
    }
}

int main(void) {
    init_io();
    init_adc();
    init_pwm();

    // Calibration check — if button pressed on startup, calibrate
    if (!(PIND & (1 << CALIB_BUTTON))) {
        calibrate_sensors();
    } else {
        // Load thresholds from EEPROM
        left_threshold  = eeprom_read_word(&eeprom_left_threshold);
        right_threshold = eeprom_read_word(&eeprom_right_threshold);
    }

    // Wire bridge on PD4: LOW = P-Control mode, HIGH = Bang-Bang mode
    uint8_t isPControl = !(PIND & (1 << PD4));

    while (1) {
        uint16_t left  = read_adc(0); // A0
        uint16_t right = read_adc(1); // A1

        uint8_t leftIsBlack  = (left  >= left_threshold);
        uint8_t rightIsBlack = (right >= right_threshold);

        if (isPControl) {
            // ----- P-Control Mode -----
            int16_t left_error  = left  - left_threshold;
            int16_t right_error = right - right_threshold;
            int16_t error       = right_error - left_error;
            int16_t correction  = (error * KP) / 10;

            int16_t left_speed  = BASE_SPEED - correction;
            int16_t right_speed = BASE_SPEED + correction;

            // Clamp speeds to valid PWM range
            if (left_speed  > 255) left_speed  = 255;
            if (left_speed  < 0)   left_speed  = 0;
            if (right_speed > 255) right_speed = 255;
            if (right_speed < 0)   right_speed = 0;

            if (!leftIsBlack && !rightIsBlack) {
                // Full white - reverse
                PORTB |=  (1 << M1_DIR) | (1 << M2_DIR);
                OCR1A  = BASE_SPEED;
                OCR1B  = BASE_SPEED;
            } else {
                // Forward with correction
                PORTB &= ~((1 << M1_DIR) | (1 << M2_DIR));
                OCR1A  = right_speed;
                OCR1B  = left_speed;
            }

        } else {
            // ----- Bang-Bang Mode -----
            uint16_t pot   = read_adc(7);        // A7 - potentiometer for speed
            uint8_t  speed = pot >> 2;            // Scale 0-1023 to 0-255

            if (leftIsBlack && rightIsBlack) {
                // Both black -> forward
                PORTB &= ~((1 << M1_DIR) | (1 << M2_DIR));
                OCR1A  = speed;
                OCR1B  = speed;
            } else if (!leftIsBlack && !rightIsBlack) {
                // Both white -> backward
                PORTB |=  (1 << M1_DIR) | (1 << M2_DIR);
                OCR1A  = speed;
                OCR1B  = speed;
            } else if (leftIsBlack && !rightIsBlack) {
                // Left on line -> turn left
                PORTB |=  (1 << M1_DIR);
                PORTB &= ~(1 << M2_DIR);
                OCR1A  = speed;
                OCR1B  = speed;
            } else if (!leftIsBlack && rightIsBlack) {
                // Right on line -> turn right
                PORTB &= ~(1 << M1_DIR);
                PORTB |=  (1 << M2_DIR);
                OCR1A  = speed;
                OCR1B  = speed;
            }
        }
    }
}
