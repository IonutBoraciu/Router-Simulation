
# Boraciu Ionut Sorin 325CA

- The router will wait in an infinite loop for a package to arrive

- Once a package arrives the router will check its time to live, if it 0 or 1
    the router will send an icmp message back to the source to announce it that the
    package was dropped ( the mac address in the ehternet layer will have to changed accordinly :
    the new source will be **the router** and the destination will be the **initial source**, in the internet
    header the ips will change in the same way)

- Check if the package is either ipv4 or arp, if its arp continue and wait until an ipv4 is received

- Check that the package wasn't corrupted or changed until this point by recomputing the control sum
       and comparing it to the saved one

- Next, we will check if the router received an "echo request" package ( by checking if the package received is an
    icmp package, and the type of it is 8 and code is 0, also checking that the destination is **THE ROUTER**). If it
    is, we will send an echo reply message, by changing the type of the icmp packet, and the source and destination.

- Decrement the ttl, if the package wasn't send until this point, and **recompute** the control sum with the new ttl

- Search for the best route by using a binary search algorithm, if no route was found send an **icmp message**, to 
    alert the source that the destination is **unreachable**

- Get the mac address of the **best route's ip**, if it wasn't found send a icmp message alerting the source

- If everything worked according to plan until this point, send **the packet** to the destination.

- At the end of the programme, free the data.



  
