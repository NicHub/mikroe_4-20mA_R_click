/**
 * https://medium.com/electronza/4-20ma-r-current-loop-arduino-tutorial-part-iii-receiver-a2787c1919c9
 */

#include <Arduino.h>
/*
  4-20mA R click receiver
  Reads result on 4-20mA bus
 */
#include <SPI.h>

// Arduino UNO with Mikroe Arduino Uno Click shield
// 4-20mA R click is placed in socket #2
// CS   is pin 9
// SCK  is pin 13
// MISO is pin 12
// MOSI is pin 11
#define ADC_CS 9

int loop_current;
int received_data;

// Calibration data obtained by running the calibration code
const int ADC_4mA = 784;
const int ADC_20mA = 3954;

// Data min and max range
// Matches the values on the transmitter code
// But it's a good ideea to resample to a lower resolution
const int data_min_range = 0;
const int data_max_range = 1023;

unsigned int get_ADC(void)
{
    /*
    DAC works on SPI
    We receive 16 bits
    Of which we extract only 12 bits
    MCP3201 has a strange way of formatting data
    with 5 bits in the first byte and
    the rest of 7 bits in the second byte
    */
    unsigned int result;
    unsigned int first_byte;
    unsigned int second_byte;

    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    digitalWrite(ADC_CS, 0);
    first_byte = SPI.transfer(0);
    second_byte = SPI.transfer(0);
    digitalWrite(ADC_CS, 1);
    SPI.endTransaction();

    /* After the second eight clocks have been
    sent to the device, the MCU receive register
    will contain the lowest order seven bits and
    the B1 bit repeated as the A/D Converter has begun
    to shift out LSB first data with the extra clock.
    Typical procedure would then call for the lower order
    byte of data to be shifted right by one bit
    to remove the extra B1 bit.
    See MCP3201 datasheet, page 15
    */
    result = ((first_byte & 0x1F) << 8) | second_byte;
    result = result >> 1;
    return result;
}

int ReadFrom420mA(void)
{
    int result;
    int ADC_result;
    float ADC_avrg = 0;
    for (int i = 0; i < 100; i++)
    {
        ADC_result = get_ADC();
        // Measure every 1ms
        delay(1);
        ADC_avrg = ADC_avrg + ADC_result;
    }
    result = (int)(ADC_avrg / 100);
    Serial.print("result = ");
    Serial.println(result);
    // now we do some shortcircuit and open loop checking
    // open loop
    if (result < (ADC_4mA - 50))
    {
        return -1;
    }
    // shortcircuit
    if (result > (ADC_20mA + 50))
    {
        return -2;
    }
    // everything is OK
    return result;
}

void setup()
{
    /* Resetting MCP3201
     * From MCP3201 datasheet: If the device was powered up
     * with the CS pin low, it must be brought high and back low
     * to initiate communication.
     * The device will begin to sample the analog
     * input on the first rising edge after CS goes low. */
    pinMode(ADC_CS, OUTPUT);
    digitalWrite(ADC_CS, 0);
    delay(100);
    digitalWrite(ADC_CS, 1);

    // initialize serial
    Serial.begin(BAUD_RATE);
    // initialize SPI
    SPI.begin();
}

void loop()
{
    // Read the loop current
    loop_current = ReadFrom420mA();
    // Error checking
    if (loop_current == -1)
    {
        Serial.print(millis());
        Serial.println(" - Error: open loop");
    }
    else if (loop_current == -2)
    {
        Serial.print(millis());
        Serial.println(" - Error: current loop is in short circuit");
    }
    // All is OK, remapping to initial data range
    else
    {
        received_data = map(loop_current, ADC_4mA, ADC_20mA, data_min_range, data_max_range);
        Serial.print(millis());
        Serial.print(" - Received value is: ");
        Serial.println(received_data);
    }
}
