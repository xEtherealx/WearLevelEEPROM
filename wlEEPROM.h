// WearLevelEEPROM.h - EEPROM library enhanced with wear leveling
// Author: Russell Brennan
//
// An EEPROM interface optimized for repeatedly writing struct-type blocks
// of data.  Via wear level, successive writes of data blocks will be placed
// will be placed throughout the EEPROM memory space.  Also, only changed
// bytes are written to minimize writes.  The scheme used here is not optimal,
// but is quite sufficient.
//
// Example usage:
//
// ...
// TODO:
// - set up a wear level block in which normal writes won't occur
//
#ifndef WLEEPROM_h
#define WLEEPROM_h

#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

// Size of buffer to use when finding current buffer, in bytes
#define WEAR_KEY_SEARCH_SIZE 128
#define WEAR_KEY_LENGTH 8


struct wear_profile {
  char key[WEAR_KEY_LENGTH];
  unsigned char checksum;
  uint16_t data_size_bytes; // Size of data block, in bytes
};


class wlEEPROM: public EEPROMClass {
public:
  using EEPROMClass::get;
  using EEPROMClass::put;
  // By default we use the entire memory range.
  wlEEPROM() :
    memory_space_start_(0),
    memory_space_end_(E2END),
    prior_key_location_(0) {
      setMemoryPool(memory_space_start_, E2END);
  }

  //STL and C++11 iteration capability.                                        
  EEPtr begin() { return memory_space_start_; }                        
  //Standard requires this to be the item after the last valid entry.
  EEPtr end() { return memory_space_end_; }
  uint16_t length() { return end() - begin(); } 


  // Sets the starting position and memory size that we manage.
  //
  // Args:
  //   memory_first_byte: int starting memory byte
  //   memory_size: optional int memory size, defaults up to E2END
  // Returns:
  //   bool indicating success (true) or failure (false)
  bool setMemoryPool(int memory_first_byte, int memory_size);


  // Reads the first valid data block matching the provided profile key,
  // with data having a valid checksum.
  //
  // Args:
  //   profile: a wear_profile for data to be read, containing the target key.
  //   data: reference to data to be written to, upon read.
  // Returns:
  //   int size of *valid* profile data read, -1 if no matching profile was
  //       found.
  template <class T> int wlRead(wear_profile& profile, T& data) {
      // Find the current wear level location, if any
      int key_address = findWearLevelledData_(profile);
      if (key_address < 0)
        return -1;

      get(key_address + sizeof(wear_profile), data);
      return profile.data_size_bytes;
  }

  // Writes data and a wear profile header to memory space preceding the current
  //    data block with matching wear key.  The current block's key will be
  //    invalidated.
  //
  // TODO: avoid writing over 'active' wear blocks via multiple keys
  //
  // Args:
  //   profile: a wear_profile for data to be read, containing the target key.
  //   data: reference to data that is to be written.
  // Returns:
  //   -1 on failure, address of key on success
  template <class T> int wlWrite(
    wear_profile& profile, const T& data) {
      int required_space = sizeof(profile) + sizeof(data);
      if (this->length() < required_space)
        return -1;

      // Find the current wear level location, create one later if not found
      int key_address = findWearLevelledData_(profile);

      // Determine new key address, overwriting the current if it exists
      if (key_address >= 0) {
        key_address -= (required_space - WEAR_KEY_LENGTH);
      } else {
        // Random new location
        srand(millis()); // Good enough
        key_address = rand();
        // TODO: accept alternate keys to support multiple wear blocks
      }

      // Ensure that the new key falls within feasible write bounds
      int range = this->length() - required_space;
      key_address = key_address % range + this->begin();

      // Write the new wear profile, and then the data. Old wear key is
      // overwritten with last bytes of data.
      profile.data_size_bytes = sizeof(data);
      profile.checksum = checkSum_(data);
      put(key_address, profile);
      put(key_address + sizeof(profile), data);
      return key_address;
  }



  // Reads a single bit.
  bool getBit(const int address, const int bit_number);
  // Writes a single bit, returns false only if bit_number is out of range
  bool putBit(const int address, const int bit_number, const bool  value);
  bool updateBit(const int address, const int bit_number, const bool  value);


  // Extensions of EEPROM to handle arrays of data
  // 'get' and 'put' arrays of objects to and from EEPROM.
  template <class T> T* get(int idx, T* data_ptr, const size_t data_len){
    for (int element=0; element < data_len; ++element) {
      get(idx + sizeof(T)*element, *(data_ptr + element));
    }
    return data_ptr;
  }
  
  void write(int idx, uint8_t val) {
    update(idx, val);
  }

  template <class T> const T* put(
      int idx, const T* data_ptr, const size_t data_len) {
    return update(idx, data_ptr, data_len);
  }
 
  template <class T> const T& update(int idx, const T& data){ 
    return put(idx, data);
  }     

  template <class T> const T* update(
      int idx, const T* data_ptr, const size_t data_len) {
    for (int element=0; element < data_len; ++element) {
      update(idx + sizeof(T)*element, *(data_ptr + element));
    }
    return data_ptr;
  }

  // Checks whether EEPROM is ready to be accessed.
  bool isReady();


  // "Private" variables
  EEPtr memory_space_start_;  // First byte of memory space
  EEPtr memory_space_end_;    // Last byte + 1 of memory space
  EEPtr prior_key_location_;  // Previously found key location  

  // Finds a substring, ignoring null characters.
  // Args:
  //   haystack: pointer to search buffer
  //   needle: pointer to 8 character key to find
  //   length: size of haystack buffer
  // Returns:
  //   pointer to the found substring or NULL
  char* substring_(char *haystack, const char *needle, size_t length);
  

  // Locates the first valid wear level profile and data block matching the
  // provided profile key, and with valid data.
  //
  // Args:
  //   profile, a wear_profile to be written to containing the target key.
  // Returns:
  //   Integer address of the found key, or -1 on failure. The provided
  //   wear_profile will be filled out.
  int findWearLevelledData_(wear_profile& profile);


  // Locates the first instance of the wear key in memory.
  // Args:
  //   mem_start: starting memory location for search
  //   key: char[] pointer to array containing target key
  // Returns:
  //  int memory address of start of key, or an out-of-bounds memory location
  //      if the key was not found.
  int findWearKey_(int mem_start, const char key[WEAR_KEY_LENGTH]);


  // Calculates a checksum of a block of data
  template <class T> uint8_t checkSum_(const T& data) {
    return checkSum_((const uint8_t*)&data, sizeof(data));
  }
  template <class T> uint8_t checkSum_(
      const T* data, const size_t data_length_bytes) {
    const uint8_t* block;
    uint8_t checksum = 0;
    block = (const uint8_t*)data;
    for (int byte = 0; byte < data_length_bytes; ++byte)
      checksum += *block++;
    return checksum;
  }
};

#endif // WLEEPROM_h

