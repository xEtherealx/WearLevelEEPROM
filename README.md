## **WearLevelEEPROM Library** for Arduino

**Written by:** _Russell Brennan_.  

### **What is the wlEEPROM library.**

The wlEEPROM library provides an easy to use interface to EEPROM storage with
optional support for wear-leveling and memory space definition.  In addition,
the get and put methods are extened to facilitate operation on arrays.
It is built on top of the EEPROM library.  This library will work on many AVR
devices like ATtiny and ATmega chips.

### **How to use it**
You must first make this library visible to your sketch, and you can find
direction on doing so [here](https://www.arduino.cc/en/Hacking/Libraries).
To add its functionality to your sketch you'll need to reference the library
header file. You do this by adding "#include <wlEEPROM.h>" to the top of your
sketch.

```Arduino
#include <wlEEPROM.h>

void setup(){

}

void loop(){

}

```

Note that the EEPROM library unfortunately defines a global EEPROM instance;
you must ignore this and create your own wlEEPROM instance.

You can view examples of how to use this library [here](examples/).

### **Library functions**

#### **`EEPROM.read( address )`** [[_example_]](examples/eeprom_read/eeprom_read.ino)

This function allows you to read a single byte of data from the eeprom.
Its only parameter is an `int` which should be set to the address you wish to read.

The function returns an `unsigned char` containing the value read.

#### **`EEPROM.write( address, value )`** [[_example_]](examples/eeprom_write/eeprom_write.ino)

The `write()` method allows you to write a single byte of data to the EEPROM.
Two parameters are needed. The first is an `int` containing the address that is
to be written, and the second is a the data to be written (`unsigned char`).

This function does not return any value.

#### **`EEPROM.update( address, value )`** [[_example_]](examples/eeprom_update/eeprom_update.ino)

This function is similar to `EEPROM.write()` however this method will only write
data if the cell contents pointed to by `address` is different to `value`.
This method can help prevent unnecessary wear on the EEPROM cells.

This function does not return any value.

#### **`EEPROM.get( address, object )`** [[_example_]](examples/eeprom_get/eeprom_get.ino)

This function will retrieve any object from the EEPROM.
Two parameters are needed to call this function. The first is an `int`
containing the address that is to be written, and the second is the object
you would like to read.

This function returns a reference to the `object` passed in.
It does not need to be used and is only returned for conveience.

#### **`EEPROM.get( address, object[], length )`** [[_example_]](examples/eeprom_get/eeprom_get.ino)

This function will retrieve an array of size `length` of any type from the EEPROM.

This function returns a pointer to the `object[]` passed in.
It does not need to be used and is only returned for conveience.

#### **`EEPROM.put( address, object )`** [[_example_]](examples/eeprom_put/eeprom_put.ino)

This function will write any object to the EEPROM.
Two parameters are needed to call this function. The first is an `int`
containing the address that is to be written, and the second is the object you
would like to write.

This function uses the _update_ method to write its data, and therefore only
rewrites changed cells.

This function returns a reference to the `object` passed in. It does not need
to be used and is only returned for conveience.

#### **`EEPROM.put( address, object[], length )`** [[_example_]](examples/eeprom_put/eeprom_put.ino)

This function will write an array of size `length` of any type to the EEPROM.

This function uses the _update_ method to write its data, and therefore only
rewrites changed cells.

This function returns a reference to the `object` passed in. It does not need
to be used and is only returned for conveience.

#### **Subscript operator: `EEPROM[address]`** [[_example_]](examples/eeprom_crc/eeprom_crc.ino)

This operator allows using the identifier `EEPROM` like an array.  
EEPROM cells can be read _and_ **_written_** directly using this method.

This operator returns a reference to the EEPROM cell.

```c++
unsigned char val;

//Read first EEPROM cell.
val = EEPROMwl[ 0 ];

//Write first EEPROM cell.
EEPROM[ 0 ] = val;

//Compare contents
if( val == EEPROM[ 0 ] ){
  //Do something...
}
```

#### **`EEPROM.length()`**

This function returns an `unsigned int` containing the number of cells in the
EEPROM.

---

### **Advanced features**

This library uses a component based approach to provide its functionality.
This means you can also use these components to design a customized approach.
Two background classes are available for use: `EERef` & `EEPtr`.

#### **`EERef` class**

This object references an EEPROM cell.
Its purpose is to mimic a typical byte of RAM, however its storage is the EEPROM.
This class has an overhead of two bytes, similar to storing a pointer to an EEPROM cell.

```C++
EERef ref = EEPROM[ 10 ]; //Create a reference to 11th cell.

ref = 4; //write to EEPROM cell.

unsigned char val = ref; //Read referenced cell.
```

#### **`EEPtr` class**

This object is a bidirectional pointer to EEPROM cells represented by `EERef` objects.
Just like a normal pointer type, this type can be dereferenced and repositioned using 
increment/decrement operators.

```C++
EEPtr ptr = 10; //Create a pointer to 11th cell.

*ptr = 4; //dereference and write to EEPROM cell.

unsigned char val = *ptr; //dereference and read.

ptr++; //Move to next EEPROM cell.
```

#### **`EEPROM.begin()`**

This function returns an `EEPtr` pointing to the first cell in the EEPROM.  
This is useful for STL objects, custom iteration and C++11 style ranged for loops.

#### **`EEPROM.end()`**

This function returns an `EEPtr` pointing at the location after the last EEPROM cell.  
Used with `begin()` to provide custom iteration.

**Note:** The `EEPtr` returned is invalid as it is out of range. 
In fact the hardware causes wrapping of the address (overflow) and 
`EEPROM.end()` actually references the first EEPROM cell.
