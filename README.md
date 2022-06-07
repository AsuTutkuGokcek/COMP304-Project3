# COMP304-Project3
Project 3 for COMP304 course of 2022, Koç University.

Reference to repo: https://github.com/AsuTutkuGokcek/COMP304-Project3/blob/main/README.md

Asu Tutku Gökçek 71766
Ali Taylan Akyürek 64229


First, we created the filesystem file in mini_fat_create.
Then we did mini_fat_write_in_block. We accomplished this by creating a file and using fseek and fwrite.
Then we did mini_fat_read_in_block. We accomplished that by doing the same thing as mini_fat_write_in_block. Onlt difference is we used fread instead of fwrite.
Then we did the mini_fat_find_empty_block function. For every fat->block_map, we checked if at the i'th location the block is EMPTY_BLOCK. If it is, we returned i.
For mini_file_open, we did accordingly: if !fd, and if is_write==true, call mini_file_create_file to create the file. Else print error. If is_write is true, check all fd->open_handles. If at i'th position is_write==true, print error. Below these we got the open_file position equal to 0, file equal to fd and is_write equal to is_write.
For seek, we used from_start in the if case; if it is true, we set newPos = offset, if not we did newPos = open_file->position + offset. If new position is bigger than size, we returned false. Lastly, we did open_file->position = newPos.
For delete, we used a for loop to check is_write in all open_handles. If it is true, we printed "it can't be deleted". Lastly, we called vector_delete_value(fs->files, file) function.