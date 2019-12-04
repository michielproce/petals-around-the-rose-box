#include <Keypad.h>
#include <Servo.h>

// Used for random seed 
int unconnectedAnalogPin = 7;

// Connected to servo pwm
int servoPin = 2;
Servo customServo; 

// Connected to ST_CP of 74HC595
int latchPin = 10;
// Connected to SH_CP of 74HC595
int clockPin = 12;
// Connected to DS of 74HC595
int dataPin = 11;

// Keypad
const byte ROWS = 4; 
const byte COLS = 3; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

// Constants
enum state_t {
  normal,
  debug,
  mayhem, 
  counting
};

// Parameters
int dieCount = 5;
int nrOfCorrectResponsesRequired = 3;

// Variables
int* currentFaces;
char* response;
int responseIndex = 0;
int nrOfCorrectResponses = 0;
state_t state = normal;
int counter = 1;


void setup() {
  customServo.attach(servoPin);
  
  pinMode(latchPin, OUTPUT);
 
  randomSeed(analogRead(unconnectedAnalogPin));

  currentFaces = new int[dieCount];
  response = new char[2];

  if (customKeypad.getKey() == '*') {
    state = debug;
  }
  
  throwDice();
}


void loop() {
  char key = customKeypad.getKey();
  
  switch (state) {
    case normal: 
      processInput(key);
      break;
    case debug:                     
      processDebugInput(key);
      break; 
    case mayhem: 
      showMayhem(key);
      break;
    case counting:
      showCounting(key);
      break;
  } 
}


void processInput(char key) {
  if (key == '\0') {
    return;
  }

  int keyInt = (int) key - 48;
  
  if (responseIndex < 2 && keyInt >= 0 && keyInt <= 9) {
    response[responseIndex] = key;
    responseIndex++;      
  }

  if (key == '*') {     
    customServo.write(90);
    reset();

    // Enter debug mode is someone enters '88' followed by a star
    if (response[0] = '7' && response[1] == '3') {
      state = debug;
    }   
  }
  

  if (key == '#') {
    checkResponse();
  }
}


void processDebugInput(char key) {
  if (key == '\0') {
    return;
  }
  
  int keyInt = (int) key - 48;

  digitalWrite(latchPin, 0);    
  if(keyInt >= 1 && keyInt <= 6) {   
    for(int i = 0; i < dieCount; i++) {         
      showDieFace(keyInt, 1);
    }    
  }
 
  if (key == '7') {
    for(int i = 0; i < dieCount; i++) {      
     shiftOut(0B11111111);
    }
  }

  if (key == '0') {
    for(int i = 0; i < dieCount; i++) {      
     shiftOut(0B00000000);
    }
  }
    
  digitalWrite(latchPin, 1);

  if (key == '9') {
      state = mayhem;
  }

  if (key == '8') {
      state = counting;
  }

  if (key == '*') {
    customServo.write(0);
  }
  if (key == '#') {
    customServo.write(90);
  }  
}

void showMayhem(char key) {
  if (key != '\0') {
    state = debug;
    return;
  }
  
  throwDice();
  delay(300);
}


void showCounting(char key) {
  if (key != '\0') {
    state = debug;
    return;
  }
  
  digitalWrite(latchPin, 0);
  for(int i = 0; i < dieCount; i++) {         
     showDieFace(counter, 1);
  }    
  digitalWrite(latchPin, 1);

  counter++;    
  if(counter > 6) {
    counter = 1;
  }
  
  delay(300);
}

void reset() {
  nrOfCorrectResponses = 0;
  responseIndex = 0;
  throwDice();    
}


void checkResponse() {  
  if (responseIndex == 0) {
    return;
  }
  
  if(responseIndex == 1) {    
    if (response[0] - 48 == correctResponse()) {
      correctResponseProvided();
    } else {
      reset();
    }
  }
  
  if(responseIndex == 2) {    
    if ((response[0] - 48) * 10 + (response[1] - 48) == correctResponse()) {
      correctResponseProvided();
    } else {
      reset();
    }
  }
  
  responseIndex = 0; 
}


int correctResponse() {
 int correct = 0;
  for (int i = 0; i < dieCount; i++) {
    if (currentFaces[i] == 3) {
      correct += 2;
    }
    if (currentFaces[i] == 5) {
      correct += 4;
    }
  }
  return correct;
}


void correctResponseProvided() { 
  nrOfCorrectResponses++;
  if(nrOfCorrectResponses >= nrOfCorrectResponsesRequired) {
    customServo.write(0);
  } 
  throwDice();
}


void throwDice() {
  digitalWrite(latchPin, 0);

  for (int i = dieCount - 1; i>=0; i--)  {
    int face = random(1, 7);
    currentFaces[i] = face;
    showDieFace(face, i < nrOfCorrectResponses);
  }
  
  digitalWrite(latchPin, 1);
}


void showDieFace(int roll, int greenLed) {
  switch(roll) {
    case 1:
      shiftOut(0B00010000 | greenLed);
      break;  
    case 2:
      shiftOut(0B01000100 | greenLed);
      break;  
    case 3:
      shiftOut(0B01010100 | greenLed);
      break;  
    case 4:
      shiftOut(0B11000110 | greenLed);
      break;  
    case 5:
      shiftOut(0B11010110 | greenLed);
      break;  
    case 6:
      shiftOut(0B11101110 | greenLed);
      break;  
    default:
      shiftOut(0);
      break;
  }
}


void shiftOut(byte myDataOut) {
  int i=0;
  int pinState;
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  digitalWrite(dataPin, 0);
  digitalWrite(clockPin, 0);

  for (i=7; i>=0; i--)  {
    digitalWrite(clockPin, 0);


    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    digitalWrite(dataPin, pinState);
    digitalWrite(clockPin, 1);
    digitalWrite(dataPin, 0);
  }

  digitalWrite(clockPin, 0);
}
