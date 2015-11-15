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

  memory_space_first_byte_ = memory_first_byte;
  memory_space_last_byte_ = memory_space_first_byte_ + memory_size - 1;
  return true;
}


bool wlEEPROM::getBit(const int address, const int bit_number) {
  if (bit_number > 7) return false;
  uint8_t byteVal =  eeprom_read_byte((unsigned char *) address);
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


int wlEEPROM::findWearKey_(int mem_start, char key[WEAR_KEY_LENGTH]) {
  char search_buffer[WEAR_KEY_SEARCH_SIZE];

  // Search over memory space, reading in chunks
  int mem_end = memory_space_last_byte_ - WEAR_KEY_LENGTH;
  int mem_increment = WEAR_KEY_SEARCH_SIZE - WEAR_KEY_LENGTH;
  for (int address = mem_start; address <= mem_end; address += mem_increment) {
    int read_bytes = min(WEAR_KEY_SEARCH_SIZE, memory_space_last_byte_ - address);
    get(address, search_buffer, read_bytes);

    // Find location of wear key within buffer, or not
    char *key_location = substring(search_buffer, key, read_bytes);
    if (key_location != NULL) {
      // Determine location wrt address and return it
      return address + (key_location - search_buffer);
    }
  }
  return -1;
}


// Finds a substring, ignoring null characters.
char* wlEEPROM::substring(char *haystack, char *needle, size_t length) {
  size_t needle_length = strlen(needle);

  for (size_t i = 0; i < length; i++) {
    if (i + needle_length > length)
      return NULL;

    if (strncmp(&haystack[i], needle, needle_length) == 0)
      return &haystack[i];
  }
  return NULL;
}
