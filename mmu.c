#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define PAGE_ENTRIES 256 
#define PAGE_BITS 8 
#define PAGE_SIZE 256 
#define TLB_ENTRIES 16 
#define FRAME_SIZE 256 

int logicalAddress;
int physicalAddress;
int pageNumber;
int offset;
int frameNumber;
signed char *value;
char line[16];
int hitCounter = 0;
int totalLinesRead = 0;
int pageFaultCounter = 0;
int TLBSize = 0;
int mem_index = 0;
int pageTableSize = 0;
int  pageTable_pgno[PAGE_SIZE];
int  pageTable_frno[PAGE_SIZE];
int  TLB_pgno[TLB_ENTRIES];
int  TLB_frno[TLB_ENTRIES];


void invalidateTLB();
void invalidatePageTable();
void selectOutputFile(int size);
int  scanTLB(int pg_num);
int  scanPageTable(int pg_num);
int  calculatePhysicalAddress(int frameNumber, int offset);
int  calculatePageNumber(int logicalAddress);
int  calculateOffset(int logicalAddress);
void insertIntoTLB(int pg_num, int frm_num);
void insertIntoPageTable(int pg_num, int frm_num);
void update();


int main(int argc, char *argv[]) {
    FILE* outputFile;
    FILE *addresses;
    FILE *BackingStore;

    int FRAME_ENTRIES = atoi(argv[1]);  
    signed char physical_memory[FRAME_ENTRIES][FRAME_SIZE];  


    int size = atoi(argv[1]);
    if(size == 256){
        outputFile = fopen("output256.csv", "w+");
    }    
    else{
        outputFile = fopen("output128.csv", "w+");
    }
   
        
        addresses = fopen(argv[3], "r");
        BackingStore = fopen(argv[2], "rb");

        invalidateTLB();
        invalidatePageTable();

        while(fgets(line, sizeof(line), addresses)){

            totalLinesRead++;
            logicalAddress = atoi(line); // string to integer conversion
            pageNumber = calculatePageNumber(logicalAddress); 
            offset = calculateOffset(logicalAddress); 


             frameNumber = scanTLB(pageNumber);
            
            // page found in TLB 
            if(frameNumber == -1){
                frameNumber = scanPageTable(pageNumber);

                //page found in the table
                if(frameNumber != -1){
                    insertIntoTLB(pageNumber, frameNumber);
                    update();

                }
                //page not found in table 
                else{
                    pageFaultCounter++;
                    
                    // getting from Backing store
                     fseek(BackingStore, pageNumber * PAGE_SIZE, SEEK_SET);

                    // Physical Memomry full
                    if(mem_index >= FRAME_ENTRIES){
                        //printf("Main Memory FUll");
                         frameNumber = pageTable_frno[0];
                         pageTable_pgno[0] = pageNumber;

                        // updating 
                        int i;
                        for(i = 0; i < (pageTableSize - 1); i++){
                            pageTable_pgno[i] = pageTable_pgno[i+1];
                            pageTable_frno[i] = pageTable_frno[i+1];    
                        }
                        pageTable_pgno[pageTableSize - 1]  = pageNumber;
                        pageTable_frno[pageTableSize - 1]  = frameNumber;

                        fread(physical_memory[frameNumber], sizeof(signed char), FRAME_SIZE, BackingStore);
                        insertIntoTLB(pageNumber, frameNumber);

                    }

                    //Physical Memory not Full
                    else{
                        frameNumber = mem_index;
                        mem_index++;

                        fread(physical_memory[frameNumber], sizeof(signed char), FRAME_SIZE, BackingStore);
                        insertIntoTLB(pageNumber, frameNumber);
                        insertIntoPageTable(pageNumber, frameNumber);
                    }
                    
                }

            }

            // Page found in TLB
            else{
                hitCounter++;
                update();
            }

            physicalAddress = calculatePhysicalAddress(frameNumber,offset);
            value = physical_memory[frameNumber][offset];
            fprintf(outputFile, "%d,%d,%d\n", logicalAddress, physicalAddress, value);
            
        }
   
    fprintf(outputFile, "Page Faults Rate, %.2f%%,\n", 100*((double) pageFaultCounter / (double) totalLinesRead));
    fprintf(outputFile, "TLB Hits Rate, %.2f%%,", 100*((double) hitCounter / (double) totalLinesRead));
    fclose(BackingStore);
    fclose(addresses);
    fclose(outputFile);
    
    return 0;
}

void invalidateTLB(){  
    int i;
    for(i = 0; i < TLB_ENTRIES; i++){
        TLB_pgno[i]= -1;    // -1 indicates empty
        TLB_frno[i]= -1;
    }
}

void invalidatePageTable(){ 
  int i;
    for(i = 0; i < PAGE_ENTRIES; i++){
        pageTable_pgno[i] = -1; // -1 indicates empty
        pageTable_frno[i] = -1;
    }
}



int calculatePageNumber(int logicalAddress){
    return (logicalAddress >> PAGE_BITS);
}

int calculateOffset(int logicalAddress){
    return logicalAddress & 255;
}


int scanTLB(int pg_num){
    int i = 0;
        while(i < 16){  

            if((TLB_pgno[i]) == pg_num){
                return (TLB_frno[i]);
            }
            i = i + 1;
        } 
    return -1;
    
}

int scanPageTable(int pg_num){
    int i = 0;
        while(i < pageTableSize){

            if((pageTable_pgno[i]) == pg_num){
                return (pageTable_frno[i]);
            }
            i = i + 1;
            
        }  
    
    return -1;
    
}

void insertIntoTLB(int pg_num, int frm_num){    //inserting at the end 

    (TLB_frno[TLBSize]) = frm_num;
    (TLB_pgno[TLBSize]) = pg_num;
     TLBSize += 1;
     TLBSize %=  TLB_ENTRIES;


}

void insertIntoPageTable(int pg_num, int frm_num){      //inserting at the end 

    (pageTable_frno[pageTableSize]) = frm_num;
    (pageTable_pgno[pageTableSize]) = pg_num;
    pageTableSize++;
    pageTableSize %= PAGE_ENTRIES;

}

int calculatePhysicalAddress(int frameNumber, int offset){
    return ((frameNumber << 8) + offset);
}

void update(){  //updates LRU
    int i;
    int LRUpage;   
    int LRUframe;

        for(i = 0; i < pageTableSize; i++){
            if(pageTable_frno[i] == frameNumber){

                LRUpage = pageTable_pgno[i];   
                LRUframe = pageTable_frno[i];

                int j = i;
                while(j < (pageTableSize - 1)){
                    pageTable_pgno[j] = pageTable_pgno[j + 1];
                    pageTable_frno[j] = pageTable_frno[j + 1];
                    j++;
                }
                pageTable_pgno[pageTableSize - 1] = LRUpage;
                pageTable_frno[pageTableSize - 1] = LRUframe;

            }

        }
}