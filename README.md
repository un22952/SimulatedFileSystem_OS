# File System Simulator

## Overview

This project implements a simplified file system simulator in C.

The simulator provides a virtual disk divided into fixed-size blocks and implements core file-system concepts including:

* Superblocks
* Inodes
* Inode and block bitmaps
* Direct data blocks
* Directories and directory entries
* File creation and deletion
* File reading
* File contents display
* Directory creation and navigation
* Recursive directory removal
* Hard links
* File metadata
* File-system statistics
* Mounting and unmounting a simulated disk

The system provides an interactive command-line interface through which users can create files and directories, navigate the file system, inspect metadata, and manage disk space.

---

## Features

### Virtual Disk

The project uses a simulated disk represented by a two-dimensional array:

```c
char disk[MAX_BLOCK][BLOCK_SIZE];
```

Each disk block contains `BLOCK_SIZE` bytes, and the disk contains `MAX_BLOCK` blocks.

The simulated disk can be:

* Mounted from an existing disk file
* Created when the disk file does not exist
* Read block-by-block
* Written block-by-block
* Saved to a disk file when unmounted

The disk implementation uses standard C file operations to load and save the simulated disk.

---

## File System Architecture

The file system maintains several major data structures in memory:

```text
+-----------------------+
|      SuperBlock       |
+-----------------------+
|      Inode Bitmap     |
+-----------------------+
|      Block Bitmap     |
+-----------------------+
|        Inodes         |
+-----------------------+
|      Data Blocks      |
+-----------------------+
```

The main file-system structures include:

```c
char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];
Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry curDir;
int curDirBlock;
```

These structures track inode allocation, block allocation, file metadata, directory information, and the current working directory.

---

## Project Structure

```text
.
├── Makefile
├── fs_sim.c
├── fs.c
├── fs_util.c
├── disk.c
├── fs.h
├── fs_util.h
├── disk.h
└── README.md
```

### Files

| File        | Description                                                        |
| ----------- | ------------------------------------------------------------------ |
| `fs_sim.c`  | Main interactive file-system simulator                             |
| `fs.c`      | Core file-system implementation                                    |
| `fs_util.c` | Utility functions for strings, bitmaps, allocation, and timestamps |
| `disk.c`    | Simulated disk implementation                                      |
| `fs.h`      | File-system structures, constants, and declarations                |
| `fs_util.h` | Utility function declarations                                      |
| `disk.h`    | Virtual disk definitions and declarations                          |
| `Makefile`  | Build configuration                                                |
| `README.md` | Project documentation                                              |

---

# Building the Project

The project is written in C and can be compiled using the provided Makefile.

Run:

```bash
make
```

If the Makefile provides a clean target, previously generated object files and executables can be removed with:

```bash
make clean
```

The exact compiler flags and executable name are determined by the provided Makefile.

---

# Running the File System Simulator

The simulator expects the name of a disk file as its argument:

```bash
./fs disk_name
```

For example:

```bash
./fs mydisk
```

The program checks that a disk name is supplied:

```text
usage: ./fs disk_name
```

When started, the simulator mounts the specified disk and presents an interactive prompt:

```text
%
```

The simulator reads commands from standard input until `quit` or `exit` is entered.

---

# Disk Mounting

When the simulator starts, `fs_mount()` attempts to mount the specified disk file.

If the disk already exists, the simulator loads:

1. Superblock
2. Inode bitmap
3. Block bitmap
4. Inodes
5. Root directory

from the simulated disk.

If the disk does not exist, the file system initializes a new disk.

The initial file system includes:

* A superblock
* Inode bitmap
* Block bitmap
* Inode table
* Root directory

The root directory contains the `"."` directory entry.

---

# Disk Layout

The first blocks of the disk are reserved for file-system metadata.

Conceptually:

```text
Block 0
+----------------------+
|      SuperBlock      |
+----------------------+

Block 1
+----------------------+
|     Inode Bitmap     |
+----------------------+

Block 2
+----------------------+
|     Block Bitmap     |
+----------------------+

Blocks 3 ...
+----------------------+
|     Inode Table      |
+----------------------+

Remaining Blocks
+----------------------+
|      Data Blocks     |
+----------------------+
```

When a new file system is initialized, the metadata blocks are marked as occupied in the block bitmap.

---

# Inodes

Each file and directory is represented by an inode.

The inode stores information such as:

* File type
* Owner
* Group
* Creation time
* Last-access time
* File size
* Number of blocks
* Link count
* Direct block references

For example, when a file is created, its inode is initialized with its type, owner, group, timestamps, size, block count, and link count.

---

# Bitmaps

The simulator uses two bitmaps to track resource allocation.

### Inode Bitmap

The inode bitmap tracks which inodes are currently allocated.

### Block Bitmap

The block bitmap tracks which disk blocks are currently allocated.

Utility functions are provided to manipulate individual bits:

```c
toggle_bit()
get_bit()
set_bit()
```

## The allocation functions scan the corresponding bitmap for a free entry, mark it as used, update the superblock counters, and return the allocated index.

# File Operations

## Create a File

```text
create <filename> <size>
```

Example:

```text
create hello.txt 100
```

The file system:

1. Checks the requested size.
2. Checks whether the filename already exists.
3. Checks directory capacity.
4. Calculates the number of required blocks.
5. Checks available blocks and inodes.
6. Allocates an inode.
7. Generates file contents.
8. Allocates data blocks.
9. Writes the contents to the simulated disk.
10. Adds the file to the current directory.

Files larger than `SMALL_FILE` are not supported.

---

## Display File Contents

```text
cat <filename>
```

Example:

```text
cat hello.txt
```

`cat` locates the file's inode, reads its direct blocks from the simulated disk, reconstructs the file contents, and prints them. It also updates the file's last-access timestamp.

---

## Read a Portion of a File

```text
read <filename> <offset> <size>
```

Example:

```text
read hello.txt 10 20
```

The `read` operation allows a specific section of a file to be retrieved.

The implementation calculates:

* The starting block
* The offset within the starting block
* The blocks that must be read
* The number of bytes remaining

It then reads the necessary disk blocks and reconstructs the requested data.

---

## File Metadata

```text
stat <filename>
```

Example:

```text
stat hello.txt
```

The command displays information including:

```text
Inode
Type
Owner
Group
Size
Link count
Number of blocks
Created time
Last access time
```

## The timestamps are formatted using the utility function `format_timeval()`.

## Remove a File

```text
rm <filename>
```

Example:

```text
rm hello.txt
```

Removing a file:

1. Finds the file's inode.
2. Removes its directory entry.
3. Checks its link count.
4. Releases its data blocks.
5. Releases its inode.
6. Updates free-block and free-inode counters.

If the inode has multiple hard links, removing one link decreases the link count rather than immediately freeing the inode and data blocks.

---

# Directory Operations

## List Directory Contents

```text
ls
```

Example output includes:

```text
type: file, name "hello.txt", inode 1, size 100 byte
type: dir, name "docs", inode 2, size 1 byte
```

The `ls` implementation iterates through the current directory's directory entries and displays each entry's type, name, inode number, and size.

---

## Create a Directory

```text
mkdir <dirname>
```

Example:

```text
mkdir docs
```

Creating a directory:

1. Checks whether the name already exists.
2. Checks directory capacity.
3. Checks available blocks and inodes.
4. Allocates a new inode.
5. Allocates a data block.
6. Creates `"."` and `".."` entries.
7. Adds the directory to the parent directory.

The new directory's `"."` entry points to itself, while `".."` points to the parent directory.

---

## Change Directory

```text
cd <dirname>
```

Example:

```text
cd docs
```

The simulator writes the current directory back to disk, loads the selected directory into memory, updates the current directory block, and updates the directory's last-access time.

To move to the parent directory:

```text
cd ..
```

---

## Remove a Directory

```text
rmdir <dirname>
```

Example:

```text
rmdir docs
```

The directory removal implementation supports recursive deletion of subdirectories.

However, if a directory contains a regular file, the operation refuses to remove the directory and instructs the user to delete the files first.

When a directory is successfully removed, its inode and data block are released and the directory entry is removed from its parent.

---

# Hard Links

The simulator supports hard links using:

```text
ln <source> <destination>
```

Example:

```text
create original.txt 100
ln original.txt copy.txt
```

A hard link does not create a new inode. Instead, the new directory entry points to the existing inode and increments its `link_count`.

Conceptually:

```text
original.txt ----+
                 |
                 v
              Inode 5
                 |
                 v
             Data Blocks

copy.txt -------+
```

Both directory entries reference the same inode.

---

# File-System Statistics

The command:

```text
df
```

displays the number of free blocks and free inodes.

Example format:

```text
File System Status:
# of free blocks: 1000 (512000 bytes), # of free inodes: 50
```

The free-space calculation is based on the counters maintained in the superblock.

---

# Available Commands

| Command                       | Description                         |
| ----------------------------- | ----------------------------------- |
| `df`                          | Display free blocks and free inodes |
| `create <file> <size>`        | Create a file                       |
| `stat <file>`                 | Display file metadata               |
| `cat <file>`                  | Display complete file contents      |
| `read <file> <offset> <size>` | Read a portion of a file            |
| `rm <file>`                   | Remove a file                       |
| `ln <src> <dest>`             | Create a hard link                  |
| `ls`                          | List current directory contents     |
| `mkdir <dir>`                 | Create a directory                  |
| `rmdir <dir>`                 | Remove a directory                  |
| `cd <dir>`                    | Change the current directory        |
| `quit`                        | Exit the simulator                  |
| `exit`                        | Exit the simulator                  |

These commands are dispatched through `execute_command()`.

---

# Example Session

Build the project:

```bash
make
```

Start the simulator:

```bash
./fs mydisk
```

Then run commands such as:

```text
% df

% create hello.txt 100

% ls

% stat hello.txt

% cat hello.txt

% read hello.txt 0 20

% mkdir docs

% ls

% cd docs

% ls

% cd ..

% ln hello.txt hello_link.txt

% ls

% rm hello.txt

% ls

% quit
```

The exact generated file contents are determined by the simulator's random-string generator. The program initializes the random seed with `srand(0)`.

---

# Utility Functions

The project includes several utility functions.

### Random String Generation

```c
rand_string()
```

generates alphanumeric strings using:

```text
abcdefghijklmnopqrstuvwxyz
ABCDEFGHIJKLMNOPQRSTUVWXYZ
0123456789
```

and is used when creating file contents.

### Bitmap Operations

```c
toggle_bit()
get_bit()
set_bit()
```

provide low-level manipulation of the inode and block allocation bitmaps.

### Timestamp Formatting

```c
format_timeval()
```

converts a `timeval` into a formatted UTC timestamp such as:

```text
YYYY-MM-DD HH:MM:SS.ffffffZ
```

---

# Mount and Unmount

## Mount

```c
fs_mount(name);
```

loads an existing disk image or initializes a new file system if the disk image does not exist.

The mount operation loads the superblock, allocation maps, inodes, and root directory into memory.

## Unmount

```c
fs_umount(name);
```

writes the current superblock, allocation maps, inodes, and current directory back to the simulated disk before closing the disk file.

---

# Data Persistence

The virtual disk exists in memory while the simulator is running:

```text
File-system operations
        |
        v
   In-memory disk
        |
        v
     Unmount
        |
        v
 Disk image file
```

`disk_mount()` loads an existing disk image into the `disk` array, while `disk_umount()` writes the entire array back to the disk image file.

This allows file-system state to persist between executions.

---

# Concepts Demonstrated

This project demonstrates several operating-system and file-system concepts:

* File-system organization
* Virtual disks
* Disk blocks
* Superblocks
* Inodes
* Inode allocation
* Block allocation
* Bitmaps
* Directory entries
* Current working directories
* File metadata
* File timestamps
* Direct block addressing
* Hard links
* Reference/link counts
* Mounting and unmounting
* Persistent disk images
* Recursive directory operations
* Low-level C memory management

---

# Limitations

The implementation is a simplified educational file system rather than a complete production file system.

Some implementation constraints include:

* Files larger than `SMALL_FILE` are rejected.
* Files use direct block references.
* Directory capacity is limited by `MAX_DIR_ENTRY`.
* The simulator operates on a fixed-size virtual disk.
* File contents are generated as random alphanumeric strings during creation.
* Directory removal requires regular files to be deleted first.
* The available commands are limited to those implemented by `execute_command()`.

These limitations are part of the simulator's design and make the project suitable for demonstrating fundamental file-system concepts.

---

# Summary

This project provides a complete educational simulation of a basic file system. It combines a virtual block device with in-memory file-system metadata and an interactive shell.

The main flow is:

```text
             Disk Image
                 |
                 v
           +-----------+
           | disk.c    |
           | Virtual   |
           | Disk      |
           +-----+-----+
                 |
                 v
           +-----------+
           |  fs.c     |
           | File      |
           | System    |
           +-----+-----+
                 |
       +---------+---------+
       |         |         |
       v         v         v
    Files   Directories  Links
       |         |         |
       +---------+---------+
                 |
                 v
           Interactive CLI
             fs_sim.c
```

The project demonstrates how higher-level file operations such as `create`, `cat`, `read`, `rm`, `mkdir`, `cd`, `rmdir`, and `ln` can be implemented on top of low-level disk blocks, bitmaps, inodes, and directory entries.
