#include "macros.h"
#include "statemachine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>


struct linkLayer {
    char port[20]; /* Dispositivo /dev/ttySx, x = 0, 1 */
    int baudRate;  /* Velocidade de transmissão */
    unsigned int sequenceNumber; /* Número de sequência da trama: 0, 1 */
    unsigned int timeout; /* Valor do temporizador: 1 s */
    unsigned int numTransmissions; /* Número de tentativas em caso de falha */
    char frame[MAX_SIZE]; /* Trama */
};

int llopen(struct linkLayer* li, int mode);

int llOpenTransmiter(struct linkLayer* li);

int llOpenReceiver(struct linkLayer* li);

int llwrite(struct linkLayer* li, unsigned char* frame, int length);

int llread(struct linkLayer* li, unsigned char* res);

int llclose(struct linkLayer* li, int mode);

int llCloseTransmiter(struct linkLayer* li);

int llCloseReceiver(struct linkLayer* li);
