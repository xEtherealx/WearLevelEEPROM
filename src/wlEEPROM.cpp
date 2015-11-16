// WearLevelEEPROM - EEPROM library enhanced with wear leveling
// Author: Russell Brennan
//
#include "wlEEPROM.h"


bool wlEEPROM::isReady() {
  return eeprom_is_ready();
}


bool wlEEPROM::setMemoryPool(int memory_first_byte, int memory_size=0) {
  // If no memory_size was provided, use max memory
  if (memory_size <= 0)
    memory_size = E2END - memory_first_byte;

  memory_space_start_ = EEPtr(memory_first_byte);
  memory_space_end_ = EEPtr(memory_first_byte + memory_size);
  return true;
}


bool wlEEPROM::getBit(const int address, const int bit_number) {
  if (bit_number > 7) return false;
  uint8_t byteVal =  read(address);
  return (byteVal & (1 << bit_number));
}


bool wlEEPROM::putBit(const int address, const int bit_number, const bool value) {
  return updateBit(address, bit_number, value);
}


bool wlEEPROM::updateBit(const int address, const int bit_number, const bool value) {
  if (bit_number> 7) return false;

  byte byteValInput  = read(address);
  byte byteValOutput = byteValInput;
  // Set bit_number
  if (value) {
    byteValOutput |= (1 << bit_number);  //Set bit_number to 1
  } else {
    byteValOutput &= ~(1 << bit_number); //Set bit_number to 0
  }
  // Store if different from input
  if (byteValOutput != byteValInput) {
    write(address, byteValOutput);
  }
  return true;
}


int wlEEPROM::findWearLevelledData_(wear_profile& profile) {
  // Start with a previously found key address, if any
  int search_start = min(this->end(), max(prior_key_location_, this->begin()));
  int key_address = findWearKey_(search_start, profile.key);

  // Find a wear key and validate the data
  while(key_address < this->end()) {
    // Read the entire wear profile from memory
    get(key_address, profile);

    // Read the data
    uint8_t data[profile.data_size_bytes];
    get(key_address + sizeof(profile), data, profile.data_size_bytes);

    // If the checksum matches, we've found our data
    if (profile.checksum == checkSum_(data, profile.data_size_bytes)) {
      return key_address;
    }

    // Didn't check out -- find the next key, if any
    key_address = findWearKey_(++key_address, profile.key);
  }
  // No dice
  return -1;
}


int wlEEPROM::findWearKey_(int mem_start, const char key[WEAR_KEY_LENGTH]) {
  char search_buffer[WEAR_KEY_SEARCH_SIZE];

  // Search over memory space, reading in chunks
  int mem_end = this->end() - WEAR_KEY_LENGTH;
  int mem_increment = WEAR_KEY_SEARCH_SIZE - WEAR_KEY_LENGTH;
  for (int address = mem_start; address <= mem_end; address += mem_increment) {
    int read_bytes = min(WEAR_KEY_SEARCH_SIZE, this->end() - address);
    get(address, search_buffer, read_bytes);

    // Find location of wear key within buffer, or not
    char *key_location = substring_(search_buffer, key, read_bytes);
    if (key_location != NULL) {
      // Determine location wrt address and return it
      return address + (key_location - search_buffer);
    }
  }
  return this->end();
}


char* wlEEPROM::substring_(char *haystack, const char *needle, size_t length) {
  for (int i=0; i <= length - WEAR_KEY_LENGTH; ++i) {
    if (strncmp(&haystack[i], needle, WEAR_KEY_LENGTH) == 0)
      return &haystack[i];
  }
  return NULL;
}
