//Cahit Yusuf Taş -1937465
//cyusuftas@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

//Structs required for File System implementation
typedef struct{
    char file_name[248];
    uint32_t first_block;
    uint32_t file_size;

} FileList;

typedef struct{
    uint32_t FAT[4096];
    FileList File_List[128];
} FS_Header;

typedef struct{
    FS_Header header;
    char Data[4096][512];
} FileSystem;

FileSystem FS;

//Global variables
char *src_file;
char *dest_file;
char *disk;
char buffer[512];

FILE *infile;
FILE *outfile;

/****************
Format function is used to clear FAT table
and file list entries.
*****************/
void Format(){
    //Clear FAT
    for(int i=1; i<4096; i++){
        FS.header.FAT[i] = 0;
    }

    //Clear File List
    char empty_file_name = 0x0;
    for(int i=0; i<128; i++){
        //To make it efficient only first byte of the file name is written
        //with special byte(0x0 - NULL) instead of clearing all 248 name bytes.
        //Name bytes will be cleared and written with new name when new file
        //is written to corresponding block.
        FS.header.File_List[i].file_name[0] = empty_file_name;
        FS.header.File_List[i].first_block = 0;
        FS.header.File_List[i].file_size = 0;
    }
    //Write formatted FS.header to fs_header_info.dat file
    FILE *outfile;
    outfile = fopen("./disk/fs_header_info.dat", "w+");
    if(outfile != NULL){
        fwrite(&FS.header, sizeof(FS.header), 1, outfile);
    }
}

/**************
Write function is used to write new file to disk.
It updates the FAT table and name, file size and first
entry is set. Then updated header file is written too.
**************/
void Write(char *srcPath, char *destFileName){
    uint32_t first_block;
    uint32_t block_size = 0;
    uint32_t prev_block;
    uint8_t list_index;
    uint8_t first_time = 1;
    uint32_t read_offset = 0;
    uint32_t write_offset = 0;

    //Open source file
    infile = fopen(srcPath, "r+");
    //Set current pointer to start of the file
    fseek(infile, 0, SEEK_SET);
    if(infile != NULL){
        FILE *dest;
        char d[256] = "";
        strcat(d,"./disk/");
        strcat(d, destFileName);
        //Open destination file
        dest = fopen(d, "w+");
        fseek(dest, 0, SEEK_SET);
        //count stores how many bytes are read
        size_t count;
        while((count = fread(buffer, 1, sizeof(buffer), infile)) > 0){
            //printf("%c %c %c %c %zu\n", buffer[0], buffer[1], buffer[2], buffer[3], count);
            uint32_t FAT_index = 0;
            //if whole chunk is read
            if(count > 0){
                //Search for an empty block
                for(int i=0; i<4096; i++){
                    if(FS.header.FAT[i] == 0){
                        FAT_index = i;
                        break;
                    }
                }
                //If an empty entry found for the data
                if(FAT_index != 0){
                    //First Time
                    if(first_time == 1){
                        first_block = FAT_index;
                        //Find empty list index
                        for(int j=0; j<128; j++){
                            if(FS.header.File_List[j].first_block == 0){
                                list_index = j;
                                break;
                            }
                        }
                        //Update first block information
                        FS.header.File_List[list_index].first_block = first_block;
                        //Update file name information
                        for(int m=0; m<strlen(destFileName); m++){
                            FS.header.File_List[list_index].file_name[m] = destFileName[m];
                        }
                        //Add string delimiter to the end of the name
                        FS.header.File_List[list_index].file_name[strlen(destFileName)] = '\0';
                        first_time = 0;
                        prev_block = FAT_index;
                    }

                    //If count < 512 it means that the last chunk is read
                    if(count < 512){
                        FS.header.FAT[prev_block] = FAT_index;
                        //Indicate that it is the last FAT entry.
                        FS.header.FAT[FAT_index] = 0xFFFFFFFF;
                        block_size += count;
                        //Update block size before leaving
                        FS.header.File_List[list_index].file_size = block_size;
                        fwrite(buffer, 1, count, dest);
                        //Close files after write operation
                        fclose(dest);
                        fclose(infile);
                        //Open FS_header.dat and update FS headers
                        FILE *fs_h;
                        fs_h = fopen("./disk/fs_header_info.dat", "w+");
                        if(fs_h != NULL){
                            fwrite(&FS.header, sizeof(FS.header), 1, fs_h);
                        }
                        //If last chunk is read exit from while loop
                        break;
                    }else{
                        //Update table entries
                        FS.header.FAT[prev_block] = FAT_index;
                        prev_block = FAT_index;
                        //Keep writing to destination file
                        fwrite(buffer, 1, sizeof(buffer), dest);
                        //Update offset values
                        read_offset += 512;
                        write_offset += 512;
                        block_size += 512;
                        //Update current pointers for destination and source files
                        fseek(infile, read_offset, SEEK_SET);
                        fseek(dest, write_offset, SEEK_SET);
                    }
                }
            }else{
                printf("Couldn't open source file\n");
            }
        }
    }
}

/*************
Read a file from disk and write to destination file.
*************/
void Read(char *srcFileName, char *destPath){
    //Try to find matching file
    uint8_t match = 0;
    for(int i=0; i<128; i++){
        if(strcmp(FS.header.File_List[i].file_name, srcFileName) == 0){
            match = 1;
            break;
        }
    }
    if(match == 1){
        FILE *f;
        char n[256] = "";
        strcat(n,"./disk/");
        strcat(n, srcFileName);
        //Open source file
        f = fopen(n, "r");
        fseek(f, 0, SEEK_SET);
        if(f != NULL){
            FILE *d;
            //Open destination file with w+
            d = fopen(destPath, "w+");
            fseek(d, 0, SEEK_SET);
            if(d != NULL){
                uint32_t offset = 0;
                size_t count;
                //Read by chunks from source and write to destination
                while((count = fread(&buffer, 1, sizeof(buffer), f)) > 0){
                    if(count == 512){
                        fwrite(&buffer, sizeof(buffer), 1, d);
                        offset += 512;
                        fseek(f, offset, SEEK_SET);
                        fseek(d, offset, SEEK_SET);
                    }else{
                        fwrite(&buffer, count, 1, d);
                        fclose(d);
                        fclose(f);
                    }
                }
            }
        }
    }else{
        printf("Couldn't find file in disk with name: %s\n", srcFileName);
    }

}

/**************
Delete a file from disk
**************/
//https://programmingsimplified.com/c-program-delete-file
void Delete(char *FileName){
    //Delete from file
    int status;
    char fn[256] = "";
    strcat(fn,"./disk/");
    strcat(fn,FileName);
    if((status = remove(fn)) != 0){
        printf("An error occured during Delete operation.\n");
    }

    //Find index of the matching file
    uint8_t index;
    for(int i=0; i<128; i++){
        if(strcmp(FS.header.File_List[i].file_name, FileName) == 0){
            index = i;
            break;
        }
    }

    //Clear the file_list entry of deleted file
    uint32_t first_block = FS.header.File_List[index].first_block;
    FS.header.File_List[index].file_name[0] = 0;
    FS.header.File_List[index].file_size = 0;
    FS.header.File_List[index].first_block = 0;

    //Clear the FAT entry of deleted file by following next pointer until it is 0xFFFFFFFF(EOF)
    uint32_t next= 0;
    uint32_t k = first_block;
    while(next != 0xFFFFFFFF){
        next = FS.header.FAT[k];
        FS.header.FAT[k] = 0;
        k = next;
    }

    //Update header file
    FILE *f;
    f = fopen("./disk/fs_header_info.dat", "w+");
    if(f != NULL){
        fwrite(&FS.header, sizeof(FS.header), 1, f);
    }
}

/*************
List all of the legitimate files(size > 0)
*************/
void List(){
    printf("file name                       file size\n");
    for(int i=0; i< 128; i++){
        if(FS.header.File_List[i].file_size > 0){
            printf("%s", FS.header.File_List[i].file_name);
            for(int j=0; j<(32-strlen(FS.header.File_List[i].file_name)); j++){
                printf(" ");
            }
            printf("%d\n", FS.header.File_List[i].file_size);
        }
    }
}

int main(int argc, char** argv){
    //Read disk name
    //Caution: disk.image is not used. Use "disk"
    disk = argv[1];

    //Header of the file system is stored as a file in the disk.
    //So when program starts it reads the header information from that file
    //if it exists, otherwise it creates an empty header information file(fs_header_info.dat)
    infile = fopen("./disk/fs_header_info.dat","rb");
    if(infile == NULL){
        printf("Couldnt find header info file. New header info file is created.\n");
        //Create empty File System headers
        FS.header.FAT[0] = 0xFFFFFFFF; //System is Little-Endian by default
        FS.header.File_List[0].file_name[0] = 0;
        FS.header.File_List[0].file_size = 0;
        FS.header.File_List[0].first_block = 0;

        for(int i=1; i<4096; i++){
            FS.header.FAT[i] = 0;
            FS.header.File_List[i].file_name[0] = 0;
            FS.header.File_List[i].file_size = 0;
            FS.header.File_List[i].first_block = 0;
        }

        //Open header info file(if it does not exist, it is created)
        outfile = fopen("./disk/fs_header_info.dat", "w+");
        if(outfile != NULL){
            fwrite(&FS.header, sizeof(FS.header), 1, outfile);
            fclose(outfile);
        }

    }else{
        //If header info file exists, read info and store it in FS.header structure
        while(fread(&FS.header, sizeof(FS.header), 1, infile));
        fclose(infile);
    }

    //Read arguments and run corresponding function.
    if(argv[2] != NULL){
        if(strcmp(argv[2],"-format") == 0){
            Format();
        }else if(strcmp(argv[2],"-write") == 0){
            if(argv[3] != NULL && argv[4] != NULL){
                src_file = argv[3];
                dest_file = argv[4];
                Write(src_file, dest_file);
            }
        }else if(strcmp(argv[2],"-read") == 0){
            if(argv[3] != NULL && argv[4] != NULL){
                src_file = argv[3];
                dest_file = argv[4];
                Read(src_file, dest_file);
            }
        }else if(strcmp(argv[2],"-delete") == 0){
            if(argv[3] != NULL){
                src_file = argv[3];
                Delete(src_file);
            }
        }else if(strcmp(argv[2],"-list") == 0){
            List();
        }
    }

//    for(int i=0; i<50; i++){
//        printf("%x %d %s\n", FS.header.FAT[i], FS.header.File_List[i].file_size, FS.header.File_List[i].file_name);
//    }


    return 0;
}

