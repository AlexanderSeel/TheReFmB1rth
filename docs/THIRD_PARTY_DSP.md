# Third-party DSP references

## Open303

The ACID engine's TB-303 filter topology and coefficient model are informed by Robin Schmidt's Open303 implementation.

- Project: Open303
- Author/copyright: Robin Schmidt, 2009
- License: MIT
- Reference commit used during implementation review: `52f8614966cde2c1fd9d76369cf9d07e766262ed`
- Relevant reference: `Source/DSPCode/rosic_TeeBeeFilter.h/.cpp`

Open303's MIT license permits use, modification and redistribution when its copyright and permission notice are retained. TheReFmB1rth does not copy ReBirth/Roland artwork or proprietary sample ROMs.

The embedded implementation in `firmware/proto/acid303.c` is a fixed-point adaptation for the FM-1 target rather than a byte-for-byte copy of the floating-point C++ code. It preserves the important modeled topology: four coupled low-pass stages, the TB-303-specific feedback gain behavior, and a high-pass filter in the resonance feedback path.

Open303 MIT notice:

Copyright (c) 2009 Robin Schmidt (www.rs-met.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
