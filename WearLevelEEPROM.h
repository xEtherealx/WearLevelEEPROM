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
#include <cstdint>

#define DEBUG_EEPROM // Enables logging of maximum of writes and out-of-memory

// Size of buffer to use when finding current buffer, in bytes
#define WEAR_KEY_SEARCH_SIZE 128
#define WEAR_KEY_LENGTH 8


struct wear_profile {
  char key[WEAR_KEY_LENGTH];
  unsigned char checksum;
  uint16_t data_size_bytes; // Size of data block, in bytes
};


class WLEEPROM: Public EEPROMClass {
public:
  // By default we use the entire memory range.
  EEPROMex2() :
    memory_space_first_byte_(0),
    memory_space_last_byte_(E2END) {
      setMemoryPool(memory_space_first_byte_, E2END);
  }


  // Sets the starting position and memory size that we manage.
  //
  // Args:
  //   memory_first_byte: int starting memory byte
  //   memory_size: optional int memory size, defaults up to E2END
  // Returns:
  //   bool indicating success (true) or failure (false)
  bool setMemoryPool(int memory_first_byte, int memory_size);


  // Reads the first valid data block matching the provided profile key,
  // with valid data.
  //
  // Args:
  //   profile: a wear_profile for data to be read, containing the target key.
  //   data: reference to data to be written to, upon read.
  // Returns:
  //   int size of *valid* profile data read, -1 if no matching profile was
  //       found.
  template <class T> int readWearLevelledData(
    const wear_profile& profile, T& data) {
      // Find the current wear level location, if any
      int key_address = findWearLevelledData(profile);
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
  //   -1 on failure, 1 on success
  template <class T> int writeWearLevelledData(
    wear_profile& profile, const T& data) {
      // Find the current wear level location, create one later if not found
      int key_address = findWearLevelledData(profile);

      // Determin new key address, overwriting the current if it exists
      if (key_address >= 0) {
        key_address -= sizeof(wear_profile) + sizeof(data) - WEAR_KEY_LENGTH;
      } else {
        // Randomize this new location within the memory space
        srand(time(NULL)); // Good enough
        int range =
            memory_space_last_byte_ - memory_space_first_byte_ - sizeof(data);
        key_address = rand() % range + memory_space_first_byte_;
      }
      profile.data_size_bytes = sizeof(data);
      profile.checksum = checkSum(data);

      // Write the new wear profile, and then the data. Old wear key is
      // overwritten with last bytes of data.
      put(key_address, profile);
      put(key_address + sizeof(profile), data);
      return 1;
  }


  // Locates the first valid wear level profile and data block matching the
  // provided profile key, and with valid data.
  //
  // Args:
  //   profile, a wear_profile to be written to containing the target key.
  // Returns:
  //   Integer address of the found key, or -1 on failure. The provided
  //   wear_profile will be filled out.
  int findWearLevelledData(wear_profile& profile) {
    int key_address = memory_space_first_byte_;

    // Find a wear key and validate the data
    while(key_address < memory_space_last_byte_) {
      key_address = findWearKey(key_address, profile.key);

      // Read the entire wear profile
      get(key_address, profile);

      // Read the data
      uint8_t data[profile.data_size_bytes];
      get(key_address + sizeof(profile), data, profile.data_size_bytes);

      // If the checksum matches, we've found our data
      if (profile.checksum == checkSum(data, profile.data_size_bytes)) {
        return key_address;
      }

      ++key_address;
    }
    // No dice
    return -1;
  }

  // Locates the first instance of the wear key in memory.
  // Args:
  //   mem_start: starting memory location for search
  // Returns:
  //  int memory address of start of key, or -1 if not found
  int findWearKey(int mem_start, const char key[WEAR_KEY_LENGTH]);


  // Calculates a checksum of a block of data
  template <class T> uint8_t checkSum(const T& data) {
    return checkSum((const uint8_t*)&data, sizeof(data));
  }
  template <class T> uint8_t checkSum(
      const T* data, const size_t data_length_bytes) {
    const uint8_t* block;
    uint8_t checksum = 0;
    block = (const uint8_t*)data;
    for (int byte = 0; byte < data_length_bytes; ++byte)
      checksum += *block++;
    return checksum;
  }


  // 'get' and 'put' arrays of objects to and from EEPROM.
  template <class T> T& get(int idx, T& t, const size_t data_len){
    for (int element=0; element < data_len; ++element) {
      get(idx + element, (uint8_t*)&t + element)
    }
    return t;
  }

  template <class T> const T& put(int idx, const T& t, const size_t data_len) {
    for (int element=0; element < data_len; ++element) {
      put(idx + element, (const uint8_t*)&t + element)
    }
    return t;
  }


  // Checks whether EEPROM is ready to be accessed.
  bool isReady();

private:
  //Private variables
  int memory_space_first_byte_;         // First byte of memory space
  int memory_space_last_byte_;          // Last byte of memory space
  char* substring(char *haystack, char *needle, size_t length);

}





class EEPROMex2 {


  // Reads a single bit.
  bool readBit(const int address, int bit);

  // Reads any type of variable, such as structs.
  // Returns:
  //   number of bytes read.
  template <class T> int readBlock(const int address, const T& value) {
    eeprom_read_block((void*)&value, (const void*)address, sizeof(value));
    return sizeof(value);
  }

  // Writes a single bit.
  bool writeBit(int , uint8_t, bool);

  // Writes any type of variable, such as structs.
  // Args:
  //   address: int beginning address of data.
  //   value: variable to be written into.
  template <class T> int writeBlock(int address, const T& value) {
    if (!isWriteOk(address + sizeof(value))) return 0;
    eeprom_write_block((void*)&value, (void*)address, sizeof(value));
    return sizeof(value);
  }


private:
  //Private variables
  int memory_space_first_byte_;         // First byte of memory space
  int memory_space_last_byte_;          // Last byte of memory space
  int next_available_address_;
  int wear_block_first_byte_;
  int wear_block_last_byte_;
  wear_profile profile;
  int num_writes_allowed_;
  char* substring(char *haystack, char *needle, size_t length);

};

#endif // WLEEPROM_h

