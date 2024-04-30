// Rev: 11/23/20.
// HERE IS CODE THAT FREES MORE OF OUR 8K SRAM JUST BY RELOCATING THE HEAP INTO XMEM+/QuadRAM.
// AND WE CAN GET AN EXTRA 56K OF SRAM FOR ARRAYS/STRUCTS BY PUTTING THEM IN HEAP SPACE.
// Below is a program to test the functionality of EITHER the XMEM+ 32K, or QuadRAM 56K.
// QuadRAM is cheaper than XMEM+, and hangs off the back of the Arduino, as opposed to XMEM+,
// which stacks on top of the Mega and completely covers it.
// QuadRAM is smaller and cheaper and simpler and has 56K versus XMEM+'s 32K, so use QuadRAM.
// We can access the extra RAM without changing the start of the heap area, but by doing so,
// we increase the available stack space because it moves the heap out of the 8KB SRAM space.
// IMPORTANT: Writing to the Serial monitor at 115200 baud causes the program to hang, for unknown reasons.
// General notes per Randy and emails with Mark at XMEM+ (QuadRAM tech support is terrible):
// The modules need to be accessed as shown below, as heap space, not as simply a contiguous amount of SRAM.
// So we should (but not required) re-define the start and end addresses of the heap, and then use malloc()
// to allocate the memory and assign a pointer to it.
// But once you've done that, you access the array just like a statically defined array.  See below.

// For access to more than 32K/56K additional SRAM, use bank switching per the documentation.
// XMEM+ offers 16 banks of 32K each.
// QuadRAM offers 8 banks of 56K each.

// If we run short of 8K SRAM, this will be a good place to store long buffers, such as the Route Element
// buffer, and also any other large structs and/or large arrays.
// The advantage over FRAM is that we can access elements of struct arrays just like ordinary arrays;
// i.e. by using the subscript i.e. locoRef[trainNum], as opposed to FRAM where we need a class
// function to calculate the address of the element(record) and then do a read and pass back a pointer.

// Here is the general form to request an array in dynamic memory heap:
// float *  fa  = (float *) malloc( arraySize * sizeof(float));

// Note that we could use "new" instead of "malloc", but the compiler throws an error if it thinks we're asking for too much RAM.
// i.e. "A_MAS:20:50: error: size of array is too large"

#define XMEM_START   ( (void *) 0x2200 )   // Start of XMEM+ *and* QuadRAM External SRAM memory space, dec 8704.

// If XMEM+, define top of memory as 0xA1FF
// #define XMEM_END     ( (void *) 0xA1FF )   // End of XMEM+ External SRAM memory space, dec 41,471.
                                           // Total size of XMEM+ External SRAM = 32,767 bytes.
// If QuadRAM, define top of memory as 0xFFFF
#define XMEM_END     ( (void *) 0xFFFF )   // End of QuadRAM External SRAM memory space, dec 65,535.
                                           // Total size of QuadRAM Ext SRAM = 56,831 bytes.
#define arraySize 14207 // XMEM+: If you change this to 8192, it exceeds the 32,767 bytes of heap and crashes
                       // QuadRAM: If you exceed 14207, it exceeds the 56,831 bytes of heap and crashes

void setup() {

  // QuadRAM: Enable external memory with no wait states.
  // Set SRE bit in the XMCRA (External Memory Control Register A):
  XMCRA = _BV(SRE);
  // QuadRAM: Disable high memory masking (don't need >56kbytes in this app):
  XMCRB = 0;

  // XMEM+ Commands are virtually identical:
  // Setup for using External Memeory and Expansion Bus
  //XMCRA  =  0x80;  // Set SRE bit, Enable xmem+, No wait states. If Wait-states are needed change this value accordingly.
  //XMCRB  =  0x00;  // All of all PORTC pins act as upper address lines, A[15:8], disable Bus keeper function .

  // QuadRAM requires four pinMode() and digitalWrite() statements; XMEM+ does not require these.
  // QuadRAM: Enable RAM device (PD7 or D38, active low)
  pinMode(38, OUTPUT); digitalWrite(38, LOW);
  // QuadRAM: Enable bank select outputs and select bank 0. Don't need multiple banks in this app.
  pinMode(42, OUTPUT); digitalWrite(42, LOW);   // PL7 or D42
  pinMode(43, OUTPUT); digitalWrite(43, LOW);   // PL6 or D43
  pinMode(44, OUTPUT); digitalWrite(44, LOW);   // PL5 or D44

  Serial.begin(115200);      // Reduce baud rate to 9600, higher baud rates can be an problem with Arduino sometimes
  
  // Relocate the heap to external SRAM.  This is not required, but frees more of the 8K SRAM so definitely do this.
  __malloc_heap_start =  (char *) ( XMEM_START );   // Reallocate start of heap area
  __malloc_heap_end   =  (char *) ( XMEM_END );     // Reallocate end of heap area

  // malloc: pointer_name = (cast-type*) malloc(size);
  // float *  fa  = (float *) malloc( arraySize * sizeof(float));  // Request an array in dynamic memory heap
  // new: pointer_variable = new datatype;
  // float * fa  = new float[arraySize * sizeof(float)];
  float *  fa  = (float *) malloc( arraySize * sizeof(float));  // Request an array in dynamic memory heap
  int i;
  Serial.println("Hello world.");
  for (int i = 0; i < arraySize; i++) {
    fa[i] = (3.1415927 * i);
  }
  // See if a correct value get stored, should be 1128 * 3.1415927 = 3543.72
  // 13000 should be 40840.705
  Serial.println(fa[14101]);   
  Serial.println("Hello again.");  // This just tells us if we've crashed or not.
}

void loop() { }
