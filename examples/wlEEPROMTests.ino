/*
 * EEPROMwl
 * Tests for all library funcions.
 */

#include <wlEEPROM.h>

#include "Arduino.h"

wlEEPROM* EEPROMwl = new wlEEPROM;


void setup()
{
  // Seed the RNG
  randomSeed(analogRead(0));
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  wear_profile profile = {
    {},
    0x00,
    0
  };
  makeRandomKey(profile.key, 8);

  Serial.print("Testing wlWrite...");
  int key_address = testwlWrite(profile);
  if (key_address < 0) {
    Serial.print("Error in wlWrite: ");
    Serial.println(key_address);
  }

  Serial.print("Testing wlRead...");
  int err_code = testwlRead(key_address, profile);
  if (err_code < 0) {
    Serial.print("Error in wlRead: ");
    Serial.println(err_code);
  }

  // Create a randomized memory pool to avoid excessive writes to the same
  // memory space during testing
  int memSizeBytes = 80;
  const int memBase = random(0, E2END - memSizeBytes);
  Serial.print("Memory pool set to "); Serial.print(memBase); Serial.print(" - ");
  Serial.println(memBase + memSizeBytes);
  EEPROMwl->setMemoryPool(memBase, memBase + memSizeBytes);


  // Read and write different data types
  Serial.print("Testing readAndWriteuint8_t...");
  if (!readAndWriteuint8_t()) {
    Serial.println("Error in readAndWriteuint8_t");
  }
  Serial.print("Testing updateAndReadDouble...");
  if (!updateAndReadDouble()) {
    Serial.println("Error in readAndWriteDouble");
  }
  Serial.print("Testing writeAndReadCharArray...");
  if (!writeAndReadCharArray()) {
    Serial.println("Error in writeAndReadCharArray");
  }
  Serial.print("Testing writeAndReadIntArray...");
  if (!writeAndReadIntArray()) {
    Serial.println("Error in writeAndReadIntArray");
  }
  Serial.print("Testing writeAndReadStruct...");
  if (!writeAndReadStruct()) {
    Serial.println("Error in writeAndReadStruct");
  }

  delete EEPROMwl;
}

void loop()
{
  // Nothing to do during loop
}


int getRandomAddress(int size_uint8_ts) {
  return random(0, E2END - size_uint8_ts);
}


template <typename T> bool verifyWriteAndRead(const T input) {
  int address = getRandomAddress(sizeof(T));
  EEPROMwl->put(address, input);
  T output;
  EEPROMwl->get(address, output);

  // Validate the results
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
  EEPROMwl->put(address, input, input_length);
  T output[input_length];
  EEPROMwl->get(address, output, input_length);

  // Validate the results
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
    return verifyWriteAndRead(input);
}

// Test reading and updating double to EEPROM
bool updateAndReadDouble() {
    double input  = 1000002.50;
    return verifyWriteAndRead(input);
}

// Test reading and updating a string (char array) to EEPROM
bool writeAndReadCharArray() {
    char input[] = "Arduino";
    return (verifyWriteAndReadBlock(input, 7) >= 0);
}

bool writeAndReadIntArray() {
    int itemsInArray = 7;
    int32_t initial[] = {1, 0, 4, 0, 16, 0 , 64};
    int blockAddress = verifyWriteAndReadBlock(initial, itemsInArray);
    if (blockAddress < 0)
      return false;

    int32_t input[]   = {1, 2, 4, 8, 16, 32, 64};
    int32_t output[itemsInArray];
    EEPROMwl->update(blockAddress, input, itemsInArray);
    EEPROMwl->get(blockAddress, output, itemsInArray);

    // Validate the results
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
  EEPROMwl->put(address, family_member);
  PERSON output;
  EEPROMwl->get(address, output);

  // Validate the results
  if (output.age != family_member.age || strcmp(output.name, family_member.name) != 0) {
    Serial.print("output: ");
    Serial.print(output.age);
    Serial.print(output.name);
    return false;
  }
  Serial.println("OK");
  return true;
}


struct WEARTEST {
  uint8_t one;
  uint16_t two;
  int three;
};


int testwlWrite(wear_profile& profile) {
  WEARTEST test_struct;
  test_struct.one = 1;
  test_struct.two = 2;
  test_struct.three = 3;

  // Test checkSum method
  if (int(EEPROMwl->checkSum_(test_struct)) != 6) {
    Serial.print("Error: expected checksum == 6, got ");
    Serial.println(int(EEPROMwl->checkSum_(test_struct)));
    return -1;
  }

  // Write the profile and data to memory
  int key_address1 = EEPROMwl->wlWrite(profile, test_struct);
  // Write again; validate that the key was written to the correct memory space
  int key_address2 = EEPROMwl->wlWrite(profile, test_struct);

  // Data size is 16 bytes, key is 8 bytes. Data should be positioned to
  // overwrite the old key.
  if (key_address1 - key_address2 != 8)
    return -2;

  Serial.println("OK");
  return key_address2;
}


int testwlRead(int expected_key_address, wear_profile& profile) {
  // Attempt to find the key (written in prior test) within the memory block
  int key_location = EEPROMwl->findWearKey_(0, profile.key);
  if (key_location != expected_key_address)
    return -1;

  // With checksum validation
  key_location = EEPROMwl->findWearLevelledData_(profile);
  if (key_location != expected_key_address)
    return -2;

  WEARTEST output;
  int bytes_read = EEPROMwl->wlRead(profile, output);
  if (bytes_read != sizeof(output))
    return -3;

  if (output.one != 1 || output.two != 2 || output.three != 3)
    return -4;

  Serial.println("OK");
  return bytes_read;
}

void makeRandomKey(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}