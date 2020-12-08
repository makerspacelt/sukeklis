#define LIBSUK_EXPORT_FILE "/sys/class/gpio/export"
#define LIBSUK_DIRECTION_FILE "/sys/class/gpio/gpio%d/direction"
#define LIBSUK_VALUE_FILE "/sys/class/gpio/gpio%d/value"

#define LIBSUK_INIT_DIR_IN  1
#define LIBSUK_INIT_DIR_OUT 0

#define LIBSUK_STEPS_PER_ROTATION 1200

#define LIBSUK_CHANGE_DIR_SLEEP 50*1000*1000L

#define LIBSUK_SLEEP_MIN  2*1000*1000L
#define LIBSUK_SLEEP_MAX 10*1000*1000L
#define LIBSUK_STEP_ACC       1.01

#include "sleep.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>

struct libsuk_pins { int   en, dir, step, ms1, ms2, ms3, t1, t2 ;};
struct libsuk_pins pin_key;
struct libsuk_pins pin_val;

struct libsuk_status {
	double position, target, acc, deg_per_step, tolerance;
	long min_sleep, max_sleep, sleep;
	int pause, spin, speed, res, steps_per_rot, dir, stop, spin_dir;
};
struct libsuk_status status;

typedef struct{
	int   val, ms1, ms2, ms3 ;
} res_t;


int libsuk_init();
int libsuk_pin_init(int pin, int dir);

int libsuk_pin_set(int pin, int val);
int libsuk_pin_get(int pin);


int libsuk_en_set(int en);
int libsuk_dir_set(int dir);
int libsuk_res_set(int res);

int libsuk_step();

int libsuk_go(volatile sig_atomic_t*);

int libsuk_rpi_rev();



double libsuk_set_target(double);
double libsuk_double_to_deg(double);
int libsuk_status(char *status, int len);
int libsuk_status_request();
int libsuk_comp(double a, double b, double e);
int libsuk_comp_pos(double a, double b);
int libsuk_calc_next_sleep(int speed_up);
res_t libsuk_int_to_res(int);
