UY NGUYEN

I implemeted: 
    file_read(file, offset, size): read "size" bytes from "offset" in file. 
        Design: It copies data stored on its direct blocks to a string then prints the string, and it updates the last access time.
        Test: It returns -1 when the "file" is not found, "file" is a directory, and "offset" is larger than the "file" size.
    hard_link(src, dest): add connection "dest" to "src".
        Design: "dest" shares the same inode with "src", link_count is updated by one every connection added. A new entry for "dest" is added to the current directory(updating numEntry and dentry), and last access time is updated.
        Test: It returns -1 when the "src" is not found or "dest" exists, the curDir.numEntry + 1 > MAX_CUR_DIR.
    file_remove(file): remove file from the curDir.
        Design: It removes its entry from the curDir and frees its data blocks if its hard link count is 1. If "file" has hard link count > 1, it will just remove the entry on the curDir and decrement the hard link count.
        Test: It returns -1 when file is not found. Deleting one of the hard links will not free the data blocks.
    dir_make(dir): create a new directory from the curDir.
        Design: It creates new entry on the curDir and occupies one new block of data and one block of new inode. Inside the newly-creatde directory by default, it will have "."(first directory) and ".."(second directory).
        Test: It returns -1 when "dir" exists, inode blocks or data blocks are full, and the number of entries exceeds the limit.
    dir_remove(dir): remove "dir" and its subdirectories.
        Design: It's a recursive process. If it encounters a file, it will request users to delete the file first before deleting the directory as we have separate functions to delete files and directories. It keeps recursively deleting subdirectories until done. When it deletes a directory, it will free the data block and inode block occupied by the directory. Then, it will change directory to the parent of "dir" to remove its entry on the parent and free its inode block, and update free inode blocks and data blocks on the super block.
        Test: It returns -1 when "dir" is not found, and when "dir" contains files in it or its subdirectories.