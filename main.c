#include <stdio.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <unistd.h>
#include <math.h>
#include <fftw3.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>

#define BUFFERSIZE 4096

fftw_complex *out_bass_l, *out_bass_r;
fftw_complex *out_mid_l, *out_mid_r;
fftw_complex *out_treble_l, *out_treble_r;
double *in_bass_l, *in_bass_r;
double *in_mid_l, *in_mid_r;
double *in_treble_l, *in_treble_r;

int ind_bass;
int ind_mid;
int ind_treble;

const int bass_size = 4096;
const int mid_size = 1024;
const int treble_size = 512;

fftw_plan p_bass_l, p_bass_r;
fftw_plan p_mid_l, p_mid_r;
fftw_plan p_treble_l, p_treble_r;

void reset_output() {
    memset(in_bass_l, 0, sizeof(double) * bass_size);
    memset(in_bass_r, 0, sizeof(double) * bass_size);
    memset(in_mid_l, 0, sizeof(double) * mid_size);
    memset(in_mid_r, 0, sizeof(double) * mid_size);
    memset(in_treble_l, 0, sizeof(double) * treble_size);
    memset(in_treble_r, 0, sizeof(double) * treble_size);
}

int low_cut = 100000;

const double coef_bass = 5000000;
const double coef_mid = 2000000;
const double coef_treble = 1000000;

int serial_port;

struct termios tty;

void open_serial() {
    serial_port = open("/dev/ttyACM0", O_RDWR);

    if (serial_port < 0){
        printf("error %i from open: %s\n", errno, strerror(errno));
        serial_port = open("/dev/ttyACM1", O_RDWR);
    }

    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("error %i from tcgetattr %s\n", errno, strerror(errno));
        return;
    }

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %i from tcsetattr %s\n", errno, strerror(errno));
    }


    char aaa[4] = {100, 1, 100, 50};
    for (int i = 0; i < 1; i++){
        uint8_t msg[] = {'a'};
        printf("%d\n", msg[0]);
        write(serial_port, msg, sizeof(msg));
    }

    char arr[1] = {(char)10};
    write(serial_port, arr, sizeof(arr));

}

int stop_thread;
int16_t buf[BUFFERSIZE/2];
int error;
pa_simple *s;

pthread_t thread_pulse;

void *read_from_pulse() {
    while(!stop_thread) {
        if (pa_simple_read(s, buf, sizeof(buf), &error) < 0) {
            printf("ERROR! pa_simple_read() failed: %s\n", pa_strerror(error));
            return 0;
        }

    }
}


void write_to_arduino(int k) {
    char bte = (char)k;
    char arr[1] = {bte};
    write(serial_port, arr, sizeof(arr));
}

int main() {

    open_serial();

/*
    while(1) {
        char bte;
        int kek;
        scanf("%d", &kek);
        bte = (char)kek;
        char arr[1] = {bte};
        write(serial_port, arr, sizeof(arr));
        scanf("%d", &kek);
        bte = (char)kek;
        arr[0] = bte;
        write(serial_port, arr, sizeof(arr));
        scanf("%d", &kek);
        bte = (char)kek;
        arr[0] = bte;
        write(serial_port, arr, sizeof(arr));
    }

*/


    const double pi = acos(-1);
    int n = 1000;
    const double step = 4 * pi / n;
    printf("Hello, World!\n");

    double a[n+1];
    double b[n+1];

    FILE *pipe = popen("gnuplot -persist", "w");
    FILE *pipe2 = popen("gnuplot -persist", "w");
    FILE *pipe3 = popen("gnuplot -persist", "w");




    static const pa_sample_spec ss = {.format = PA_SAMPLE_S16LE, .rate = 44100, .channels = 2};

    static const pa_buffer_attr pb = {.maxlength = (uint32_t)-1, .fragsize = BUFFERSIZE};

    s = NULL;
    int16_t frames = BUFFERSIZE/4;

    printf("KEK\n");

    if (!(s = pa_simple_new(NULL, "testAudio", PA_STREAM_RECORD, "alsa_output.pci-0000_00_1f.3.analog-stereo.monitor", "testing some audio",
            &ss, NULL, &pb, &error))) {
        printf("ERROR! Could not open pulseAudio source: %s\n", pa_strerror(error));
        return 0;
    }
    printf("KEK\n");
    int kek = 0;


    in_bass_l = fftw_alloc_real(bass_size);
    memset(in_bass_l, 0, bass_size * sizeof(double));
    out_bass_l = fftw_alloc_complex(bass_size);
    memset(out_bass_l, 0, bass_size * sizeof(fftw_complex));
    p_bass_l = fftw_plan_dft_r2c_1d(bass_size, in_bass_l, out_bass_l, FFTW_MEASURE);

    in_bass_r = fftw_alloc_real(bass_size);
    memset(in_bass_r, 0, bass_size * sizeof(double));
    out_bass_r = fftw_alloc_complex(bass_size);
    memset(out_bass_r, 0, bass_size * sizeof(fftw_complex));
    p_bass_r = fftw_plan_dft_r2c_1d(bass_size, in_bass_r, out_bass_r, FFTW_MEASURE);

    in_mid_l = fftw_alloc_real(mid_size);
    memset(in_mid_l, 0, mid_size * sizeof(double));
    out_mid_l = fftw_alloc_complex(mid_size);
    memset(out_mid_l, 0, mid_size * sizeof(fftw_complex));
    p_mid_l = fftw_plan_dft_r2c_1d(mid_size, in_mid_l, out_mid_l, FFTW_MEASURE);

    in_mid_r = fftw_alloc_real(mid_size);
    memset(in_mid_r, 0, mid_size * sizeof(double));
    out_mid_r = fftw_alloc_complex(mid_size);
    memset(out_mid_r, 0, mid_size * sizeof(fftw_complex));
    p_mid_r = fftw_plan_dft_r2c_1d(mid_size, in_mid_r, out_mid_r, FFTW_MEASURE);

    in_treble_l = fftw_alloc_real(treble_size);
    memset(in_treble_l, 0, treble_size * sizeof(double));
    out_treble_l = fftw_alloc_complex(treble_size);
    memset(out_treble_l, 0, treble_size * sizeof(fftw_complex));
    p_treble_l = fftw_plan_dft_r2c_1d(treble_size, in_treble_l, out_treble_l, FFTW_MEASURE);

    in_treble_r = fftw_alloc_real(treble_size);
    memset(in_treble_r, 0, treble_size * sizeof(double));
    out_treble_r = fftw_alloc_complex(treble_size);
    memset(out_treble_r, 0, treble_size * sizeof(fftw_complex));
    p_treble_r = fftw_plan_dft_r2c_1d(treble_size, in_treble_r, out_treble_r, FFTW_MEASURE);


    pthread_create(&thread_pulse, NULL, read_from_pulse, NULL);

    while(1){

        /*fprintf(pipe, "set yrange [-30000:30000]\n");
        fprintf(pipe, "plot '-' with lines\n");
        for (int i = 0; i < BUFFERSIZE/2; i+=2) {
            double x = i;
            double y = buf[i];
            fprintf(pipe, "%f\t%f\n", x, y);
        }
        fprintf(pipe, "%s\n", "e");
        fflush(pipe);
         */
        //usleep(100000);
        ++kek;

        memset(in_bass_l, 0, bass_size * sizeof(double));
        memset(out_bass_l, 0, bass_size * sizeof(fftw_complex));
        memset(in_bass_r, 0, bass_size * sizeof(double));
        memset(out_bass_r, 0, bass_size * sizeof(fftw_complex));
        memset(in_mid_l, 0, mid_size * sizeof(double));
        memset(out_mid_l, 0, mid_size * sizeof(fftw_complex));
        memset(in_mid_r, 0, mid_size * sizeof(double));
        memset(out_mid_r, 0, mid_size * sizeof(fftw_complex));

        memset(in_treble_l, 0, treble_size * sizeof(double));
        memset(out_treble_l, 0, treble_size * sizeof(fftw_complex));
        memset(in_treble_r, 0, treble_size * sizeof(double));
        memset(out_treble_r, 0, treble_size * sizeof(fftw_complex));


        ind_bass = 0;
        ind_mid = 0;
        ind_treble = 0;

        for (int i = 0; i < BUFFERSIZE/2; i += 2) {
            in_bass_l[ind_bass] = buf[i];
            in_bass_r[ind_bass] = buf[i+1];
            in_mid_l[ind_mid] = buf[i];
            in_mid_r[ind_mid] = buf[i+1];
            in_treble_l[ind_treble] = buf[i];
            in_treble_r[ind_treble] = buf[i+1];

            ind_bass = (ind_bass + 1) % bass_size;
            ind_mid = (ind_mid + 1) % mid_size;
            ind_treble = (ind_treble + 1) % treble_size;

        }

        fftw_execute(p_bass_l);
        fftw_execute(p_bass_r);
        fftw_execute(p_mid_l);
        fftw_execute(p_mid_r);
        fftw_execute(p_treble_l);
        fftw_execute(p_treble_r);


        double mx1 = 0, mx2 = 0, mx3 = 0;

        fprintf(pipe, "set title \"bass\"\n");
        fprintf(pipe, "set yrange [0:5000000]\n");
        fprintf(pipe, "plot '-' with lines\n");
        int kkek = 0;
        double mx = 0;
        for (int i = 0; i < BUFFERSIZE/128; i+=2) {
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y > mx)
                mx = y;
        }
        for (int i = 0; i < BUFFERSIZE/128; i+=2) {
            double x = kkek++;
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y < low_cut)
                y = 0;
            if (y > mx1)
                mx1 = y;
            fprintf(pipe, "%f\t%f\n", x, y);
        }
        fprintf(pipe, "%s\n", "e");
        fflush(pipe);



        fprintf(pipe2, "set title \"mid\"\n");
        fprintf(pipe2, "set yrange [0:5000000]\n");
        fprintf(pipe2, "plot '-' with lines\n");
        int kkkek = 0;
        mx = 0;
        for (int i = BUFFERSIZE/128; i < BUFFERSIZE/32; i+=2) {
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y > mx)
                mx = y;
        }
        for (int i = BUFFERSIZE/128; i < BUFFERSIZE/32; i+=2) {
            double x = ++kkkek;
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y < low_cut)
                y = 0;
            if (y > mx2)
                mx2 = y;
            fprintf(pipe2, "%f\t%f\n", x, y);
        }
        fprintf(pipe2, "%s\n", "e");
        fflush(pipe2);



        fprintf(pipe3, "set title \"treble\"\n");
        fprintf(pipe3, "set yrange [0:5000000]\n");
        fprintf(pipe3, "plot '-' with lines\n");
        int kkkkek = 0;
        for (int i = BUFFERSIZE/32; i < BUFFERSIZE/16; i+=2) {
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y > mx)
                mx = y;
        }
        for (int i = BUFFERSIZE/32; i < BUFFERSIZE/16; i+=2) {
            double x = kkkkek++;
            double y1 = hypot(out_bass_l[i][0], out_bass_l[i][1]);
            double y2 = hypot(out_bass_r[i][0], out_bass_r[i][1]);
            double y;
            if (y1 > y2)
                y = y1;
            else
                y = y2;
            if (y < low_cut)
                y = 0;
            if (y > mx3)
                mx3 = y;
            fprintf(pipe3, "%f\t%f\n", x, y);
        }
        fprintf(pipe3, "%s\n", "e");
        fflush(pipe3);

        printf("%f %f %f\n", mx1/coef_bass, mx2/coef_mid, mx3/coef_treble);

        double c_bass = mx1/coef_bass;
        double c_mid = mx2/coef_mid;
        double c_treble = mx3/coef_treble;
        if (c_bass > 1.0)
            c_bass = 1.0;
        if (c_mid > 1.0)
            c_mid = 1.0;
        if (c_treble > 1.0)
            c_treble = 1.0;
        c_bass *= 255.0;
        c_mid *= 255.0;
        c_treble *= 255.0;

        int k_bass = (int)round(c_bass);
        int k_mid = (int)round(c_mid);
        int k_treble = (int)round(c_treble);
        write_to_arduino(k_bass);
        write_to_arduino(k_mid);
        write_to_arduino(k_treble);

        usleep(50000);

    }

    fftw_free(out_bass_l);
    fftw_free(out_bass_r);
    fftw_free(in_bass_l);
    fftw_free(in_bass_r);
    fftw_destroy_plan(p_bass_l);
    fftw_destroy_plan(p_bass_r);

    pa_simple_free(s);

    pclose(pipe);
    pclose(pipe2);
    pclose(pipe3);

    close(serial_port);
    return 0;
}
