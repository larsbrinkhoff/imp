## Arpanet IMP implemented in C

This is part of the effort to add simulated IMP interfaces in SIMH.

See [BBN Report 1822](http://bitsavers.org/pdf/bbn/imp/BBN1822_Jan1976.pdf)
for the protocol used to communicate with the IMP.

### API

First reset the IMP:

- t_stat imp_reset (IMP *imp)

Then set imp.receive_bit to a callback which is used to receive data
from the network.  The callback will receive a boolean bit, and
another boolean to say whether the bit was the last in a message.  The
callback has this type:

- t_stat (*) (int bit, int last)

The IMP accepts data from a host using one of these two APIs:

- t_stat imp_send_bits (IMP *imp, uint64 data, int n, int last)
- t_stat imp_send_packet (IMP *imp, void *octets, int n)

The first function accepts input in the lower n bits of data.  Message
bits accumulate until last is true.

The second function can be used to send a complete packet with n bits.

![](https://user-images.githubusercontent.com/775050/37401960-1caf3e6e-278a-11e8-8a9f-357b8cea3720.jpg)
