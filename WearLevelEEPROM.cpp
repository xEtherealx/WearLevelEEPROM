// WearLevelEEPROM - EEPROM library enhanced with wear leveling
// Author: Russell Brennan
//
#include "EEPROMex2.h"


bool EEPROMex2::isReady() {
  return eeprom_is_ready();
}


bool EEPROMex2::setMemoryPool(int memory_first_byte, int memory_size=0) {
  // memory_space_first_byte can only be adjusted if no mem has already been used
  if (memory_first_byte != memory_space_first_byte_
      && next_available_address_ != memory_space_first_byte_) {
#ifdef DEBUG_EEPROM
    Serial.println("Cannot change memory_first_byte, addresses have been issued");
#endif
    return false;
  }
  
  // If no memory_size was provided, use max memory
  if (memory_size <= 0)
    memory_size = E2END - memory_first_byte;

  // Ceiling can only be adjusted if not below an issued addresses
  if (memory_first_byte + memory_size < next_available_address_) {
#ifdef DEBUG_EEPROM
    Serial.println("Cannot change ceiling, below issued addresses");
#endif
    return false;
  }
  
  // Only modify next address if we hadn't already written to memory
  if (next_available_address_ == memory_space_first_byte_) {
    next_available_address_ = memory_first_byte;
  }
  memory_space_first_byte_ = memory_first_byte;
  memory_space_last_byte_ = memory_space_first_byte_ + memory_size - 1;
  return true;
}


void EEPROMex2::setMaxAllowedWrites(int num_allowed_writes) {
#ifdef DEBUG_EEPROM
  num_writes_allowed_ = num_allowed_writes;
#endif			
}


int EEPROMex2::getNextWriteAddress(int num_bytes_to_write){
#ifdef DEBUG_EEPROM
  if (next_available_address_ + num_bytes_to_write > memory_space_last_byte_) {
    Serial.println("Attempt to write outside of EEPROM memory");
    return -(next_available_address_ + num_bytes_to_write);
  }
#endif
  int available_address   = next_available_address_;
  next_available_address_ += num_bytes_to_write;
  return available_address;
}


uint8_t EEPROMex2::read(const int address) {
  if (!isReadOk(address+sizeof(uint8_t))) return 0;
  return eeprom_read_byte((unsigned char *) address);
}


bool EEPROMex2::readBit(const int address, int bit) {
  if (bit > 7) return false;
  if (!isReadOk(address + sizeof(uint8_t))) return false;
  byte byteVal =  eeprom_read_byte((unsigned char *) address);      
  byte bytePos = (1 << bit);
  return (byteVal & bytePos);
}


bool EEPROMex2::writeBit(int address, uint8_t bit, bool value) {
  updateBit(address, bit, value);
  return true;
}


bool EEPROMex2::updateBit(int address, uint8_t bit_number, bool value) {
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


bool EEPROMex2::isWriteOk(int address)
{
#ifdef DEBUG_EEPROM
  --num_writes_allowed_;
  if (num_writes_allowed_ <= 0) {
    Serial.println("Exceeded maximum number of writes");
    return false;
  }

  if (address > memory_space_last_byte_) {
    Serial.println("Attempt to write outside of EEPROM memory");
    return false;
  }
#endif
  return true;
}


bool EEPROMex2::isReadOk(int address)
{
#ifdef DEBUG_EEPROM    
  if (address > mmemory_space_last_byte_) {
    Serial.println("Attempt to write outside of EEPROM memory");
    return false;
  }
#endif
  return true;	
}


int EEPROMex2::findWearKey(int mem_start) {
  char search_buffer[WEAR_KEY_SEARCH_SIZE];
  
  // Search over memory space, reading in chunks
  int mem_end = memory_space_last_byte_ - WEAR_KEY_LENGTH;
  int mem_increment = WEAR_KEY_SEARCH_SIZE - WEAR_KEY_LENGTH;
  for (int address = mem_start; address <= mem_end; address += mem_increment) {
    int read_bytes = min(WEAR_KEY_SEARCH_SIZE, memory_space_last_byte_ - address);
    readBlock(address, search_buffer, read_bytes);
    
    // Find location of wear key within buffer, or not
    char *key_location = substring(search_buffer, wear_profile_.wear_level_key, read_bytes);
    if (key_location != NULL) {
      // Determine location wrt address and return it
      return address + key_location - search_buffer;
    }
  }
  return -1;
}


// Finds a substring, ignoring null characters.
char* EEPROMex2::substring(char *haystack, char *needle, size_t length) {
  size_t needle_length = strlen(needle);
  
  for (size_t i = 0; i < length; i++) {
    if (i + needle_length > length)
      return NULL;
    
    if (strncmp(&haystack[i], needle, needle_length) == 0)
      return &haystack[i];
  }
  return NULL;
}
