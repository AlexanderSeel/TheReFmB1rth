# Visual DSP graphics

The FM-1 UI must make modulation and synthesis changes visible instead of exposing only numbers.

Felucca already proves the 240×240 UI can efficiently render ADSR and LFO curves in its graph panel. TheReFmB1rth will reuse that rendering concept while keeping the graph model separate from the drawing backend so the FM-1 UI and future React/WASM editor show the same parameter response.

## Required FM-1 graphs

### LFO
- live waveform preview for sine/triangle/saw/square and later random/S&H;
- amplitude reflected vertically;
- phase/start offset reflected horizontally;
- rate shown by number of visible cycles;
- destination indicator (pitch/filter/amp/etc.);
- animated playhead/phase dot only when animation cost is safe.

### Filter
- frequency-response-style curve;
- cutoff moves the knee horizontally;
- resonance visibly raises the peak;
- envelope amount adds a second/ghost curve or sweep range;
- active modulation can show a lightweight moving cutoff marker;
- acid pages prioritize cutoff/resonance/envelope interaction.

### ADSR
- attack/decay/sustain/release curve updates immediately with knobs;
- sustain level is visibly horizontal;
- attack/decay/release duration changes horizontal segment length;
- optional envelope position dot while a note is sounding.

## Rendering rules
- graph computation must allocate no memory;
- graph values use fixed-point/integer math on device;
- redraw only when parameters/signature or animation phase changes;
- curves remain readable in all palettes and high-contrast mode;
- graphs must not reduce audio real-time stability;
- UI tests render extreme parameter combinations and check clipping/overflow.

## Shared graph model

`firmware/proto/ui_graph_model.*` is the first host-testable model. It outputs normalized curve points for ADSR, LFO and filter response. The final FM-1 renderer will map those points to Felucca-style canvas primitives, while the browser editor can map the same normalized data to SVG/Canvas.

The initial filter graph is illustrative rather than a mathematical transfer-function plot. Once the final acid filter DSP is selected, its graph model should be calibrated against the actual DSP response.
