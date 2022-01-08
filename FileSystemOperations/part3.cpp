#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <iostream> 
#include <vector>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

using namespace std;

int const timeSize=25;
int const directSize=10;
int const fileSize=14;

struct SuperBlock {
    int numberInode;
    int inodeAddr;
    int blockAddr;
    int bitMapAddr;
    int blockNumber;
    int blockBitmapAddr;
    int blockSize;
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

struct dataType{
    int nodeNumber;
    char fileName[20];
};
int byte=1024;

int blockNumberSB=0;
int initBlockAddr;
int initInodeNumber=0;
int startInodeAddr=0;
int blockBitmapAddr=0;
int inodeBitmapAddr=0;
int createdBefore=-1;
int changeAuthority=0;//deişiklikler için yazıp yazılmayacağına karar verir
int operationType=1;//rmdir ve mkdir icim
int oneBlockSize=0;


void mkdir(FILE *fp,char* fileName);
void rmdir(FILE *fp,char* fileName);
void list(FILE *fp,char* fileName);
void dumpe2fs(FILE *fp);
void writeFile(FILE *fp,char* path,char* fileName);
void readFile(FILE *fp,char* path,char* fileName);
void deleteCommand(FILE *fp,char* path);


int tripInodesAndBlocks(FILE *fp,char* fileName);
int findInode(FILE *fp,int inodeNumber);
void fillInode(FILE *fp,int emptyInodeAddr,int inodeNumber,char* fileName,int blockAddr);
int checkCreated(FILE *fp,char* fileName,int inodeNumber,int emptyInode);
int findInodeBlock(FILE *fp,char* fileName,int emptyInode,int blockAddres,int inodeno);
int findEmptyBlock(FILE *fp,int blockNumber);
int checkCanDelete(FILE *fp,char* fileName,int deleteFlag);
void allInformation(FILE *fp);
void infoAboutSystem(FILE *fp);
int calculaEmptyInitNum(FILE *fp);
int calculaEmptyBlockNum(FILE *fp);
int numberOfFilesAndDirect(FILE *fp,int num);
void writeInFile(FILE *fdRead,FILE *fp,int blockAddres,int inodeNumber);
int getEmptyDirectIndex(Inode read);
void readCommentWriteInLinuxFile(FILE *fp,FILE *fdWrite,Inode read);
void deleteCommentInFileSystem(FILE *fp,Inode read);
void printInfoFile(FILE *fp);
void clearContentBlocks(FILE *fp,int inodeNo,char* path);
int changeCopyContent=0;



int main(int argc, char **argv) {
	if(((strcmp(argv[2],"write")==0) || strcmp(argv[2],"read")==0) && argc!=5){
        printf("Usage: write comment wrong inputs %s [directory]\n", *argv);
        exit(EXIT_FAILURE);
    }
    else if(strcmp(argv[2],"dumpe2fs")==0 && argc!=3){
        printf("Usage: dumpe2fs comment wrong inputs please give 3 input \n");
        exit(EXIT_FAILURE);
    }
    else if(strcmp(argv[2],"dumpe2fs")!=0 && strcmp(argv[2],"write")!=0 && strcmp(argv[2],"read")!=0 && argc !=4) {
        printf("Usage: %s [directory]\n", *argv);
        exit(EXIT_FAILURE);
    }
    FILE *fp;
    fp = fopen(argv[1], "r+");
	allInformation(fp);
    if(strcmp(argv[2],"list")==0){
   		operationType=3;
    	list(fp,argv[3]);
    }
    else if(strcmp(argv[2],"mkdir")==0){
		mkdir(fp,argv[3]);
    }
    else if(strcmp(argv[2],"rmdir")==0){
		rmdir(fp,argv[3]);
    }
    else if(strcmp(argv[2],"dumpe2fs")==0){
		dumpe2fs(fp);
    }
    else if(strcmp(argv[2],"write")==0){
   		operationType=4;
		writeFile(fp,argv[3],argv[4]);
    }
    else if(strcmp(argv[2],"read")==0){
   		operationType=5;
    	readFile(fp,argv[3],argv[4]);
    }
    else if(strcmp(argv[2],"del")==0){
    	operationType=6;
    	deleteCommand(fp,argv[3]);
    }
    else{
    	printf("Usage : Wrong operation %s \n",argv[2] );
    	exit(EXIT_FAILURE);
    }
    fclose(fp);
    return 0;
}

void mkdir(FILE *fp,char* fileName){
    operationType=1;
  	tripInodesAndBlocks(fp,fileName);
}

void rmdir(FILE *fp,char* fileName){
	operationType=0;
  	tripInodesAndBlocks(fp,fileName);
}

void deleteCommand(FILE *fp,char* path){
	int size=strlen(path);
	char second[fileSize];
	int totalCounter=0,i=0,number=1;
	char delim[] = "/";
	for(i=0;i<size;i++){
		if(path[i]=='/'){
			totalCounter++;
		}
	}

	Inode read;
	int inodeNumber=0,blockAddres=0;
	printf("path %s \n",path );
	inodeNumber=tripInodesAndBlocks(fp,path);
	printf("inodeNumber %d \n",inodeNumber );
	if(inodeNumber==-1){
		printf("Error this file can not delete\n");
		exit(EXIT_FAILURE);
	}
	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); 
	fread(&read, sizeof(Inode),1, fp);
	char *ptrSecond = strtok(path, delim);
	if(totalCounter==1){
		changeAuthority=1;
	}

	while(ptrSecond != NULL)
	{
		strcpy(second,ptrSecond);
		ptrSecond = strtok(NULL, delim);
	}
	if(read.type==0){
		printf("ne oluyor\n");
		blockAddres=read.blockAddr;
		i=0;
		while(1){			
			fseek(fp,blockBitmapAddr+((blockAddres)*sizeof(int)), SEEK_SET); 
			number=-1;
			fwrite(&number, sizeof(number),1, fp);  //delete block in bitmap
			if(read.directBlocks[i]!=-1 && i<directSize){
				blockAddres=read.directBlocks[i];
			}
			else{
				break;
			}
			i++;
		}
		fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); 
		deleteCommentInFileSystem(fp,read);
		fseek(fp,inodeBitmapAddr+((inodeNumber)*sizeof(int)) , SEEK_SET);
		number=0;
		fwrite(&number, sizeof(number),1, fp);  //delete inode in bitmap
	}
}

void deleteCommentInFileSystem(FILE *fp,Inode read){
	int count=0,index=0,number=-1;
	char writeByte='\0';
	while(1){
		fwrite(&writeByte, sizeof(writeByte),1, fp);
		count++;
		if(count>=oneBlockSize*byte){
			if(read.directBlocks[index]!=-1){
				fseek(fp,blockBitmapAddr+((read.directBlocks[index]-1)*sizeof(int)), SEEK_SET); 
				fwrite(&number, sizeof(number),1, fp);  //delete block in bitmap
				fseek(fp,initBlockAddr+((read.directBlocks[index])*oneBlockSize*byte) , SEEK_SET); 
				index++;
			}
			else{
				break;
			}
			count=0;
		}
	}
}

void readFile(FILE *fp,char* path,char* fileName){
	int size=strlen(path);
	char second[fileSize];
	int totalCounter=0,i=0;
	char delim[] = "/";
	for(i=0;i<size;i++){
		if(path[i]=='/'){
			totalCounter++;
		}
	}

	Inode read;
	int inodeNumber=0,blockAddres=0;
  	inodeNumber=tripInodesAndBlocks(fp,path);
  	if(inodeNumber==-1){
  		printf("Error , read path is invalid.\n");
  		exit(EXIT_FAILURE);
  	}
	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); 
	fread(&read, sizeof(Inode),1, fp);
	char *ptrSecond = strtok(path, delim);
	if(totalCounter==1){
		changeAuthority=1;
	}
	FILE *fdWrite;
	if ((fdWrite = fopen(fileName, "w")) == NULL){
        printf("Error : %s file can not open to write\n",fileName );
        exit(EXIT_FAILURE);
	}
	while(ptrSecond != NULL)
	{
		strcpy(second,ptrSecond);
		ptrSecond = strtok(NULL, delim);
	}
	if(read.type==0){
		blockAddres=read.blockAddr;
		fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); 
		readCommentWriteInLinuxFile(fp,fdWrite,read);
	}
	fclose(fdWrite);
}

void readCommentWriteInLinuxFile(FILE *fp,FILE *fdWrite,Inode read){
	int count=0,index=0;
	char readByte;
	while(1){
		fread(&readByte, sizeof(readByte),1, fp);
		if(readByte!='\0')
		fwrite(&readByte, sizeof(readByte),1, fdWrite);
		count++;
		if(count>=oneBlockSize*byte){
			if(read.directBlocks[index]!=-1){
				fseek(fp,initBlockAddr+((read.directBlocks[index])*oneBlockSize*byte) , SEEK_SET); 
				index++;
			}
			else{
				break;
			}
			count=0;
		}
		if(readByte=='\0' ){

			break;
		}
	}
}

void writeFile(FILE *fp,char* path,char* fileName){
	FILE *fdRead;
	int size=strlen(path);
	if ((fdRead = fopen(fileName, "r")) == NULL){
        printf("Error : %s file can not open to read\n",fileName );
        exit(EXIT_FAILURE);
	}
	char temp[size];
	char second[fileSize];
	int totalCounter=0,tempCounter=0,i=0,number=1,emptyBlockAddr=0,emptyInodeAddr=0;
	char delim[] = "/";
	for(i=0;i<size;i++){
		if(path[i]=='/'){
			totalCounter++;
		}
	}
	for(i=0;i<size;i++){
		if(path[i]=='/'){
			tempCounter++;
		}
		if(totalCounter==1){
			temp[i]=path[i];
		}
		if(tempCounter!=totalCounter){
			temp[i]=path[i];
		}
		else{
			if(totalCounter==1)
				i++;
			temp[i]='\0';	
			break;
		}
	}
	Inode read;
	int inodeNumber=0,blockAddres=0,emptyInode=0;
	operationType=4;
    fseek(fp,inodeBitmapAddr , SEEK_SET); /* rewind(fp) */
	emptyInode=findInode(fp,initInodeNumber);

  	inodeNumber=tripInodesAndBlocks(fp,temp);
	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); /* rewind(fp) */
	fread(&read, sizeof(read),1, fp);
	int newCreatedBlock;
	char *ptrSecond = strtok(path, delim);
	if(totalCounter==1){
		changeAuthority=1;
	}

	while(ptrSecond != NULL)
	{
		strcpy(second,ptrSecond);
		ptrSecond = strtok(NULL, delim);
	}
	if(read.type==1){
		blockAddres=read.blockAddr;
		fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
		newCreatedBlock=findInodeBlock(fp,second,emptyInode,blockAddres,inodeNumber);
		if(newCreatedBlock==-1 && totalCounter!=1){
			fclose(fp);
			return ;
		}
	}
	if(createdBefore!=-1){
		fseek(fp,inodeBitmapAddr+(emptyInode*sizeof(int)) , SEEK_SET);
		fwrite(&number, sizeof(number),1, fp);
		fseek(fp,blockBitmapAddr, SEEK_SET); 
		emptyBlockAddr=findEmptyBlock(fp,blockNumberSB);
		fseek(fp, startInodeAddr+(emptyInode*sizeof(Inode)), SEEK_SET);
		fillInode(fp,emptyInodeAddr,emptyInode,second,emptyBlockAddr);
		writeInFile(fdRead,fp,emptyBlockAddr,emptyInode);
	}

}

void writeInFile(FILE *fdRead,FILE *fp,int blockAddres,int inodeNumber){
	struct stat fileStat;
	fstat(fileno(fdRead), &fileStat);
	fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
	char readByte;
	int counter=0,number=1,emptyBlockAddr=0,i=0,fileSize=0,count=0,index=0,totalSize=0,size=0;
	vector<int> blockVect;
	Inode read;
	fileSize=fileStat.st_size;
	if(fileSize>(directSize*oneBlockSize)){
		printf("Direct access full .Single,double,triple access should be bot not implemented.\n");
		exit(EXIT_FAILURE);
	}

	while(count<fileSize){
		fread(&readByte, sizeof(readByte),1, fdRead);
		fwrite(&readByte, sizeof(readByte),1, fp);
		counter++;
		if(counter>=oneBlockSize*byte){
			counter=0;
		    fseek(fp, blockBitmapAddr, SEEK_SET);
			emptyBlockAddr=findEmptyBlock(fp,blockNumberSB);
		    fseek(fp, blockBitmapAddr+(emptyBlockAddr*sizeof(int)), SEEK_SET);
			fwrite(&number, sizeof(number),1, fp);
			fseek(fp,initBlockAddr+((emptyBlockAddr)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
		    blockVect.push_back(emptyBlockAddr);
		}
		totalSize++;
		count++;
	}

	fseek(fp, startInodeAddr+(inodeNumber*sizeof(Inode)), SEEK_SET);
	fread(&read,sizeof(Inode),1,fp);
	size=blockVect.size();
	for(i=0;i<size;i++){
		index=getEmptyDirectIndex(read);
		if(index==-1){
			printf("Direct access full .Single,double,triple access should be bot not implemented.\n");
			exit(EXIT_FAILURE);
		}
		read.directBlocks[index]=blockVect[i];
	}
	fseek(fp, (sizeof(Inode)*-1), SEEK_CUR);
	read.size=totalSize;
	fwrite(&read, sizeof(Inode), 1, fp);
}

int getEmptyDirectIndex(Inode read){
	int i=0;
	for(i=0;i<directSize;i++){
		if(read.directBlocks[i]==-1){
			return i;
		}
	}
	return -1;
}

void dumpe2fs(FILE *fp){
	cout<<"Block count "<<blockNumberSB<<endl;
	cout<<"Inode count "<<initInodeNumber<<endl;
	cout<<"Free Block "<<calculaEmptyBlockNum(fp)<<endl;
	cout<<"Free Inode "<<calculaEmptyInitNum(fp)<<endl;
	cout<<"Number of Files "<<numberOfFilesAndDirect(fp,0)<<endl;
	cout<<"Number of Directories "<<numberOfFilesAndDirect(fp,1)<<endl;
	cout<<"Block Size "<<oneBlockSize<<endl;
	printInfoFile(fp);
}

void printInfoFile(FILE *fp){
	fseek(fp,inodeBitmapAddr, SEEK_SET); /* rewind(fp) */
	int i=0,number=0,index=0,t=0,currentAddr=0;
	Inode read;
	printf("Filename  ");
	printf("\tInode no  ");
	printf("\tBlock numbers  \n");
	for(i=0;i<initInodeNumber;i++){
		fread(&number, sizeof(int),1, fp);
		currentAddr=ftell(fp);
		if(number==1){
			fseek(fp,startInodeAddr+(i*sizeof(Inode)) , SEEK_SET); /* rewind(fp) */
			fread(&read, sizeof(Inode),1, fp);
			printf("%s ",read.fileName);
			printf("\t\t    %d ",read.number);
			printf("\t\t      %d ",read.blockAddr);
			index=getEmptyDirectIndex(read);
			for(t=0;t<index;t++){
				printf("- %d",read.directBlocks[t]);
			}
			printf("\n");
			fseek(fp,currentAddr , SEEK_SET); /* rewind(fp) */
		}
	}
}


int calculaEmptyBlockNum(FILE *fp){
	fseek(fp,blockBitmapAddr, SEEK_SET); /* rewind(fp) */
	int i=0,number=0,counter=0;
	for(i=0;i<blockNumberSB;i++){
		fread(&number, sizeof(number),1, fp);
		if(number==-1){
			counter++;
		}

	}
	return counter;
}

int calculaEmptyInitNum(FILE *fp){
	fseek(fp,inodeBitmapAddr, SEEK_SET); /* rewind(fp) */
	int i=0,number=0,counter=0;
	for(i=0;i<initInodeNumber;i++){
		fread(&number, sizeof(number),1, fp);
		if(number==0){
			counter++;
		}
	}
	return counter;
}

int numberOfFilesAndDirect(FILE *fp,int num){
	fseek(fp,startInodeAddr , SEEK_SET); /* rewind(fp) */
	int i=0,counter=0;
	Inode read;
	for(i=0;i<initInodeNumber;i++){
		fread(&read, sizeof(read),1, fp);
		if(read.type==0 && num==0){
			counter++;
		}
		else if(read.type==1 && num==1) {
			counter++;
		}

	}
	return counter;
}


void list(FILE *fp,char* fileName){
	Inode read;
	int inodeNumber=0,blockAddres=0;
  	inodeNumber=tripInodesAndBlocks(fp,fileName);
	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); /* rewind(fp) */
	fread(&read, sizeof(read),1, fp);
	blockAddres=read.blockAddr;
	fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
	infoAboutSystem(fp);
}

void infoAboutSystem(FILE *fp){
	dataType data;
	int count=0;
	Inode read;
	int currentAddr=0;
	while(count<byte){
		count+=fread(&data, sizeof(data),1, fp);
		if(strcmp(data.fileName,"")==0 ){
			return;
		}
		if(!strcmp(data.fileName,"/")==0){
			currentAddr=(int)ftell(fp);
			fseek(fp, startInodeAddr+(data.nodeNumber*sizeof(Inode)), SEEK_SET);
			fread(&read, sizeof(read),1, fp);
			printf("%d ",read.size);
			printf(" %s ",read.lastDate);
			printf(" %s ",read.time);
			printf("%s\n",read.fileName);
	    	fseek(fp, currentAddr, SEEK_SET);		
		}
	}
}
void allInformation(FILE *fp){
	struct SuperBlock rec1;
	fread(&rec1, sizeof(rec1),1, fp);
	initInodeNumber=rec1.numberInode;
	initBlockAddr=rec1.blockAddr;
	blockNumberSB=rec1.blockNumber;
	blockBitmapAddr=rec1.blockBitmapAddr;
	inodeBitmapAddr=rec1.bitMapAddr;
	oneBlockSize=rec1.blockSize;
    startInodeAddr=rec1.inodeAddr;
}

int tripInodesAndBlocks(FILE *fp,char* name){
	int emptyInode=0,emptyInodeAddr=0,inodeAddr=0,number=1,emptyBlockAddr=0,i=0,size=0;
	int init_size = strlen(name);
	char delim[] = "/";
	char delim2[] = "/";
	int totalCounter=0,counter=0;
	char second[init_size];
	char allPath[init_size];
	strcpy(allPath,name);
	strcpy(second,name);
    if(strcmp(allPath,"/")==0 && operationType==0){
		printf("Error : Root can not delete.\n");
		exit(EXIT_FAILURE );	
	}
	else if(strcmp(allPath,"/")==0 && operationType==1){
		printf("Error : Root can create again.\n");
		exit(EXIT_FAILURE );	
	}
    fseek(fp,inodeBitmapAddr , SEEK_SET); 
	if(operationType==0){
		emptyInode=1;
	}
	else{
		emptyInode=findInode(fp,initInodeNumber);
	}
	size=strlen(name);
	for(i=0;i<size;i++){
		if(name[i]=='/'){
			totalCounter++;
		}
	}
	char *ptrSecond = strtok(name, delim);
	while(ptrSecond != NULL)
	{
		counter++;
		if(totalCounter!=counter){
			changeAuthority=0;
		}
		else{
			changeAuthority=1;
		}
		inodeAddr=checkCreated(fp,ptrSecond,inodeAddr,emptyInode);
		if(inodeAddr==-1){
			if(operationType==1){
				printf("Can't craeate this %s file. Wrong path input. \n",allPath);
				exit(EXIT_FAILURE );				
			}
			else if(operationType==3){
				printf("Cant't listed this %s path. Wrong path input. \n",allPath);
				exit(EXIT_FAILURE );	
			}
			else if (operationType==0){
				printf("Error : File can not delete. Wrong path %s input.\n",allPath);	
				exit(EXIT_FAILURE );
			}
		}
		strcpy(second,ptrSecond);
		ptrSecond = strtok(NULL, delim2);
	}
	if(createdBefore!=-1){
    	fseek(fp,inodeBitmapAddr+(emptyInode*sizeof(int)) , SEEK_SET); 
		fwrite(&number, sizeof(number),1, fp);
    	fseek(fp,blockBitmapAddr, SEEK_SET); 
		emptyBlockAddr=findEmptyBlock(fp,blockNumberSB);
    	fseek(fp, startInodeAddr+(emptyInode*sizeof(Inode)), SEEK_SET);
		fillInode(fp,emptyInodeAddr,emptyInode,second,emptyBlockAddr);
	}
	else if(operationType==1){
		printf("Error : File Exist\n");
		exit(EXIT_FAILURE );
	}
	else if (operationType==0){
		printf("Error : File can not delete.It is not empty.\n");
		exit(EXIT_FAILURE);	
	}

	return inodeAddr;
}

int checkCreated(FILE *fp,char* fileName,int inodeNumber,int emptyInode){
	Inode read;
	int blockAddres;

	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); /* rewind(fp) */

	fread(&read, sizeof(read),1, fp);
	int newCreatedBlock;
	if(read.type==1){
		blockAddres=read.blockAddr;
		fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
		newCreatedBlock=findInodeBlock(fp,fileName,emptyInode,blockAddres,inodeNumber);
		if(newCreatedBlock==-1){
			return -1;
		}
	}
	return newCreatedBlock;
}

int findInodeBlock(FILE *fp,char* fileName,int emptyInode,int blockAddres,int inodeno){
	dataType data;
	int count=0,counterData=1,i=0,controlDelete=0,emptyBlockAddr,number=1,index=0,size=0;
	char x='\0';
	Inode read,emtpyInode;
	while(count<byte){
		count+=fread(&data, sizeof(data),1, fp);
		if(strcmp(data.fileName,"")==0 ){
			if(changeAuthority==0){
				return -1;
			}
			if(operationType==1 || operationType==4){
				fseek(fp,-1*sizeof(data),SEEK_CUR);
				data.nodeNumber=emptyInode;
				strcpy(data.fileName,fileName);
				fwrite(&data, sizeof(data), 1, fp);
				createdBefore=0;
				return 1;
			}
		}
		if(strcmp(data.fileName,(fileName))==0 ){
			if(changeAuthority==1 && ((operationType==1) || (operationType==4))){
				if(operationType==4){
					changeCopyContent=1;
				}
				else{
					createdBefore=-1;
				}
			}
			else if(operationType==0 && changeAuthority==1){
		    	fseek(fp, startInodeAddr+(data.nodeNumber*sizeof(Inode)), SEEK_SET);
	    		fread(&read, sizeof(read),1, fp);
	    		if(read.type==1){
	    			fseek(fp, initBlockAddr+((read.blockAddr)*oneBlockSize*byte), SEEK_SET);
	    			controlDelete=checkCanDelete(fp,fileName,0);
	    			if(controlDelete==1){
	    				fseek(fp,blockBitmapAddr, SEEK_SET); /* rewind(fp) */
  						findEmptyBlock(fp,read.blockAddr);
					    fseek(fp,inodeBitmapAddr , SEEK_SET); /* rewind(fp) */
						findInode(fp,data.nodeNumber);
						fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
						controlDelete=checkCanDelete(fp,fileName,1);
				    	fseek(fp, startInodeAddr+(data.nodeNumber*sizeof(Inode)), SEEK_SET);
						fwrite (&emtpyInode, sizeof(struct Inode), 1, fp);//remove inode
	    			}
	    		}
				fseek(fp,-1*sizeof(data),SEEK_CUR);
				size=sizeof(data);
				for(i=0;i<size;i++){
		    		fwrite (&x, sizeof(x), 1, fp);  //delete datablock 
				}
			}
			else if(operationType==6 && changeAuthority==1){
				fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */
				controlDelete=checkCanDelete(fp,fileName,1);
			}
			return data.nodeNumber;
		}
		counterData++;
	}
	if(operationType==1){			/*Block is full directories*/
	    fseek(fp, blockBitmapAddr, SEEK_SET);
		emptyBlockAddr=findEmptyBlock(fp,blockNumberSB);
	    fseek(fp, blockBitmapAddr+(emptyBlockAddr*sizeof(int)), SEEK_SET);
		fwrite(&number, sizeof(number),1, fp);
		fseek(fp,initBlockAddr+((emptyBlockAddr)*oneBlockSize*byte) , SEEK_SET); /* rewind(fp) */

		fseek(fp, startInodeAddr+(inodeno*sizeof(Inode)), SEEK_SET);
		fread(&read,sizeof(Inode),1,fp);
		index=getEmptyDirectIndex(read);
		if(index==-1){
			printf("Direct access full.Single,double,triple access should be bot not implemented.\n");
			exit(EXIT_FAILURE);
		}
		read.directBlocks[index]=emptyBlockAddr;
		
		fseek(fp, (sizeof(Inode)*-1), SEEK_CUR);
		fwrite(&read, sizeof(Inode), 1, fp);
	}	

	//--------------burda yeni block numarası alınıcak ve oraya yazılıp inode güncellenicek.
	createdBefore=-1;
	return -1;
}

void clearContentBlocks(FILE *fp,int inodeNo,char* path){
	int size=strlen(path);
	char second[fileSize];
	int totalCounter=0,i=0,number=1;
	char delim[] = "/";
	for(i=0;i<size;i++){
		if(path[i]=='/'){
			totalCounter++;
		}
	}

	Inode read;
	int inodeNumber=0,blockAddres=0;
	inodeNumber=tripInodesAndBlocks(fp,path);
	fseek(fp,startInodeAddr+((inodeNumber)*sizeof(Inode)) , SEEK_SET); 
	fread(&read, sizeof(Inode),1, fp);
	char *ptrSecond = strtok(path, delim);
	if(totalCounter==1){
		changeAuthority=1;
	}

	while(ptrSecond != NULL)
	{
		strcpy(second,ptrSecond);
		ptrSecond = strtok(NULL, delim);
	}
	if(read.type==0){
		blockAddres=read.blockAddr;
		i=0;
		while(1){			
			fseek(fp,blockBitmapAddr+((blockAddres-1)*sizeof(int)), SEEK_SET); 
			number=-1;
			fwrite(&number, sizeof(number),1, fp);  //delete block in bitmap
			if(read.directBlocks[i]!=-1 && i<directSize){
				blockAddres=read.directBlocks[i];
			}
			else{
				break;
			}
			i++;
		}
		fseek(fp,initBlockAddr+((blockAddres)*oneBlockSize*byte) , SEEK_SET); 
		deleteCommentInFileSystem(fp,read);
		fseek(fp,inodeBitmapAddr+((inodeNumber)*sizeof(int)) , SEEK_SET);
		number=0;
		fwrite(&number, sizeof(number),1, fp);  //delete inode in bitmap
	}
}

int checkCanDelete(FILE *fp,char* fileName,int deleteFlag){
	dataType data;
	int count=0,counter=0,j=0,size=0;
	char x='\0';
	while(count<byte){
		count+=fread(&data, sizeof(data),1, fp);
		if(strcmp(data.fileName,"")==0 && changeAuthority==1){ //siliniceğine karar verildiği yer
			if(counter==0){
				size=sizeof(data);
				for(j=0;j<2*size;j++){
	    			fwrite (&x, sizeof(x), 1, fp);
		    	}
		    	return 1;
			}
			return 0;
		}
		if(strcmp(data.fileName,fileName)==0 && changeAuthority==1){ //siliniceğine karar verildiği yer
			if(deleteFlag==1){
				size=sizeof(data);
				fseek(fp,-1*sizeof(data),SEEK_CUR);
				for(j=0;j<size;j++){
		    		fwrite (&x, sizeof(x), 1, fp);
			    }
		    	return 1;
			}
		}
		if(!(strcmp(data.fileName,".")==0 || strcmp(data.fileName,"..")==0)  ){
			counter++;
		}
	}
	return 0;
} 

void fillInode(FILE* fp,int emptyInodeAddr,int inodeNumber,char* fileName,int blockAddr){
    int number=1,i=0;
	Inode inodeNew,example;
	inodeNew.number=inodeNumber;
	inodeNew.size=oneBlockSize*byte;    
    time_t now;
    time(&now);

    struct tm* now_tm;
    now_tm = localtime(&now);

    strftime (inodeNew.lastDate, 80, "%m-%d %H:%M:%S", now_tm);
	strcpy(inodeNew.time,inodeNew.lastDate );
	inodeNew.blockAddr=blockAddr;
	strcpy(inodeNew.fileName,fileName);
	if(operationType==4){
		inodeNew.type=0;
	}
	else{
		inodeNew.type=1;	
	}
	for(i=0;i<directSize;i++){
		inodeNew.directBlocks[i]=-1;
	}
	fwrite(&inodeNew, sizeof(Inode), 1, fp);
    fseek(fp, (sizeof(Inode)*-1), SEEK_CUR);
    fread(&example, sizeof(example),1, fp);
    fseek(fp, blockBitmapAddr+(blockAddr*sizeof(int)), SEEK_SET);
	fwrite(&number, sizeof(number),1, fp);
//. ve .. içinde dizin bilgisini koyulacak burda
}


int findInode(FILE *fp,int inodeNumber){
	int i=0,number=0;
	for(i=0;i<inodeNumber;i++){
		fread(&number, sizeof(number),1, fp);
		if(operationType==0){
			if(i==inodeNumber-1){
				number=0;
				fwrite(&number, sizeof(number),1, fp);
				return 1;
			}
		}
		else{
			if(number==0){
				number=1;
				return i;
			}
		}
	}
	return -1;
}

int findEmptyBlock(FILE *fp,int blockNumber){
	int i=0,number=0;

	for(i=0;i<blockNumber;i++){
		fread(&number, sizeof(number),1, fp);
		if(operationType==0){
			if(i==blockNumber-1){
				number=-1;
				fwrite(&number, sizeof(number),1, fp);
				return 1;
			}
		}
		else{
			if(number==-1){
				return i;
			}
		}
	}
	return -1;
} 