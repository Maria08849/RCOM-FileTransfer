#include "link_layer.h"
#define BAUDRATE B38400


volatile int STOP = FALSE;
int alarmCount = 0;
int alarmTriggered = FALSE;

void alarmHandler(int signal) {
    alarmTriggered = FALSE;
    printf("Alarm called for %d\n", alarmCount);
    alarmCount++;
}

int llOpenTransmiter(struct linkLayer* ll){

    (void) signal(SIGALRM, alarmHandler);
    printf("llOpen Transmiter called\n");
    int fd = open(ll->port, O_RDWR | O_NOCTTY);

    if (fd < 0){
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    if(tcgetattr(fd, &oldtio) == -1){
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }
    unsigned char buf[SET_SIZE] = {FLAG, A_SENDER, C_SET, A_SENDER ^ C_SET, FLAG};
    int finish = FALSE;

    state_machine* st = (state_machine*)malloc(sizeof(state_machine));

    while(finish == FALSE && ll->numTransmissions > alarmCount){
        if (alarmTriggered == FALSE){
            alarm(ll->timeout);
            alarmTriggered = TRUE;
        }
        st->current_state = START;
        int bytes = write(fd, buf, SET_SIZE);
        int bytes1 = read(fd, buf, SET_SIZE);
        transition(st, buf, bytes1, A_RECEIVER, C_UA);
        if (st->current_state == STATE_STOP){
            finish = TRUE;
            printf("%d\n", bytes);
            printf("Transmiter Opened with success\n");
            return 0;
        }
    }
    ll->numTransmissions--;
    return -1;
}

int llOpenReceiver(struct linkLayer* ll){
    int fd = open(ll->port, O_RDWR | O_NOCTTY);

    if (fd < 0){
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    if(tcgetattr(fd, &oldtio) == -1){
        perror("tcgetattr");
        exit(-1);
    }

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    unsigned char buf[SET_SIZE];

    state_machine* st = (state_machine*)malloc(sizeof(state_machine));

    while (STOP == FALSE){
        st->current_state = START;
        int bytes = read(fd, buf, SET_SIZE);
        buf[bytes] = '\0';
        transition(st, buf, bytes, A_SENDER, C_SET);
        if (st->current_state == STATE_STOP){
            unsigned char buf[SET_SIZE] = {FLAG, A_RECEIVER, C_UA, A_RECEIVER ^ C_UA, FLAG};
            write(fd, buf, SET_SIZE);
            STOP = TRUE;
        }
    }

    free(st);
    printf("Receiver Opened with success\n");
    return 0;
}

int llopen(struct linkLayer* ll, int mode){
    if (mode == TRANSMITER){
        return llOpenTransmiter(ll);
    }
    return llOpenReceiver(ll);
}

unsigned char BCC2(unsigned char* frame, int len){

    unsigned char res = frame[0];
    for (int i = 1; i < len; i++){
        res = res ^ frame[i];
    }
    return res;
}

int byteStuffing(unsigned char* frame, int length) {

  unsigned char aux[length + 6];

  for(int i = 0; i < length + 6 ; i++){
    aux[i] = frame[i];
  }
  
  int finalLength = 4;
  for(int i = 4; i < (length + 6); i++){

    if(aux[i] == FLAG && i != (length + 5)) {
      frame[finalLength] = 0x7D;
      frame[finalLength+1] = 0x5E;
      finalLength = finalLength + 2;
    }
    else if(aux[i] == 0x7D && i != (length + 5)) {
      frame[finalLength] = 0x7D;
      frame[finalLength+1] = 0x5D;
      finalLength = finalLength + 2;
    }
    else{
      frame[finalLength] = aux[i];
      finalLength++;
    }
  }

  return finalLength;
}



int llwrite(struct linkLayer* li, unsigned char* frame, int length) {

    (void) signal(SIGALRM, alarmHandler);

    int fd = open(li->port, O_RDWR | O_NOCTTY);

    unsigned char buf[length + 6];
    buf[0] = FLAG;
    buf[1] = A_SENDER;

    if (li->sequenceNumber == 0) buf[2] = 0;
    else buf[2] = 0x40;

    buf[3] = A_SENDER ^ buf[2];

    for (int i = 4; i < length + 4; i++)
        buf[i] = frame[i - 4];

    buf[length + 4] = BCC2(frame, length);
    buf[length + 5] = FLAG;

    printf("-----------------Bytes Before the Stuffing--------------------\n");
    for (int i = 0; i < length + 6; i++) printf("%d,", buf[i]);
    printf("\n");

    length = byteStuffing(buf, length);

    int bytes = write(fd, buf, length);

    alarm(3);
    alarmTriggered = TRUE;

    printf("-----------------Bytes After the Stuffing--------------------\n");
    for (int i = 0; i < length; i++) printf("%d,", buf[i]);
    printf("\n");

    int finish = FALSE;
    unsigned char answer[5];
    state_machine* st = (state_machine*)malloc(sizeof(state_machine));

    while (!finish && alarmCount < li->numTransmissions) {
        bytes = read(fd, answer, 5);

        if (bytes == 0 && alarmTriggered == FALSE) {
            write(fd, buf, length);
            alarm(li->timeout);
            alarmTriggered = TRUE;
            continue;
        }

        if(bytes == 0 && alarmTriggered == TRUE) continue;

        alarmCount = 0;
        alarm(0);
        alarmTriggered = FALSE;

        for (int i = 0; i < bytes; i++) {
            printf("%d, ", answer[i]);
        }
        printf("\n");

        if (li->sequenceNumber == 0) {
            st->current_state = START;
            transition(st, answer, 5, A_RECEIVER, REJ0);
            if (st->current_state == STATE_STOP) {
                write(fd, buf, length);
                alarm(li->timeout);
                alarmTriggered = TRUE;
                printf("The Message was not received correctly\n");
            } else {
                st->current_state = START;
                transition(st, answer, 5, A_RECEIVER, RR1);
                if (st->current_state == STATE_STOP) {
                    finish = TRUE;
                    printf("The Message was received correctly\n");
                    li->sequenceNumber = !li->sequenceNumber;
                    return bytes;
                } else {
                    printf("Something really weird!\n");
                    write(fd, buf, length);
                    alarm(li->timeout);
                    alarmTriggered = TRUE;
                }
            }
        } else if (li->sequenceNumber == 1) {
            st->current_state = START;
            transition(st, answer, 5, A_RECEIVER, REJ1);

            if (st->current_state == STATE_STOP) {
                write(fd, buf, length);
                alarm(li->timeout);
                alarmTriggered = TRUE;
                printf("The Message was not received correctly\n");
            } else {
                st->current_state = START;
                transition(st, answer, 5, A_RECEIVER, RR0);
                if (st->current_state == STATE_STOP) {
                    printf("The Message was received correctly\n");
                    finish = TRUE;
                    li->sequenceNumber = !li->sequenceNumber;
                    return bytes;
                } else {
                    printf("Something really weird!\n");
                    write(fd, buf, length);
                    alarm(li->timeout);
                    alarmTriggered = TRUE;
                }
            }
        } else {
            printf("The sequence number can only be 0 or 1, something is wrong here\n");
        }
    }

    free(st);

    if (alarmCount >= li->numTransmissions) {
        printf("Transmission failed after the maximum number of attempts.\n");
        return -1;
    }

    else return 0;

}



int byteDestuffing(unsigned char* frame, int length) {

  unsigned char aux[length + 5];

  for(int i = 0; i < (length + 5) ; i++) {
    aux[i] = frame[i];
  }

  int finalLength = 0;

  for(int i = 0; i < length; i++) {

    if(aux[i] == 0x7D){
      if (aux[i+1] == 0x5D) {
        frame[finalLength] = 0x7D;
      }
      else if(aux[i+1] == 0x5E) {
        frame[finalLength] = FLAG;
      }
      i++;
      finalLength++;
    }
    else{
      frame[finalLength] = aux[i];
      finalLength++;
    }
  }

  return finalLength;
}



int llread(struct linkLayer* li, unsigned char* res){
    int fd = open(li->port, O_RDWR | O_NOCTTY);
    unsigned char buf[1] = {0};
    int finish = FALSE;
    while (!finish){
        unsigned char *information = (unsigned char *)malloc(300 * sizeof(unsigned char));
        int len = 0;
        unsigned char answer[5];
        unsigned char expected, notexpected;
        int STATE = START;
        while (STATE != STATE_STOP){
            int bytes = read(fd, buf, 1);

            if (bytes > -1){
                printf("Read %d with success: %d\n", buf[0], 1);
            }

            switch(STATE){
                case START:
                    if (buf[0] == FLAG) STATE = FLAG_RCV;
                    continue;
                case FLAG_RCV:
                    printf("Flag state!\n");
                    if (buf[0] == FLAG) continue;
                    else if (buf[0] == A_SENDER) {
                        STATE = A_RCV;
                    }
                    else STATE = START;
                    continue;
                case A_RCV:
                    printf("A state\n");
                    if (li->sequenceNumber == 0){
                        expected = 0x0;
                        notexpected = 0x40;
                    }
                    else{
                        expected = 0x40;
                        notexpected = 0x0;
                    }
                    if (buf[0] == FLAG) STATE = FLAG_RCV;
                    else if (buf[0] == expected) STATE = C_RCV;
                    else if (buf[0] == notexpected){
                        answer[0] = FLAG;
                        answer[1] = A_RECEIVER;
                        if (li->sequenceNumber == 1) answer[2] = RR0;
                        else answer[2] = RR1;
                        answer[3] = answer[1] ^ answer[2];
                        answer[4] = FLAG;
                        write(fd, answer, 5);
                        finish = TRUE;
                        break;
                    }
                    else STATE = START;
                    continue;
                case C_RCV:
                    printf("C state\n");
                    if (buf[0] == FLAG) STATE = FLAG_RCV;
                    else{
                        unsigned char expected;
                        if (li->sequenceNumber == 0){
                            expected = 0x0;
                        }
                        else{
                            expected = 0x40;
                        }
                        if (buf[0] == (A_SENDER ^ expected)) STATE = READ_DATA;
                        else STATE = START;
                    }
                    continue;
                case READ_DATA:
                    printf("READ_DATA state\n");
                    if (buf[0] == FLAG) STATE = STATE_STOP;
                    else{
                        information[len] = buf[0];
                        len++;
                    }
                    continue;
            }
        }
        printf("sai do ciclo!\n");

        int tempLen = byteDestuffing(information, len);

        printf("Lenght before the destuffing: %d\n", len);
        printf("Lenght after the destuffing: %d\n", tempLen);

        len = tempLen;

        for (int i = 0; i < len;i++){
            printf("%d, ", information[i]);
        }

        unsigned char theorical_bcc2 = information[len-1];

        printf("Theorical BCC2: %d\n", theorical_bcc2);

        unsigned char practical_bcc2 = BCC2(information, len-1);
        printf("Practial BCC2: %d\n", practical_bcc2);
        printf("Len: %d\n", len);

        if (theorical_bcc2 == practical_bcc2 ){
            printf("Cheguei aqui!\n");
            answer[0] = FLAG;
            answer[1] = A_RECEIVER;
            if (li->sequenceNumber == 0){
                answer[2] = RR1;
            }
            else if (li->sequenceNumber == 1){
                answer[2] = RR0;
            }
            answer[3] = answer[1] ^answer[2];
            answer[4] = FLAG;
            write(fd, answer, 5);
            li->sequenceNumber = !li->sequenceNumber;
            memcpy(res, information, len-1);
            finish = TRUE;
            return len-1;

        }
        else{
            answer[0] = FLAG;
            answer[1] = A_RECEIVER;
            if (li->sequenceNumber == 0) answer[2] = REJ0;
            else answer[2] = REJ1;
            answer[3] = answer[1] ^ answer[2];
            answer[4] = FLAG;
            printf("wrong!\n");
            write(fd, answer, 5);
            STATE = START;
        }

        free(information);

    }

    return -1;
}

int llCloseTransmiter(struct linkLayer* li){
    int fd = open(li->port, O_RDWR | O_NOCTTY);
    unsigned char buf[SET_SIZE] = {FLAG, A_SENDER, C_DISC, A_SENDER ^ C_DISC, FLAG};
    int finish = FALSE;

    state_machine* st = (state_machine*)malloc(sizeof(state_machine));

    while(!finish){
        st->current_state = START;
        write(fd, buf, SET_SIZE);
        int bytes1 = read(fd, buf, SET_SIZE);
        transition(st, buf, bytes1, A_RECEIVER, C_DISC);
        if (st->current_state == STATE_STOP){
            finish = TRUE;
            buf[1] = A_SENDER;
            buf[2] = C_UA;
            buf[3] = A_SENDER ^ C_UA;
            int bytes2 = write(fd, buf, SET_SIZE);
            printf("Sent bytes: %d\n", bytes2);
            return 0;
        }
    }

    return -1;
}

int llCloseReceiver(struct linkLayer* li){
    int fd = open(li->port, O_RDWR | O_NOCTTY);
    unsigned char buf[SET_SIZE];
    state_machine* st = (state_machine*)malloc(sizeof(state_machine));

    STOP = FALSE;

    while (STOP == FALSE)
    {
        st->current_state = START;
        int bytes = read(fd, buf, SET_SIZE);
        transition(st, buf, bytes, A_SENDER, C_DISC);


        if (st->current_state == STATE_STOP){
            printf("primeira trama recebida com sucesso\n");
            unsigned char buf[SET_SIZE] = {FLAG, A_RECEIVER, C_DISC, A_RECEIVER ^ C_DISC, FLAG};
            int bytes = write(fd, buf, SET_SIZE);
            STOP = TRUE;
            int STOP2 = FALSE;
            while(!STOP2){
                bytes = read(fd, buf, SET_SIZE);
                st->current_state = START;
                for (int i = 0; i < bytes; i++) printf("%d,", buf[i]);
                printf("\n");
                transition(st, buf, bytes, A_SENDER, C_UA);
                if (st->current_state == STATE_STOP) {
                    STOP2 = TRUE;
                    return 0;
                }
            }
        }
    }

    free(st);
    return -1;
}

int llclose(struct linkLayer* li, int mode){
    if (mode == RECEIVER){
        return llCloseReceiver(li);
    }
    return llCloseTransmiter(li);
}




