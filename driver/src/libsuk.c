#define _GNU_SOURCE

#include "libsuk.h"

res_t resolutions[5] = {
	{ .val = 1,  .ms1 = 0, .ms2 = 0, .ms3 = 0 },
	{ .val = 2,  .ms1 = 1, .ms2 = 0, .ms3 = 0 },
	{ .val = 4,  .ms1 = 0, .ms2 = 1, .ms3 = 0 },
	{ .val = 8,  .ms1 = 1, .ms2 = 1, .ms3 = 0 },
	{ .val = 16, .ms1 = 1, .ms2 = 1, .ms3 = 1 }
};


int libsuk_init() {

	pin_key.en   = 23;
	pin_key.dir  = 4;
	pin_key.step = 18;
	pin_key.ms1  = 25;
	pin_key.ms2  = 24;
	pin_key.ms3  = 22;
	pin_key.t1   = 17;
	pin_key.t2   = 21;

	status.position      = 0.0;
	status.target        = 0.0;
	status.min_sleep     = LIBSUK_SLEEP_MIN;
	status.max_sleep     = LIBSUK_SLEEP_MAX;
	status.acc           = LIBSUK_STEP_ACC;
	status.spin          = 0;
	status.spin_dir      = 0;
	status.pause         = 0;
	status.stop          = 0;
	status.sleep         = 0;
	status.speed         = 0;
	status.dir           = 0;
	status.res           = 0;
	status.steps_per_rot = 0;
	status.deg_per_step  = 0.0;
	status.tolerance     = 0.0;

	int rev = libsuk_rpi_rev();
	switch (rev) {
		case '1':
		case '2':
			pin_key.t2 = 21;
			break;
		case '3':
		case '4':
		case '5':
			pin_key.t2 = 28;
			break;
	}

	libsuk_pin_init(pin_key.en, LIBSUK_INIT_DIR_OUT);
	libsuk_pin_init(pin_key.dir, LIBSUK_INIT_DIR_OUT);
	libsuk_pin_init(pin_key.step, LIBSUK_INIT_DIR_OUT);
	libsuk_pin_init(pin_key.ms1, LIBSUK_INIT_DIR_OUT);
	libsuk_pin_init(pin_key.ms2, LIBSUK_INIT_DIR_OUT);
	libsuk_pin_init(pin_key.ms3, LIBSUK_INIT_DIR_OUT);

	libsuk_en_set(1);
	libsuk_dir_set(1);
	libsuk_res_set(16);
	return 0;
}
int libsuk_pin_init(int pin, int dir) {
	int fd;
	char *fname;
	char *cpin;
	dir &= 1;
	asprintf(&cpin,"%d",pin);

	asprintf(&fname, LIBSUK_EXPORT_FILE );
	fd = open(fname, O_WRONLY|O_TRUNC|O_FSYNC);
	write(fd, cpin, strlen(cpin) );
	close(fd);

	asprintf(&fname, LIBSUK_DIRECTION_FILE, pin );
	fd = open(fname, O_WRONLY|O_TRUNC|O_FSYNC);
	write(fd, (dir==1)?"in":"out", (dir==1)?2:3);
	close(fd);

	return 0;
}
int libsuk_en_set(int en) {
	en &= 1;
	pin_val.en = en;
	libsuk_pin_set(pin_key.en, en);
	return 0;
}
int libsuk_dir_set(int dir) {
	dir &= 1;
	pin_val.dir = 1-dir;
	libsuk_pin_set(pin_key.dir, 1-dir);
	status.dir = dir;
	nsleep(LIBSUK_CHANGE_DIR_SLEEP);
	return 0;
}
int libsuk_res_set(int res) {
	res_t r = libsuk_int_to_res(res);
	pin_val.ms1 = r.ms1;
	pin_val.ms2 = r.ms2;
	pin_val.ms3 = r.ms3;
	libsuk_pin_set(pin_key.ms1, pin_val.ms1 );
	libsuk_pin_set(pin_key.ms2, pin_val.ms2 );
	libsuk_pin_set(pin_key.ms3, pin_val.ms3 );
	status.res = res;
	status.steps_per_rot = LIBSUK_STEPS_PER_ROTATION * status.res;
	status.deg_per_step = 360.0 / status.steps_per_rot;
	status.tolerance = status.deg_per_step/1.9;
	return 0;
}
int libsuk_step() {
	libsuk_pin_set(pin_key.step, 1);
	libsuk_pin_set(pin_key.step, 0);
	status.position += (status.dir==1)?status.deg_per_step:-status.deg_per_step;
	status.position = libsuk_double_to_deg(status.position);
	return 0;
}
int libsuk_go(volatile sig_atomic_t *fin) {
	status.sleep = status.max_sleep+1;
	status.speed = 0;

	double calc_pos, break_dist, a, b;
	int calc_dir, speed_up;

	while ( *fin < 1 ) {
		// do we need to stop?
		if ( status.stop == 1 || status.pause == 1 ) {
			if ( status.speed > 0 ) {
				libsuk_calc_next_sleep(0);
				nsleep(status.sleep);
				libsuk_step();
				printf("-(STOP) pos %.5f del %.1f\n", status.position, status.sleep/1000.0/1000.0 );
			} else {
				// thats it, we have stopped.
				// no more steps until stop or pause flag is set
				nsleep(status.sleep);
			}
		} else
		// do we need to spin?
		if ( status.spin == 1 ) {
			speed_up = 1;
			if (status.spin_dir != status.dir) {
				if (status.speed == 0) {
					libsuk_dir_set(status.spin_dir);
				} else {
					speed_up = 0;
				}
			}
			libsuk_calc_next_sleep(speed_up);
			nsleep(status.sleep);
			libsuk_step();
			printf("-(SPIN) pos %.5f del %.1f \n", status.position, (double)(status.sleep/1000.0/1000.0) );
		} else
		// let's go, target awaits!
		if ( libsuk_comp_pos(status.position, status.target) != 0 || status.speed != 0 ) {
			speed_up = 1;
			// we need to slow_down if
			// we are heading wrong direction
			//printf("target(%.5f); speed(%d);\n", status.target, status.speed);
			break_dist = status.speed * status.deg_per_step;
			//printf("break_dist(%.5f);\n", break_dist);
			break_dist *= (status.dir==1)?1:-1;
			//printf("break_dist(%.5f);\n", break_dist);
			calc_pos = status.position + break_dist;
			//printf("pos(%.5f);\n", status.position);
			//printf("calc_pos(%.5f);\n", calc_pos);
			calc_pos = libsuk_double_to_deg(calc_pos);
			//printf("calc_pos(%.5f);\n", calc_pos);
			a = libsuk_double_to_deg( status.target - calc_pos );
			b = libsuk_double_to_deg( calc_pos - status.target );
			calc_dir = (a<b)?1:0;
			//printf("cur_dir(%d); calc_dir(%d);\n", status.dir, calc_dir);
			// do we need to change direction?
			if (calc_dir != status.dir) {
				if (status.speed <= 0) {
					libsuk_dir_set(calc_dir);
					//printf("set cur_dir(%d);\n", status.dir);
				}
				speed_up = 0;
			}
			// do target calculations
			a = libsuk_double_to_deg( status.target - status.position );
			b = libsuk_double_to_deg( status.position - status.target );
			if ( ((a<b)?a:b) <= ((status.speed+2)*status.deg_per_step) )
				speed_up = 0;

			libsuk_calc_next_sleep(speed_up);
			nsleep(status.sleep);
			libsuk_step();
			printf("-(TRGT) pos %.5f del %.1f spd %d\n", status.position, status.sleep/1000.0/1000.0, status.speed );
		} else {
			nsleep(status.sleep);
		}
		if ( status.speed == 0 && status.stop == 1 ) {
			libsuk_set_target(0.0);
			status.position = 0.0;
			status.stop = 0;
			status.spin = 0;
		}
	}
	return 0;
}
// FIXME: this function changes global variables!!!
int libsuk_calc_next_sleep(int speed_up) {
	if ( speed_up == 1 ) {
		if (status.min_sleep < status.sleep) {
			status.sleep = (long)(status.sleep/status.acc);
			status.speed++;
		}
	} else {
		if (status.max_sleep > status.sleep && status.speed > 0) {
			status.sleep *= status.acc;
			status.speed--;
		}
	}
	return 0;
}
int libsuk_pin_set(int pin, int val) {
	int fd;
	char *fname;

	asprintf(&fname, LIBSUK_VALUE_FILE, pin);
	fd = open(fname, O_WRONLY|O_TRUNC|O_FSYNC);
	write(fd, (val==1)?"1":"0", 1);
	close(fd);

	return 0;
}
int libsuk_pin_get(int pin) {
	int fd;
	char *fname;
	char val;

	asprintf(&fname, LIBSUK_VALUE_FILE, pin);
	fd = open(fname, O_RDONLY);
	read(fd, &val, 1);
	close(fd);

	return val-48;
}

int libsuk_rpi_rev() {
	return 2;
}

double libsuk_set_target(double t) {
	status.target = libsuk_double_to_deg(t);
	return status.target;
}

double libsuk_double_to_deg(double d) {
	d = fmod(d, 360.0);
	if (d < 0) d = 360.0 + d;
	return d;
}
int libsuk_comp_pos(double a, double b) {
	if ( a < 1 || a > 359) {
		a = libsuk_double_to_deg( a + 180 );
		b = libsuk_double_to_deg( b + 180 );
	}
	return libsuk_comp(a, b, status.tolerance);
}
int libsuk_comp(double a, double b, double e) {
	if ( a == b ) return 0;
	if ( a > b && a > (b+e) ) return 1;
	if ( a < b && (a+e) < b ) return -1;
	return 0;
}
res_t libsuk_int_to_res(int r) {
	int i;
	for (i=0; i<sizeof(resolutions); i++) {
		if (resolutions[i].val == r) {
			return resolutions[i];
		}
	}
	res_t empty = {0,0,0,0};
	return empty;
}
int libsuk_status(char *st, int len) {
	char *l="";
	memset(st, 0 ,len);
	asprintf(&l, "[status]\n"); strcat(st, l);
	asprintf(&l, "enabled = %d\n",        pin_val.en           ); strcat(st, l);
	asprintf(&l, "position = %.5f\n",     status.position      ); strcat(st, l);
	asprintf(&l, "target = %.5f\n",       status.target        ); strcat(st, l);
	asprintf(&l, "pause = %d\n",          status.pause         ); strcat(st, l);
	asprintf(&l, "stop = %d\n",           status.stop          ); strcat(st, l);
	asprintf(&l, "spin = %d\n",           status.spin          ); strcat(st, l);
	asprintf(&l, "spin_dir = %d\n",       status.spin_dir      ); strcat(st, l);
	asprintf(&l, "dir = %d\n",            status.dir           ); strcat(st, l);
	asprintf(&l, "sleep = %ld\n",         status.sleep         ); strcat(st, l);
	asprintf(&l, "speed = %d\n",          status.speed         ); strcat(st, l);
	asprintf(&l, "[config]\n"); strcat(st, l);
	asprintf(&l, "acc = %.5f\n",          status.acc           ); strcat(st, l);
	asprintf(&l, "min_sleep = %ld\n",     status.min_sleep     ); strcat(st, l);
	asprintf(&l, "max_sleep = %ld\n",     status.max_sleep     ); strcat(st, l);
	asprintf(&l, "res = %d\n",            status.res           ); strcat(st, l);
	asprintf(&l, "ms1 = %d\n",            pin_val.ms1          ); strcat(st, l);
	asprintf(&l, "ms2 = %d\n",            pin_val.ms2          ); strcat(st, l);
	asprintf(&l, "ms3 = %d\n",            pin_val.ms3          ); strcat(st, l);
	asprintf(&l, "steps_per_rot = %d\n",  status.steps_per_rot ); strcat(st, l);
	asprintf(&l, "deg_per_step = %.5f\n", status.deg_per_step  ); strcat(st, l);
	asprintf(&l, "tolerance = %.5f\n",    status.tolerance     ); strcat(st, l);
	*(st+strlen(st)+1) = '\0';
	return strlen(st);
}
