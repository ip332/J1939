## Summary

The `J1939` repository provides a library for encoding and decoding CAN frames, along with tools to compile DBC files into a compact, generated data format.

## Background

There are two different scenarios where you might want to read a J1939 stream:
* **Message-centric**, when you know exactly which messages you need to read. This is how a typical ECU (engine, brakes, FLR, ACC, etc.) works: it knows how to handle a small subset of messages and ignores the rest.
* **Streaming**, when you want to read all messages, either to log them to a file or to share them with other components in the system.

Both scenarios could be addressed with the same implementation, but the message-centric case should be designed to meet the performance and memory constraints typical of automotive ECUs. In this case, the application code declares a variable for each message it cares about (e.g., TSC1, XBR) and ignores the rest of the CAN traffic. In streaming mode, however, declaring an individual variable for every message type would be impractical, so this mode needs a generic API backed by detailed metadata about all messages and signals.

Regardless of the use case, encoding and decoding should behave consistently across all modules and subsystems. One option is to parse a DBC file at runtime, as the roscan project does, but this approach may not be acceptable for safety-critical applications. Compiling the DBC at build time also allows more thorough validation.

This project therefore provides a DBC compiler that generates output in two formats — **PGN** and **Union** — from one or more input files.

## Content

* **J1939** — the library combining the DBC parser, runtime data types, and the J1939 encoding/decoding functionality. Built as a static library.
* **dbc_compiler** — the DBC compiler itself, which processes multiple input files and generates output code in the two formats described below.
* **tests** — the unit test suite covering most of the library's components.
* **can_reader** — a collection of classes, built as a static library, that handle long (multi-frame) messages and reading/writing a physical CAN port via the SocketCAN API.
* **options** — a lightweight command-line parser. May be replaced with **gflags** in the future (TBD).
* **console** — a simple console application that reads a log file and prints its content in a human-readable format, demonstrating how the parser can be used.

### Building and running

Dependencies:
* protobuf (follow the [instructions](https://github.com/protocolbuffers/protobuf/blob/main/src/README.md) to install the latest version)
* gtest — ```apt install libgtest-dev```

The following commands build the code and place three executables (*all_tests*, *can_sandbox*, and *dbc_compiler*) into the **bin** folder. Building inside the `build` directory keeps the repository root free of CMake artifacts:
```bash
mkdir build
cd build
cmake ..
make all
```
#### Running unit tests

This is the simplest part:
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
#### Using can_sandbox

This tool decodes CAN traffic log files in several text formats (ASC, LOG, TRC, OUT, and ROS) and prints the content to the console. A sample log file is provided.
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
##### Decoding a CAN stream from a log file or a physical CAN port (SocketCAN interface)
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
The first format produces a single file where **all input DBC files** are compiled into one `std::map`:
```c++
std::map<uint32_t, PGN> truck_messages_dbc = {
        . . .
};
```
The `J1939` class uses this map to decode/encode arbitrary CAN frames (see `can_sandbox.cpp` for details).
Note: by default, the name of the first input DBC file is used to build the map name (e.g., `truck_messages.dbc` → `truck_messages_dbc`). This can be overridden with the `-name` command-line option.

##### Union format
The second format packs a byte buffer and a bitfield struct into an unnamed union, which is saved to a header file.
*Note: a single header file is generated for all listed input DBC files.*
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
This header can be used from C code if necessary, though C++ also gets the convenience of getters/setters and the `fillFrameWith()` method.

The DBC compiler also generates a second structure that places all messages into a single union to simplify processing:
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
*Note: the name of that union is taken from the first input DBC file's name (e.g., `xyz_messages.dbc` → `all_xyz_messages_t`).*
