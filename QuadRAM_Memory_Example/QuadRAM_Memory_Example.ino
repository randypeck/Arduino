// ***** QUADRAM EXTERNAL SRAM MODULE ***
// 11/27/20: Testing the locations of variables before and after changing the location of the heap.
// For QuadRAM: Define external SRAM start and end.  Total size = 56,831 bytes.
#define XMEM_START   ( (void *) 0x2200 )   // Start of QuadRAM External SRAM memory space = dec 8,704.
#define XMEM_END     ( (void *) 0xFFFF )   // End of QuadRAM External SRAM memory space = dec 65,535.

unsigned int freeMemory () {  // Excellent little utility that returns the amount of unused SRAM.
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


// Arduino memory:
// 0x8FF: Top of SRAM
// Stack grows down
//                 <- SP
// (free memory)
//                 <- *__brkval
// Heap (relocated)
//                 <- &__bss_end
// .bss  = not-yet-initialized global variables
// .data = initialized global variables
// 0x0100: Bottom of SRAM
// SP will be replaced with: *((uint16_t volatile *) (0x3D))  // Bottom of stack (grows down)
extern int __heap_start;
extern void *__brkval;          // Top of heap; should match __bss_end after moving heap.
extern unsigned int __bss_end;  // Lowest available address above globals

// All variables (int, pointers, etc.) defined before setup() will be placed in the Arduino's GLOBAL memory space,
// also known as STATIC data, which seems to occupy the bottom of memory up until at least 3EC or higher, depending
// on how much global data exists.  Fixed length since the compiler knows at compile time exactly how many globals
// will be used.

// Just above the fixed STATIC data part of memory, is the HEAP, which grows upward towards the (down-coming) STACK.
// The HEAP stores dynamically assigned data such as used with malloc().

// Finally, data that comes into existence inside of functions (including setup() and loop()) starts at the top of
// known memory, just below 0x2200, and grows DOWN towards the heap.

// If we move the location of the heap up to 0x2200, as we can do with QuadRAM, then the stack is able to grow even
// farther down.  So whatever the heap would previously require is now availble for the stack.
// And since we have moved the heap from Arduino internal SRAM to external memory of about 56K, our heap is huge.
// Finally, since the heap is gone from internal memory, our STATIC (GLOBAL) DATA area can also be larger.
// i.e. STACK and STATIC (GLOBAL) DATA combined, can be increased by whatever amount of space would otherwise
// have been used by the heap.  Probably not much in our normal apps, because we try not to use the heap.
// However, now that we know we can malloc() large arrays and structs in the (new) heap, whatever data we now
// put there will no longer be taking up space in the STACK or GLOBAL (STATIC) DATA areas.

// OTHER IMPORTANT TAKEAWAY FROM THIS CODE:
// Variables defined above setup() will be stored in the STATIC/GLOBAL part of internal memory, located near the
// bottom, regardless of where the heap is located.  Thus, moving the heap has no bearing on where these global
// variables are stored.
// Similarly, ordinary variables defined within setup() or any other function will be stored on the STACK, starting
// at the top of internal memory (0x2200) and growing down, regardless of where the heap is located.
// So it makes NO DIFFERENCE if we define ordinary variables above setup(), or even inside of setup(), before or
// after we change the location of the heap.
// We WOULD find trouble if we were to allocate space in the heap such as by using malloc(), new() or however else
// you create space in the heap.

int* beforeSetupPtr;  // Declare a global pointer
int  beforeSetupGlobal;

void setup() {

  Serial.begin(115000);

  Serial.println(F("SRAM area borders before relocate heap ============================="));
  Serial.print(F(".bss end (top of global data) 2937 and above   = ")); Serial.println((int)&__bss_end);
  Serial.print(F("__brkval (top of heap)          = ")); Serial.println((int)__brkval);
  Serial.print(F("Stack pointer (bottom of stack) 8700 and below = ")); Serial.println((int)SP);
  Serial.print(F("Free memory                     = ")); Serial.println(SP - (int)&__bss_end);
  

  
  // All variables (pointers or otherwise) declared within Setup() will be placed on the STACK.
  // This starts just below 0x2200 and growns DOWN toards zero.
  // This is regardless of where the heap is located.
  int* inSetupBeforeMallocPtr;  // Declare a Setup pointer before malloc
  int  inSetupBeforeMallocGlobal;

  
  Serial.println(); Serial.println("*** START ***");
  Serial.print(F("__malloc_heap_start before           : ")); Serial.println(uint16_t(__malloc_heap_start), HEX);
  Serial.print(F("__malloc_heap_end   before           : ")); Serial.println(uint16_t(__malloc_heap_end), HEX);
  // Note that __malloc_heap_end == 0 just means that the top of the heap has not yet been defined.
  // This is what you would normally expect at the beginning of a non-QuadRAM program.
  
  // *** QUADRAM EXTERNAL SRAM MODULE ***
  // QuadRAM: Enable external memory with no wait states.
  // Set SRE bit in the XMCRA (External Memory Control Register A):
  XMCRA = _BV(SRE);
  // QuadRAM: Disable high memory masking (don't need >56kbytes in this app):
  XMCRB = 0;
  // QuadRAM requires four pinMode() and digitalWrite() statements.
  // QuadRAM: Enable RAM device (PD7 or D38, active low)
  pinMode(38, OUTPUT); digitalWrite(38, LOW);
  // QuadRAM: Enable bank select outputs and select bank 0. Don't need multiple banks in this app.
  pinMode(42, OUTPUT); digitalWrite(42, LOW);   // PL7 or D42
  pinMode(43, OUTPUT); digitalWrite(43, LOW);   // PL6 or D43
  pinMode(44, OUTPUT); digitalWrite(44, LOW);   // PL5 or D44
  // QuadRAM: Relocate the heap to external SRAM.
  // This is not required, but frees more of the 8K SRAM so definitely do this.
  __malloc_heap_start =  (char *) ( XMEM_START );   // Reallocate start of heap area = 0x2200 = dec 8,704.
  __malloc_heap_end   =  (char *) ( XMEM_END );     // Reallocate end of heap area = 0xFFFF = dec 65,535.

  char* buffer = malloc(1000);
  *buffer = ";lkajsdf;lkjasd;lfkjasd;lfkjasd;flkjasd;lkjasdf;lkjasd;lkjasdf;lkjasdf;lkjasd;lkjasdfj";

  int* inSetupAfterMallocPtr;  // Declare a Setup pointer before malloc
  int  inSetupAfterMallocGlobal;

  Serial.println(F("SRAM area borders after relocate heap ============================="));
  Serial.print(F(".bss end (top of global data) 2937 and above   = ")); Serial.println((int)&__bss_end);
  Serial.print(F("__brkval (top of heap)          = ")); Serial.println((int)__brkval);
  Serial.print(F("Stack pointer (bottom of stack) 8700 and below = ")); Serial.println((int)SP);
  Serial.print(F("Free memory                     = ")); Serial.println(SP - (int)&__bss_end);


  Serial.println(F("Defined QuadRAM and changed location of heap..."));
  Serial.print(F("__malloc_heap_start after            : ")); Serial.println(uint16_t(__malloc_heap_start));
  Serial.print(F("__malloc_heap_end   after            : ")); Serial.println(uint16_t(__malloc_heap_end));
  Serial.print(F("location of beforeSetupPtr           : ")); Serial.println(uint16_t(&beforeSetupPtr));
  Serial.print(F("location of beforeSetupGlobal        : ")); Serial.println(uint16_t(&beforeSetupGlobal));
  Serial.print(F("location of inSetupBeforeMallocPtr   : ")); Serial.println(uint16_t(&inSetupBeforeMallocPtr));
  Serial.print(F("location of inSetupBeforeMallocGlobal: ")); Serial.println(uint16_t(&inSetupBeforeMallocGlobal));
  Serial.print(F("location of inSetupAfterMallocPtr    : ")); Serial.println(uint16_t(&inSetupAfterMallocPtr));
  Serial.print(F("location of inSetupAfterMallocGlobal : ")); Serial.println(uint16_t(&inSetupAfterMallocGlobal));

}

void loop() { }
