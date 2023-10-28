# Peer-to-Peer Group Based File Sharing System

A Peer-to-Peer (P2P) Group Based File Sharing System that enables users to share and download files from groups they belong to. This system divides files into logical pieces, performs SHA1 hashing for verification, provides user authentication for login, and includes error handling.

## Table of Contents

- [Introduction](#introduction)
- [Architecture Overview](#architecture-overview)
- [Commands](#commands)
- [Usage](#usage)
- [Requirements](#requirements)
- [Contributing](#contributing)
- [License](#license)

## Introduction

This project implements a P2P file-sharing system with group-based access control, user authentication, and parallel downloading of file pieces from multiple peers.

## Architecture Overview

The system consists of the following entities:

1. Synchronized Trackers (2 tracker system):
   - Maintain information of clients and their shared files.
   - Synchronized trackers are always in sync with each other.

2. Clients:
   - User account creation and registration with trackers.
   - User login with credentials.
   - Creation of groups with ownership.
   - Group management, including joining, leaving, and accepting/rejecting join requests.
   - Sharing files within a group.
   - Parallel downloading of file pieces from multiple peers.
   - Integrity verification using SHA1 hashing.
   - Stop sharing files and log out.

## Commands

### Tracker

1. **Run Tracker:**
   - Command: `./tracker tracker_info.txt tracker_no`
   - `tracker_info.txt` contains IP and port details of all trackers.

2. **Close Tracker:**
   - Command: `quit`

### Client

1. **Run Client:**
   - Command: `./client <IP>:<PORT> tracker_info.txt`
   - `tracker_info.txt` contains IP and port details of all trackers.

2. **Create User Account:**
   - Command: `create_user <user_id> <password>`

3. **Login:**
   - Command: `login <user_id> <password>`

4. **Create Group:**
   - Command: `create_group <group_id>`

5. **Join Group:**
   - Command: `join_group <group_id>`

6. **Leave Group:**
   - Command: `leave_group <group_id>`

7. **List Pending Join Requests:**
   - Command: `list_requests <group_id>`

8. **Accept Group Joining Request:**
   - Command: `accept_request <group_id> <user_id>`

9. **List All Groups in Network:**
   - Command: `list_groups`

10. **List All Sharable Files in Group:**
    - Command: `list_files <group_id>`

11. **Upload File:**
    - Command: `upload_file <file_path> <group_id>`

12. **Download File:**
    - Command: `download <group_id> <file_name> <destination_path>`

13. **Logout:**
    - Command: `logout`

14. **Show Downloads:**
    - Command: `show_downloads`

15. **Stop Sharing:**
    - Command: `stop_share <group_id> <file_name>`

## Usage

1. Ensure that all files are placed in the same directory before running the code. Remove empty "client" or "tracker" folders, as they may cause linker issues.
2. For compiling the `client.cpp` file, include flags for OpenSSL. For example, for macOS users:
   ```shell
   g++ client.cpp -o client -lssl -lcrypto -L/opt/homebrew/opt/openssl@3/lib -I/opt/homebrew/opt/openssl@3/include
   ```

3. To run the client file, use a command like this:
   ```shell
   ./client 127.0.0.1:9999 tracker_info.txt

   # Here, 127.0.0.1 is the IP and 9999 is the port number.
   ```

4. To compile the tracker.cpp file, use the following command:

```shell
   g++ tracker.cpp -o tracker
   ```

5. To run the tracker, use the command:
   ```shell
   ./tracker tracker_info.txt 1

   # Here, 1 is the tracker number.
   ```

6. tracker_info.txt should contain all the tracker information.

7. The code will create extra folders and files in the working directory.

8. Make sure OpenSSL is installed before running.

## Requirements
C++ compiler (g++)
OpenSSL library

## Contributing
If you want to contribute to this project, feel free to submit issues or pull requests on the GitHub repository.

## License
This project is licensed under the MIT License - see the LICENSE file for details.

