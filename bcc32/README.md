# C++Builder 2009 / BCC32 Win32 DLL

`NmeaDecoderBcc32.cpp` is the C++03 implementation of the DHF NMEA module for
the classic **BCC32 6.11** compiler bundled with C++Builder 2009. It does not
include the modern C++17 parser sources from `NMEA/`, which use language and
standard-library features unavailable to that compiler.

## Build

From a C++Builder 2009 command prompt, set `BCB` to the product directory and
run:

```bat
cd bcc32
build_bcc32.bat
```

The script invokes `bcc32 -WD -tWD -6`, producing the 32-bit DLL
`nmea_decoder.dll`. The accompanying DEF file fixes the host-facing export
names to `getIModuleExt` and `removeIModuleExt`; both functions use the DHF
interface's `__stdcall` convention.

## Compatibility scope

* The DLL is a native Win32 DLL and implements the supplied `IModuleExt` ABI.
* It accepts newline-separated NMEA 0183 input and validates its XOR checksum.
* It accepts `UdPbC\0` NMEA-450 packets, including grouped AIS fragments; groups
  expire after three seconds using the Win32 `GetTickCount` API.
* The DLL and the DHF host must both be built with the same C++Builder runtime
  memory-manager configuration because an `IModuleExt` object is created and
  destroyed through the exported factory pair.
