#define MAX_CARDS 7 
#define MAX_INPUTS 3 
#define FIFO_LVL	   512
#define SAMPLE_FREQ        100000 //Khz
#define REVERSE_IQ_ORDER 0
#define USING_GC314 1

typedef struct _copy_chuck{ 
        int length;
        int start;
        int channel;
        int buffer;
        double frequency;
        int radar;
        int beamdir;
} t_copy_chunk;

typedef struct _noise_calculate { 
        int samples;
        int antenna;
        int channel;
        int buffer;
        int ns0;
        int ns1;
        int nnoise;
        double n_thresh;
} t_noise_calculate;

