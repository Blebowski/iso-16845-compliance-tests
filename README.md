
# ISO 16845 Compliance tests

This repository implements ISO 16845 compliance test suite and cycle accurate model
of CAN Bus according to ISO 11898.

# Principle of operation

The test suite is written in C++ 17 and compiled to shared object library. The shared
object library is then linked to digital simulation where an Implementation under
test is simulated. ISO Compliance test suite communicates with digital simultion via
VHPI or VPI interfaces. Inside the simulation there is a CTU CAN FD VIP that drives
pins of Implementation under test (IUT). Currently, only CTU CAN FD IP Core implementation
is supported.

The model of CAN Bus communication is used to create CAN frame objects, that are
converted to series of Dominant and Recessive values. These values are then sent
to digital simulator, and applied on DUT pins. The approach is equivalent to TLM as is
common in Digital verification frameworks such as UVM. Frame objects are abstract
objects / transactions, that are converted to series of 1 and 0 driven or monitored
on DUT input pins.

Frames in digital simulation are processed by CAN Agent that contains driver and
monitor. There are two types of frames in each test:
- Driven frame - Driven by driver on `can_rx` pin of IUT
- Monitored frame - Monitored by monitor on `can_tx` pin of IUT

# Supported simulators

Currently, the simulation is supported with following simulators:
 - Synopsys VCS - via VHPI interface
 - NVC - via VHPI interface
 - GHDL - via GHDL specific subset of VPI interface.

There is an ongoing effort to port the implementation to NVC.

# Required dependencies

To build the compliance tests you need following dependencies:
- CMake 3.5 or higher
- C++ 17 compliance compiler (GCC 7.2.0 and GCC 8.5.0 are confirmed to work)

# Build

To build the compliance tests, execute:

```
./build.sh
```

# License

ISO Compliance test suite is licensed under semi-proprietary license requiring
a license agreement for commercial usage. Non-commercial, academical or hobby
usage dos not require license agreement.
