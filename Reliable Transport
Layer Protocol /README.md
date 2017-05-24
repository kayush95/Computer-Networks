# A Reliable Transport Layer Protocol using Raw Sockets
The objective of this work is to develop a reliable transport layer protocol on the top of
the IP layer. IP layer provides unreliable datagram delivery ­ there is no guarantee
that a packet transmitted using IP will be eventually delivered to the receiver, because of any
of the following reasons:
* The packet may get lost during end­to­end delivery ­ this can happen due to many
reasons, like buffer overflow, forwarding error etc.
* The packet is transmitted to the end host, but the content of the packet may get
corrupted.
