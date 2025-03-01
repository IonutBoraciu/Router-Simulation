
# Router Simulation

- The router will wait in an infinite loop for a packet to arrive

- Once a package arrives the router will check its Time to Live( TTL), if it 0 or 1
    the router will send an ICMP message back to the source to announce it that the
    packet was dropped ( the mac address in the ehternet layer will have to changed accordinly :
    the new source will be **the router** and the destination will be the **initial source**, the IP addresses in the Internet header will also be updated in the same way)

- Check if the packet is either IPV4 or ARP, if its ARP continue and wait until an IPV4 is received

- Check that the packet wasn't corrupted or changed until this point by recomputing the control sum
       and comparing it to the saved one

- Next, check if the router received an "echo request" packet by verifying that the packet is an ICMP packet, its type is 8, and its code is 0, while also ensuring that the destination is the ***ROUTER***). If it
    is, we will send an echo reply message, by changing the type of the ICMP packet, and the source and destination.

- Decrement the TTL, if the packet wasn't send until this point, and **recompute** the checksum sum with the new TTL

- Search for the best route by using a binary search algorithm, if no route was found send an **ICMP message**, to 
    alert the source that the destination is **unreachable**

- Get the MAC address of the **best route's ip**, if it cannot be found send a ICMP message alerting the source

- If everything has worked correctly up to this point, send the ***packet*** to the destination.

- At the end of the program, free the allocated data.



  
