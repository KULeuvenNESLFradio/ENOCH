## Verified Transmitting Nodes

This folder contains the verified and working original code for **7-node transmitting nodes latency experiments**. The highest priority is assigned to **Node 0**, followed by **Node 1**, and so on.

### Priority Sequence *(just for evaluation, if you want to change the cycles, modify `Send_preamble(int cycles)`)*
- **Node 0**: 10 cycles (highest priority)
- **Node 1**: 9 cycles
- …
- **Minimum priority gradient**: 1 cycle = 100 µs

In the experiment, the priority sequence was configured in decreasing order, ensuring a structured transmission hierarchy.


