Assignment #3 - Peer-to-Peer Group Based File Sharing System

1. Take all the files out of all the folders and place at one place before running the code, don't keep empty client or tracker folder in the same place, that may cause linker issue.
2. To compiling the client.cpp file include flags for openssl needs. Eg for mac users- 
	g++ client.cpp -o client -lssl -lcrypto -L/opt/homebrew/opt/openssl@3/lib -I/opt/homebrew/opt/openssl@3/include
3. To run client file, run command like this- ./client 127.0.0.1:9999 tracker_info.txt, where 127.0.0.1 is the ip and 	9999 is the port number.
4. To compile tracker.cpp file, use command, g++ tracker.cpp -o tracker
5. To run tracker, ./tracker tracker_info.txt 1, where 1 is the tracker number.
6. tracker_info.txt has been added with all the tracker information.
7. Code will create extra folders and files while running at the working directory.
8. Make sure openssl is installed before running.
