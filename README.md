unorthodox-arduino
==================

General purpose library with many useful things for the Arduino Leonardo: Graphical User Interface, Flash file system, Hardware drivers, Virtual memory, Binary trees, Attribute tries, and even LZW compression. All written by Jeremy Lee, specifically for the Leonardo.

PREVIEW RELEASE V0.1
====================

I'm releasing this "as is", even though many parts are still a mess and need work, because there's enough useful stuff to be worth it. But there are no examples, very little documentation, and not a lot of explanations for the ass-backwards way I seem to do some things in the name of optimization. 

~ Jeremy

USING
=====
Just copy the "Unorthdox" folder to your Arduino "libraries" directory, and #include <Unorthodox.h> in the usual way.
BUT: Please include all the standard libraries you intend to use (like SPI, EEPROM and Wire) _first_ because the Unorthodox lib will only declare dependant classes (such as EEPROMPage or SPIDevice) if the requirements are _already_ loaded.

The library is currently "AVR only" simply because of the directory structure, and I don't have SAM hardware to test on, but most of the classes should port without issue. And yes, I know it takes a long time to compile - everything is in header files rather than .cpp, for various stupid reasons.

LICENCE
=======
In plain English: If you intend to use this code for projects that are personal, non-profit or educational in nature, then you are considered an "academic peer" and have fair use rights so long as attribution is given. If you intend to resell or commercially profit from this code as part of some product or service, then I will require you to enter into a commercial licence for its use.

CONTRIBUTIONS
=============
I am not actively seeking contributors or patches at this time, though of course bug reports and feedback are always welcome. The code is not technically "open source", not yet anyway. I don't even know if other people will find it useful.


VIRTUAL MEMORY CLASSES
======================
Page
ReadPage
MemoryPage
NearProgramPage
FarProgramPage
EEPROMPage
ZeroPage
BytePage
WordPage
Cardinal

DATA STRUCTURE CLASSES
======================
PrefixTree
PrefixNodeResult
RedBlackTree
Map
MapNode

STREAMING PARSER CLASSES
========================
Cursor
BufferCursor
PageCursor
NumberCursor
PrefixCursor

FILE SYSTEM CLASSES
===================
JournalFS
TokenFS

HARDWARE DEVICE CLASSES
=======================
Device
DHT11
I2CDevice
MPU6050
SPIDevice
MAX6957
ENC28J60

RASTER DEVICE CLASSES
=====================
Raster
Raster8
Raster16
ILI9325C
ILI9325C_Pins
ILI9325C_Leo
ST7735
ST7735_SPI
RasterDraw16
RasterFont16

ROBOTICS CLASSES
================
Debounce
DroidBeeps
Motivator
BaseDroid
RasterDroid

GUI CLASSES
===========
RasterSpan
RasterPager
SignalsPager
SourcePager
CodesPager


