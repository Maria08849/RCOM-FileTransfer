#include "application_layer.h"


unsigned char* createControlPacket(int C, long FileSize, const char* filename, int* packetSize){
    const int LenL1 = sizeof(FileSize);
    const int LenL2 = strlen(filename);
    int finalSize =  LenL1 + LenL2 + 5;
    *packetSize = finalSize;

    unsigned char* ret = (unsigned char*)malloc(finalSize);

    ret[0] = C;
    ret[1] = 0;
    ret[2] = LenL1;

    for (unsigned char i = 0 ; i < LenL1 ; i++) {
        ret[2+LenL1-i] = FileSize | 0x00;
        FileSize >>= 8;
    }

    ret[LenL1+3] = 1;
    ret[LenL1+4] = LenL2;

    for (int i = 0; i < LenL2; i++){
        ret[LenL1 + 5 + i] = filename[i];
    }

    return ret;
}

unsigned char* decodeControlPacket(unsigned char* packet, int size, unsigned long int* fileSize){
    unsigned char fileSizeNBytes = packet[2];
    unsigned char fileSizeAux[fileSizeNBytes];
    memcpy(fileSizeAux, packet+3, fileSizeNBytes);

    for(unsigned int i = 0; i < fileSizeNBytes; i++)
        *fileSize |= (fileSizeAux[fileSizeNBytes-i-1] << (8*i));

    unsigned char fileNameNBytes = packet[3+fileSizeNBytes+1];
    unsigned char *name = (unsigned char*)malloc(fileNameNBytes);
    memcpy(name, packet+3+fileSizeNBytes+2, fileNameNBytes);
    return name;

}

unsigned char* createDataPacket(const unsigned char* packageContent, int packageContentSize, int* packetSize){
    int LenL2 = packageContentSize / 256;
    int LenL1 = packageContentSize % 256;
    *packetSize = 3 + packageContentSize;
    unsigned char* ret = (unsigned char*)malloc(*packetSize);

    ret[0] = 1;
    ret[1] = LenL2;
    ret[2] = LenL1;
    for (int i = 0; i < packageContentSize; i++){
        ret[i + 3] = packageContent[i];
    }

    return ret;
}

void applicationLayerTransmiter(struct linkLayer* ll, const char *filename){
    if (llopen(ll, TRANSMITER) == -1) return;
    FILE* file = fopen(filename, "rb");

    int begin = ftell(file);
    fseek(file, 0, SEEK_END);
    long FileSize = ftell(file);
    fseek(file, begin, SEEK_SET);

    int controlPacketLen;

    unsigned char* controlPacket = createControlPacket(2, FileSize, filename, &controlPacketLen);

    llwrite(ll, controlPacket, controlPacketLen);

    char* data = (char *)malloc(FileSize);

    fread(data, 1, FileSize, file);

    long int RemainingBytes = FileSize;

    while (RemainingBytes > 0){
        int packageContentSize;
        if (RemainingBytes > 100) packageContentSize = 100;
        else packageContentSize = RemainingBytes;

        unsigned char* packageContent = (unsigned char*) malloc(packageContentSize);

        memcpy(packageContent, data, packageContentSize);

        int packageSize;

        unsigned char* packageToSend = createDataPacket(packageContent, packageContentSize, &packageSize);

        int bytesSent = llwrite(ll, packageToSend, packageSize);

        if (bytesSent < 0) return;

        RemainingBytes -= packageContentSize;
        data += packageContentSize;
        free(packageContent);
    }

    controlPacket = createControlPacket(3, FileSize, filename, &controlPacketLen);

    llwrite(ll, controlPacket, controlPacketLen);

    llclose(ll, TRANSMITER);

    fclose(file);
}



void applicationLayerReceiver(struct linkLayer* ll){
    llopen(ll, RECEIVER);

    unsigned char *packet = (unsigned char*)malloc(2000);
    
    int size = llread(ll, packet);


    //char* FileName = "teste.gif";

    unsigned int long ControlFileSize;
    unsigned char* FileName = decodeControlPacket(packet, size, &ControlFileSize);

    FILE* File = fopen(FileName, "wb+");

    int finish = FALSE;

    unsigned long int practicalSize = 0;

    while (!finish){
        int packetSize = llread(ll, packet);
        if (packetSize == -1) return;
        if(packet[0] == 3) finish = TRUE;
        else{
            unsigned long int contentSize = packet[1] * 256 + packet[2];
            practicalSize += contentSize;

            printf("=======================Dados que chegaram ao application=========\n");
            for (int i = 0; i < packetSize; i++){
                printf("%d,", packet[i]);
            }
            printf("\n=======================Dados que chegaram ao application=========\n");
            
            fwrite(&packet[3], sizeof(unsigned char), packetSize - 3, File);

            }
    }

    printf("The practical Size is: %ld\n", practicalSize);


    fclose(File);

    llclose(ll, RECEIVER);
}


void applicationLayer(const char *serialPort, int mode, int baudRate,
                      int nTries, int timeout, const char *filename){
    
    struct linkLayer* ll;
    ll = (struct linkLayer*)malloc(sizeof(struct linkLayer));
    strcpy(ll->port, serialPort);
    ll->baudRate = baudRate;
    ll->timeout = timeout;
    ll->numTransmissions = nTries;



    if (mode == RECEIVER){
        printf("receiver\n");
        applicationLayerReceiver(ll);
    }

    if (mode == TRANSMITER){
        printf("transmiter\n");
        applicationLayerTransmiter(ll, filename);
    }

}

