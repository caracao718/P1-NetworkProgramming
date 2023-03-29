# P1-NetworkProgramming PART 1
Compression Detection Client/Server Application
Developer's Name: Cara Cao

## Instructions
To run Server:
1. cd into the server directory: cd server
2. run: gcc server.c -o server
3. run: ./server 8787

To run Client:
1. cd into the client directory: cd client
2. run: gcc client.c -o client
2. run: ./client ../config.json

## Rubric 
To my best understanding, I have completed all the requirments for this project.
1. Config File (enabling the feature and correct parsing) ~ DONE
2. Pre/Post-Probing Phase TCP Connection ~ DONE
3. UDP Packet Train: All packets have the fixed and correct size: implemented as in the config file ~ DONE
4. UDP Packet Train: Don’t fragment bit is set: client.c on line 205 ~ DONE
5. UDP Packet Train: Packet IDs are properly set and retrieved: first two bytes of the UDP packet (on my VM it's little endian) ~ DONE
6. UDP Packet Train: Low entropy and high entropy payloads are properly set ~ DONE
    - Low entropy: I used the followng code to fill payload with 0:
        for (int i = 2; i < config.size_UDP_payload; i++) {
            udp_message[i] = 0x00;
        }
    - High entropy: I used the following code to set random sequence of bits:
        for (int i = 2; i < config.size_UDP_payload; i++) {
            udp_message[i] = rand() % 256;
        }
        This for loop fills udp_message (after the second byte) with random values between 0 and 255.

7. UDP Packet Train: Source/Destination ports and IP addresses are set ~ DONE
8. Error checking and handling ~ DONE


