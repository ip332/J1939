## Summary

`J1939` repository provides a library to handle encoding/decoding of a CAN frame and tools to convert DBC files into a proprietary data format. 

## Background
There are two different tasks where we might want to read a J1939 stream:
* **message-centric**, when we know what exactly messages to read. This is how a typical ECU (engine, brakes, FLR, ACC, etc.) works: it knows how to deal with a small subset of all messages and ignores the rest. 
* **streaming**, when we want to read all messages to log them into a file or to share it with some other components in the system.

While both tasks could be addressed using the same implementation, the first one should be designed in a way to satisfy typical requirements for the automotive ECU with respect to the performance memory management.
In such case, the application code would declare variable for each message to deal with (i.e. TSC1, XBR, etc.) and ignore the rest of the CAN traffic.
In streaming mode, however, creating individual variable for each message type would be impractical so for that mode, we need a generic API and detailed information about all messages and signals stored in some data storage.

On the other side, regardless of the use case, the encoding/decoding functionality should be performed in a consistent way across all modules and subsystems.
A possible solution would be to parse a DBC file at runtime (like it was done in the roscan repository). But this method might be not acceptable for the safety critical applications. Besides, compiling DBC during the build time provides more time for validation.  

Therefore, the proposed solution is based on providing a DBC compiler which can generate output in two different formats (named **PGN** and **Union**) for multiple input files.

## Content
* **J1939**, the library which combines DBC parser, run-time data types and actual J1939 encoding/decoding functionality. It is compiled into a static library.
* **dbc_compiler**, the actual DBC compiler, which can process multiple input files and generate output code in two formats (see below).
* **test**, the unit tests folder which covers most of the components in the library.
* **can_reader**, collection of classes, compiled into a static library, to handle long messages and read/write of a physical CAN port using Socket CAN API. 
* **options**, super light-weight command line parser. Could be replaced by **gflags** in the future (TBD).
* **console**, a simple console app to read a log file and show its content in a human-readable format. It is provided to show how the parser could be used.

### How to build and run it.
dependencies:
* protobuf (follow the [instructions](https://github.com/protocolbuffers/protobuf/blob/main/src/README.md) to install the latest)
* gtest ```apt install libgtest-dev```

The following two commands will build the code and place three executables (*all_tests*, *can_sandbox* and *dbc_compiler*) into the **bin** folder:
(using the ```build``` directory keep the root directory free of CMake build artifacts)
```bash
mkdir build
cd build
cmake ..
make all
```
#### Running unit tests.
This is probably the easiest part:
```bash
VirtualBox:~/src/J1939$ bin/all_tests 
Running main() from /build/googletest-j5yxiC/googletest-1.10.0/googletest/src/gtest_main.cc
[==========] Running 20 tests from 5 test suites.
[----------] Global test environment set-up.
[----------] 5 tests from DbcParserTest
[ RUN      ] DbcParserTest.SimpleSignalFormatTest
[       OK ] DbcParserTest.SimpleSignalFormatTest (0 ms)
[ RUN      ] DbcParserTest.SignalWithValueFormatTest
       * * *
[----------] Global test environment tear-down
[==========] 20 tests from 5 test suites ran. (3 ms total)
[  PASSED  ] 20 tests.
VirtualBox:~/src/J1939$
```
#### Using can_sandbox.
This tool can decode CAN traffic log files in different text formats (ASC, LOG, TRC, OUT and ROS) and show the content on the console. A sample log file is provided. 
```bash
VirtualBox:~/src/J1939$ bin/can_sandbox
Usage: can_sandbox <options>
   where <options> could be:
    -in <arg>		Defines the input device (can0) or a log file (log|trc|txt|asc|ros|out) to read the data from.
    -out <arg>		Defines the CAN interface to play back the log file into or a file name to record the stream into (.log)
    -dbc <arg>		Defines a DBC file which should be used to parse CAN frames.
    -ext		Treat input messages as extended CAN frames (by default the extended bit from the CAN ID is used)
    -all		Show unset fields, i.e. filled with all 1
    -quiet		Do not show the stream content.

```
##### Using can_sandbox to decode a CAN stream from the log file or a physical CAN port (socket CAN interface).
```bash
virtualBox:~/src/J1939$ bin/can_sandbox -in candump.out -dbc tests/dbc/truck_messages.dbc
         0.000000 EEC1 PGN:F004(61444) SA:0(0) Prio:3 Data(8): FFFF0FFF0C0C1CFF { EngTorqueMode: 255 ActlEngPrcntTorqueHighResolution: 1 DriversDemandEngPercentTorque: 130 A
ctualEngPercentTorque: 18446744073709551506 EngSpeed: 415 SrcAddrssOfCntrllngDvcForEngCtrl: 12 EngStarterMode: 28 EEC1_DNC_1: 1 EngDemandPercentTorque: 130 }
         0.001000 UNDEF_2365548032 PGN:FF66(65382) SA:0(0) DA:FF(255) Prio:3 Data(8): 810303FF70000000 (....p...)
         0.002000 DM01 PGN:FECA(65226) SA:0(0) Prio:6 Data(8): 73FF0F38F000FF01 { ProtectLampStatus: 115 AmberWarningLampStatus: 28 RedStopLampState: 7 MalfunctionIndicatorL
ampStatus: 1 FlashProtectLamp: 255 FlashAmberWarningLamp: 63 FlashRedStopLamp: 15 FlashMalfuncIndicatorLamp: 3 DTC1: 15742991 DM01_DNC: 511 }
         0.003000 UNDEF_2349465377 PGN:900(2304) SA:21(33) DA:FF(255) Prio:3 Data(8): 73FF0FFFFF0FFFFF (s.......)
        * * *
        23.614000 EBC2 PGN:FEBF(65215) SA:B(11) Prio:6 Data(8): 000000FFFF0F7DFF { FrontAxleSpeed: 0 RelativeSpeedFrontAxleLeftWheel: 18446744073709551609 RelativeSpeedFrontAxleRightWheel: 8 RelativeSpeedRearAxle1LeftWheel: 8 RelativeSpeedRearAxle1RightWheel: 18446744073709551610 RelativeSpeedRearAxle2LeftWheel: 0 RelativeSpeedRearAxle2RightWheel: 8 }
        23.615000 EBC1 PGN:F001(61441) SA:17(23) Prio:6 Data(8): FFFF0F3CFF0FFFFF { ASREngCtrlActive: 255 ASRBrakeCtrlActive: 63 AntiLockBrakingActive: 15 EBSBrakeSwitch: 3 BrakePedalPos: 102 ABSOffroadSwitch: 15 ASROffroadSwitch: 3 ASRHillHolderSwitch: 0 TractionCtrlOverrideSwitch: 0 AccelInterlockSwitch: 60 EngDerateSwitch: 15 EngAuxShutdownSwitch: 3 RemoteAccelEnableSwitch: 0 EngRetarderSelection: 102 ABSFullyOperational: 15 EBSRedWarningSignal: 3 ABS_EBSAmberWarningSignal: 0 ATC_ASRInformationSignal: 0 SrcAddrssOfCntrllngDvcFrBrkeCtrl: 255 EBC1_DNC_1: 255 HaltBrakeSwitch: 63 TrailerABSStatus: 15 TrctrMntdTrailerABSWarningSignal: 3 }
        23.616000 EEC1 PGN:F004(61444) SA:0(0) Prio:3 Data(8): FFFF0FFF000000FF { EngTorqueMode: 255 ActlEngPrcntTorqueHighResolution: 1 DriversDemandEngPercentTorque: 130 ActualEngPercentTorque: 18446744073709551506 EngSpeed: 31 SrcAddrssOfCntrllngDvcForEngCtrl: 0 EngStarterMode: 0 EEC1_DNC_1: 0 EngDemandPercentTorque: 130 }
Average bus load: 8000.47 bytes/sec; duration: 23.615 sec.
                  1000.08 msg/sec
```

##### PGN format
The first one produces a single file where **all input DBC files** are compiled into a single std::map:
```c++
std::map<uint32_t, PGN> truck_messages_dbc = {
        . . .
};
```
J1939 class can use this map to decode/encode arbitrary CAN frame (take a look at the can_sandbox.cpp for details.) 
Note: by default, the names of the first input DBC file will be used to build the map name (i.e. "truck_messages.dbc" -> "truck_messages_dbc"). If needed, it could be overriden by using "-name" command line option. 
##### Union format
The second format places a bytes buffer and a packed structure of bitfields into an unnamed union which is saved into a header file.
*Note: a single header file will be generated for all listed input DBC files*
```c++
#define CTLEGO_MESSAGE_ID	0x8c210253
#define CTLEGO_MESSAGE_DLC	6

struct ctlego_message_t {
	union {
	    uint8_t data[CTLEGO_MESSAGE_DLC];
	    struct {
		uint16_t VehicleSpeed : 16;
		uint8_t ControlEngine : 1;
		uint16_t YawRate : 16;
		uint8_t TransCurrentGear : 8;
	    } fields __attribute__((packed));
	};
#ifdef __cplusplus

	double vehicle_speed() const { return fields.VehicleSpeed * 0.001 + 0; }
	void vehicle_speed(double value) { fields.VehicleSpeed = (value - 0) / 0.001; }

	uint8_t control_engine() const { return fields.ControlEngine; }
	void control_engine(uint8_t value) { fields.ControlEngine = value; }

	double yaw_rate() const { return fields.YawRate * 0.00012207 + -3.92; }
	void yaw_rate(double value) { fields.YawRate = (value - -3.92) / 0.00012207; }

	double trans_current_gear() const { return fields.TransCurrentGear * 1 + -125; }
	void trans_current_gear(double value) { fields.TransCurrentGear = (value - -125) / 1; }

	void fillFrameWith(uint8_t value = 0xFF) { memset(data, value, sizeof(data)); }
#endif

};
```
As you can see, this header file can be used even in a C code, if it is necessary, although the C++ would also provide the convenient getters/setters and fillFrameWith() methods.
DBC compiler also generates another structure which places all messages into a single union tp simplify the processing:
```c++
struct all_xyz_messages_t {
    const uint16_t max_dlc_ = 8;
    union {
	// Use this buffer to read the data from the CAN stream into. Mind the buffer size.
	uint8_t buffer_[max_dlc_]; 
	// Once you place the CAN frame into the buffer_[], use CAN ID to see if this is the
	// message you need and select the corresponding field below.
	ctlwdbrn_message_t ctlwdbrn_;
	ccvs2_message_t ccvs2_;
	ctlego_message_t ctlego_;
    . . .
    };
};
```
*Note: the name of that union will be taken from the first input DBC file name (i.e. xyz_messages.dbc -> all_xyz_messages_t).
