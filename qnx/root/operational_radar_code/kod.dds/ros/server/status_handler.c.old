#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#include "global_server_variables.h"

extern int verbose;
extern int gpssock;
extern int diosock;
extern struct GPSStatus gpsstatus;
extern int txstatus;

void status_exit(void *arg)
{
   int *sockfd = (int *) arg;
   pthread_t tid;
/* get the calling thread's ID */
   tid = pthread_self();
}

void *status_handler(void *arg)
{
   int gpssecond,gpsnsecond;
   struct DriverMsg msg;
/* set the cancellation parameters --
   - Enable thread cancellation 
   - Defer the action of the cancellation 
*/
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
   pthread_cleanup_push(&status_exit,NULL);

   while (1) {
//     if (verbose > 1) printf("Inside the Status Handler while loop.\n");
//     msg.type=GPS_SET_TRIGGER_RATE;
/*     printf("Get GPS Time\n");
     msg.type=GPS_GET_SOFT_TIME;
     msg.status=1;
     send_data(gpssock, &msg, sizeof(struct DriverMsg));
     recv_data(gpssock,&gpssecond, sizeof(int));
     recv_data(gpssock,&gpsnsecond, sizeof(int));
     recv_data(gpssock, &msg, sizeof(struct DriverMsg));
     //printf("GPS Second %d NSecond %d %d\n",gpssecond,gpsnsecond,msg.status);
     //printf("Get GPS Status\n");
     msg.type=GPS_GET_HDW_STATUS;
     msg.status=1;
     send_data(gpssock, &msg, sizeof(struct DriverMsg));
     recv_data(gpssock, &gpsstatus, sizeof(struct GPSStatus));
     recv_data(gpssock, &msg, sizeof(struct DriverMsg));
     //printf("Get Transmitter Status\n");
//     msg.type=GPS_SET_TRIGGER_RATE;
     msg.type=DIO_GET_TX_STATUS;
     msg.status=1;
     send_data(diosock, &msg, sizeof(struct DriverMsg));
     recv_data(diosock, &txstatus, sizeof(struct tx_status));
     recv_data(diosock, &msg, sizeof(struct DriverMsg));
*/
     pthread_testcancel();
     sleep(1);
     
   }
   pthread_cleanup_pop(0);
   pthread_exit(NULL);
};


