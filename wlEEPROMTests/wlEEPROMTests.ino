/*
 * EEPROM_
 *
 * Tests reading, writing and updating data in the EEPROM
 * to the computer.
 */
 
#include <wlEEPROM.h>

#include "Arduino.h"

wlEEPROM* EEPROM_;

int getRandomAddress(int size_uint8_ts) {
  return random(0, E2END - size_uint8_ts);
}


template <typename T> bool verifyWriteAndRead(const T input) {
  int address = getRandomAddress(sizeof(T));
  EEPROM_->write(address, input);
  T output;
  EEPROM_->get(address, output);

  if (input != output) {
    Serial.print("address: ");
    Serial.println(address);
    Serial.print("input: ");
    Serial.println(input);
    Serial.print("output: ");
    Serial.println(output);
return false;
  } else
    Serial.println("OK");
  return true;
}


template <typename T> int verifyWriteAndReadBlock(const T input[], const int input_length) {
  int address = getRandomAddress(sizeof(T) * input_length);
  EEPROM_->put(address, input, input_length);
  T output[input_length];
  EEPROM_->get(address, output, input_length);
  
  for (int element = 0; element < input_length; element++) {
    if (input[element] != output[element]) {
      Serial.println("Block write/read error -- elements do not match.");
      Serial.print("address: ");
      Serial.println(address);
      Serial.print("input: ");
      Serial.println(input_length);
      for (int element = 0; element < input_length; element++) {
        Serial.print(input[element]); Serial.print(", ");
      }
      Serial.print("output: ");
      for (int element = 0; element < input_length; element++) {
        Serial.print(output[element]); Serial.print(", ");
      }
      Serial.println("");
      return -1;
    }
  }
  Serial.println("OK");

  return address;
}



// Test reading and writing uint8_t to EEPROM
bool readAndWriteuint8_t() { 
    uint8_t input  = 120;
    return verifyWriteAndRead<uint8_t>(input);
}

// Test reading and writing int to EEPROM
bool readAndWriteInt() {  
    int input  = 30000;
    return verifyWriteAndRead<int>(input);
}

// Test reading and writing long to EEPROM
bool readAndWriteLong() {    
    long input  = 200000000;
    return verifyWriteAndRead<long>(input);
}

// Test reading and writing float to EEPROM
bool readAndWriteFloat() { 
    double input  = 1010102.50;
    return verifyWriteAndRead<float>(input);
}

// Test reading and updating double to EEPROM
bool updateAndReadDouble() { 
    double input  = 1000002.50;
    return verifyWriteAndRead<double>(input);
}

// Test reading and updating a string (char array) to EEPROM
bool writeAndReadCharArray() {
    char input[] = "Arduino";
    return (verifyWriteAndReadBlock<char>(input, 7) >= 0);
}

bool writeAndReadByteArray() {
    int itemsInArray = 7;
    uint8_t initial[] = {1, 0, 4, 0, 16, 0 , 64 };
    int blockAddress = verifyWriteAndReadBlock(initial, itemsInArray);
    if (blockAddress < 0)
      return false;

    uint8_t input[]   = {1, 2, 4, 8, 16, 32, 64 };
    uint8_t output[sizeof(input)];
    int expectedWrittenuint8_ts = 3;
    EEPROM_->put<uint8_t>(blockAddress, input, itemsInArray);
    EEPROM_->get<uint8_t>(blockAddress, output, itemsInArray);

    for(int element = 0; element < itemsInArray; element++) {
      if (input[element] != output[element]) {
        Serial.println("Update failed -- elements do not match.");
        return false;
      }
    }

    return true;
}


bool writeAndReadStruct() {
  struct PERSON {   // Declare PERSON struct type
    int age;   // Declare member types
    long ss;
    float weight;
    char name[25];
  } family_member;   // Define object of type PERSON
  family_member.age = 75;
  family_member.ss = 123455L;
  family_member.weight = 160.23;
  strcpy(family_member.name, "Skippy Johnson");

  int address = getRandomAddress(sizeof(family_member));
  EEPROM_->put(address, family_member);
  PERSON output;
  EEPROM_->get(address, output);
  if (output.age != family_member.age || strcmp(output.name, family_member.name) != 0) {
    Serial.print("output: ");
    Serial.print(output.age);
    Serial.print(output.name);
    return false;
  }
  Serial.println("OK");
  return true;
}

// Check if we get errors when writing too much or out of bounds
void errorChecking() {
    Serial.println("Expecting Attempt to write outside of EEPROM memory...");
    if (EEPROM_->put(E2END + 1, long(1)))
      Serial.println("ERROR: Write outside E2END should have failed!");    
}



bool testFindWearKey() {
  // Set up a deterministic memory pool
  delete[] EEPROM_;
  EEPROM_ = new wlEEPROM();
  wear_profile profile = {
    {'A','B','C','1','2','3','0','0'}
  };
  

  const char test_key[] = "ABC12300";
  // Write the key to a memory address
  int address = 255;
  EEPROM_->put(address, test_key, 8);

  // Attempt to find the key within the memory block
  int key_location = EEPROM_->findWearLevelledData_(profile);
  if (key_location != address)
    return false;
  Serial.print("OK");
  return true;
}


bool testwriteWearLevelling() {
  struct WEARTEST {
    uint8_t one;
    uint16_t two;
    int three;
  } test_struct;
  test_struct.one = 1;
  test_struct.two = 2;
  test_struct.three = 3;

  wear_profile profile = {
    {'A','B','C','1','2','3','0','0'}
  };
  
  // Set up a deterministic memory pool
  delete[] EEPROM_;
  EEPROM_ = new wlEEPROM();

  const char test_key[] = "wearlevl";
  // Write the key, checksum, and data to a memory address
  int address = 255;
  EEPROM_->writeWearLevelledData(profile, test_struct);
  // Verify checksum
  if (int(EEPROM_->checkSum_(test_struct)) != 6) {
    Serial.print("Error: expected checksum == 6, got ");
    Serial.println(int(EEPROM_->checkSum_(test_struct)));
  }

  // Attempt to find the key within the memory block
  WEARTEST read_data;
  int data_size = EEPROM_->readWearLevelledData(profile, read_data);
  if (data_size != sizeof(read_data))
    return false;
  Serial.print("OK");
  return true;
}
    
  
void setup()
{  
  randomSeed(analogRead(0));
  EEPROM_ = new wlEEPROM();
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  // Writes before membase or beyond memBase + memSizeuint8_ts will only give errors when _EEPROM__DEBUG is set
  // Create randomized memory pool to avoid excessive writes during testing
  int memSizeuint8_ts = 80;
  const int memBase = random(0, E2END - memSizeuint8_ts);
  Serial.print("Memory pool set to "); Serial.print(memBase); Serial.print(" - ");
  Serial.println(memBase + memSizeuint8_ts);
  EEPROM_->setMemoryPool(memBase, memBase + memSizeuint8_ts);

  testwriteWearLevelling();
  return;
  
  Serial.println("");       

  // Read and write different data primitives
  Serial.print("Testing readAndWriteuint8_t...");
  if (!readAndWriteuint8_t()) {
    Serial.println("Error in readAndWriteuint8_t");
  }
  Serial.print("Testing readAndWriteInt...");
  if (!readAndWriteInt()) {
    Serial.println("Error in readAndWriteInt");
  }
  Serial.print("Testing readAndWriteLong...");
  if (!readAndWriteLong()) {
    Serial.println("Error in readAndWriteLong");
  }
  Serial.print("Testing readAndWriteFloat...");
  if (!readAndWriteFloat()) {    
    Serial.println("Error in readAndWriteFloat");
  }
  Serial.print("Testing updateAndReadDouble...");
  if (!updateAndReadDouble()) {
    Serial.println("Error in readAndWriteDouble");
  }
  Serial.print("Testing writeAndReadCharArray...");
  if (!writeAndReadCharArray()) {
    Serial.println("Error in writeAndReadCharArray");  
  }
  Serial.print("Testing writeAndReaduint8_tArray...");
  if (!writeAndReadByteArray()) {
    Serial.println("Error in writeAndReaduint8_tArray");
  }
  Serial.print("Testing writeAndReadStruct...");
  if (!writeAndReadStruct()) {
    Serial.println("Error in writeAndReadStruct");
  }
  
  // Test error checking
  errorChecking();

  Serial.print("Testing findWearKey...");
  if (!testFindWearKey()) {
    Serial.println("Error in findWearKey");
  }
}

void loop()
{
  // Nothing to do during loop
}    


