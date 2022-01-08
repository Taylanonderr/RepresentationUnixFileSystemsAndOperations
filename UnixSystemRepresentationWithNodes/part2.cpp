#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <iostream> 
#include <vector>
#include <string.h>
#include <time.h>

using namespace std;
int const timeSize=25;

int totalSize=1024;
int byte=1024;
int const directSize=10;
int const fileSize=14;
struct dataType{
    int nodeNumber;
    char fileName[fileSize];
};


struct Inode {
    int size;
    int number;
    char lastDate[timeSize];
    char time[timeSize];
    char fileName[fileSize];
    int blockAddr;
	int directBlocks[directSize];
    int type;
};

struct SuperBlock {
    int numberInode;
    int inodeAddr;
    int blockAddr;
    int bitMapAddr;
    int blockNumber;
    int blockBitmapAddr;
    int blockSize;
};

struct Block {
 	dataType data[2];
};



void writeFile(char *fileName,Inode inodes,SuperBlock sb,int blockSize,int inodeNumber);

int main(int argc, char **argv) {
    if(argc !=4) {
        printf("Usage: %s [directory]\n", *argv);
        exit(0);
    }
    int blockSize=atoi(argv[1]);
    int inodeNumber=atoi(argv[2]);
   	SuperBlock sB;
   	Inode a;
   	sB.numberInode=inodeNumber;
	writeFile(argv[3],a,sB,blockSize,inodeNumber);
    return 0;
}

void writeFile(char *fileName,Inode inodes,SuperBlock sb,int blockSize,int inodeNumber ){
	FILE* fd=fopen(fileName,"w");
    time_t rawtime;
	int size=0,inodeSize=0,bitmapSize=0;
	int i=0, blockNumber,j=0;
	if (fd == NULL) 
    { 
        puts("Couldn't open file"); 
        exit(0); 
    } 
    else
    { 
    	inodeSize=sizeof(struct Inode)*inodeNumber;
    	size=sizeof(struct SuperBlock);
    	//bitmap.bitmapArr=(int*)calloc(inodeNumber,sizeof(int));
    	bitmapSize+=sizeof(int)*inodeNumber;
    	inodeSize+=size;

    	sb.numberInode=inodeNumber;
	    sb.inodeAddr=size;
	    sb.blockAddr=inodeSize+bitmapSize;
	    sb.bitMapAddr=inodeSize;
	    size=inodeSize+bitmapSize;
       	blockNumber=(((totalSize*byte)-size)/byte)/blockSize;
	    sb.blockNumber=blockNumber;
	    sb.blockSize=blockSize;
    	fwrite (&sb, sizeof(struct SuperBlock), 1, fd);
		Inode inodeRoot;

    	for(i=0;i<directSize;i++){
    		inodeRoot.directBlocks[i]=-1;
    		inodes.directBlocks[i]=-1;
    	}
    	for(i=0;i<inodeNumber;i++){
    		if(i==0){
    			time(&rawtime);
    			inodeRoot.number=0;
    			inodeRoot.blockAddr=0;
    			inodeRoot.size=1;
    			inodeRoot.type=1;
				strcpy(inodeRoot.fileName,"/");
    			fwrite (&inodeRoot, sizeof(struct Inode), 1, fd);

    		}
    		else{
    			fwrite (&inodes, sizeof(struct Inode), 1, fd);
	    	}

    	}
		i=1;
		fwrite (&i, sizeof(int), 1, fd);	
		i=0;
    	for(j=0;j<inodeNumber-1;j++){
			fwrite (&i, sizeof(int), 1, fd);	
    	}		
		i=1;
		sb.blockBitmapAddr=ftell (fd);
		fwrite (&i, sizeof(int), 1, fd);	
		i=-1;

		for(j=0;j<blockNumber-1;j++){
			fwrite (&i, sizeof(int), 1, fd);	
    	}

		char x='\0';
    	for(i=0;i<blockNumber;i++){
    		if(i==0){
    			dataType blockData;
    			blockData.nodeNumber=-1;
    			strcpy(blockData.fileName,"");
				for(j=0;j<byte*blockSize;j++){
	    			fwrite (&x, sizeof(x), 1, fd);
		    	}
    		}
    		else{
				for(j=0;j<byte*blockSize;j++){
	    			fwrite (&x, sizeof(x), 1, fd);
		    	}
    		}
	    	size+=blockSize*byte;
    	}

   		fseek(fd,0 , SEEK_SET); /* rewind(fp) */
   		sb.blockAddr=inodeSize+bitmapSize+(sizeof(int)*blockNumber);
    	fwrite (&sb, sizeof(struct SuperBlock), 1, fd);

    } 
   	fclose(fd);

}
