/*
  Active Learning Labs
  Harvard University 
  tinyMLx - OV7675 Camera Test

  Requires the the Arduino_OV767X library
*/

#include <Arduino_OV767X.h>

#define BUTTON_PIN 13

bool commandRecv = false; // flag used for indicating receipt of commands from serial port
bool liveFlag = false; // flag as true to live stream raw camera bytes, set as false to take single images on command
bool captureFlag = false;

// Variables for button debouncing
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;
bool buttonState;

// Image buffer;
byte image[176 * 144 * 2]; // QCIF: 176x144 x 2 bytes per pixel (RGB565)
int bytesPerFrame;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(BUTTON_PIN, OUTPUT);
  digitalWrite(BUTTON_PIN, HIGH);
  nrf_gpio_cfg_out_with_input(digitalPinToPinName(BUTTON_PIN));

  // Initialize camera
  if (!Camera.begin(QCIF, RGB565, 1)) {
    Serial.println("Failed to initialize camera");
    while (1);
  }
  bytesPerFrame = Camera.width() * Camera.height() * Camera.bytesPerPixel();

  Serial.println("Welcome to the OV7675 test\n");
  Serial.println("Available commands:\n");
  Serial.println("single - take a single image and print out the hexadecimal for each pixel (default)");
  Serial.println("live - the raw bytes of images will be streamed live over the serial port");
  Serial.println("capture - when in single mode, initiates an image capture");
}

void loop() {
  int i = 0;
  String command;

  bool buttonRead = nrf_gpio_pin_read(digitalPinToPinName(BUTTON_PIN));
  
  if (buttonRead != lastButtonState) {
    lastDebounceTime = millis();
  }

  if (millis() - lastDebounceTime >= debounceDelay) {
    if (buttonRead != buttonState) {
      buttonState = buttonRead;

      if (!buttonState) {
        if (!liveFlag) {
          if (!captureFlag) {
            captureFlag = true;
          }
        }
      }
    }
  }

  lastButtonState = buttonRead;

  // Read incoming commands from serial monitor
  while (Serial.available()) {
    char c = Serial.read();
    if ((c != '\n') && (c != '\r')) {
      command.concat(c);
    } 
    else if (c == '\r') {
      commandRecv = true;
      command.toLowerCase();
    }
  }

  // Command interpretation
  if (commandRecv) {
    commandRecv = false;
    if (command == "live") {
      Serial.println("\nRaw image data will begin streaming in 5 seconds...");
      liveFlag = true;
      delay(5000);
    }
    else if (command == "single") {
      Serial.println("\nCamera in single mode, type \"capture\" to initiate an image capture");
      liveFlag = false;
      delay(200);
    }
    else if (command == "capture") {
      if (!liveFlag) {
        if (!captureFlag) {
          captureFlag = true;
        }
        delay(200);
      }
      else {
        Serial.println("\nCamera is not in single mode, type \"single\" first");
        delay(1000);
      }
    }
  }
  
  if (liveFlag) {
    Camera.readFrame(image);
    Serial.write(image, bytesPerFrame);
  }
  else {
    if (captureFlag) {
      captureFlag = false;
      Camera.readFrame(image);
      Serial.println("\nImage data will be printed out in 3 seconds...");
      delay(3000);
      for (int i = 0; i < bytesPerFrame - 1; i += 2) {
        Serial.print("0x");
        Serial.print(image[i+1], HEX);
        Serial.print(image[i], HEX);
        if (i != bytesPerFrame - 2) {
          Serial.print(", ");
        }
      }
      Serial.println();
    }
  }
}

void nrf_gpio_cfg_out_with_input(uint32_t pin_number) {
  nrf_gpio_cfg(
    pin_number,
    NRF_GPIO_PIN_DIR_OUTPUT,
    NRF_GPIO_PIN_INPUT_CONNECT,
    NRF_GPIO_PIN_PULLUP,
    NRF_GPIO_PIN_S0S1,
    NRF_GPIO_PIN_NOSENSE);
}
