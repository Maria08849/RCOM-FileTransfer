#include "link_layer.h"

unsigned char* createControlPacket(int C, long FileSize, const char* filename, int* packetSize);

unsigned char* decodeControlPacket(unsigned char* packet, int size, unsigned long int* fileSize);

unsigned char* createDataPacket(const unsigned char* packageContent, int packageContentSize, int* packetSize);

void applicationLayerTransmiter(struct linkLayer* ll, const char *filename);

void applicationLayerReceiver(struct linkLayer* ll);

void applicationLayer(const char *serialPort, int mode, int baudRate,
                      int nTries, int timeout, const char *filename);

                      