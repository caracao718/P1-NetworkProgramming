# P1-NetworkProgramming PART 2
Compression Detection Standalone Application
Developer's Name: Cara Cao

## Instructions
1. run: gcc client.c -o client
2. run: sudo ./client ./config.json

## Rubric
1. TTL is set properly in UDP packet train ~ DONE
2. Head and Tail SYN packets are correctly created as specified ~ DONE
3. Capturing and handling the RST packets: 
    For some reason, I cannot capture any RST packets. In Wireshark, it shows clearly that I have sent out SYN packets to 2 different ports that are less commonly used. 
    After doing some research online, I found out that maybe it's a firewall issue at my place that's blocking the RST packets.
4. Timeout ~ DONE
5. Error checking and handling ~ DONE
6. .pcap file ~ DONE  (it's in the same directory as this README file)