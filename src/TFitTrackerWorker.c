#include <pebble_worker.h>
	
#define MAX(a,b) ((a) > (b) ? a : b)
#define PERSIST_KEY_MEDTHRESSOLD 10
#define PERSIST_KEY_MEDTHRESSOLDC 11
	
int samples = 25;
static int16_t step = 0;
static int16_t medThreesold;
static int16_t medThreesoldC;
static uint64_t lastTime = 0;

DataLoggingSessionRef my_data_log;

static void data_handler(AccelData *data, uint32_t num_samples) {
  
	static int16_t xmax = -4000;
	static int16_t xmin = 4000;
	static int16_t xavg = 0;
	static int16_t ymax = -4000;
	static int16_t ymin = 4000;
	static int16_t yavg = 0;
	static int16_t zmax = -4000;
	static int16_t zmin = 4000;
	static int16_t zavg = 0;
	static int16_t threesold = -4000;
	
	for(int i = 0; i< samples; i++){
		if(!data[i].did_vibrate && data[i].x < xmin )
			xmin = data[i].x;
		if(!data[i].did_vibrate && data[i].x > xmax )
			xmax = data[i].x;
		
		if(!data[i].did_vibrate && data[i].y < ymin )
			ymin = data[i].y;
		if(!data[i].did_vibrate && data[i].y > ymax )
			ymax = data[i].y;
		
		if(!data[i].did_vibrate && data[i].z < zmin )
			zmin = data[i].z;
		if(!data[i].did_vibrate && data[i].z > zmax )
			zmax = data[i].z;
	}
	
	xavg = (xmax + xmin)/2;
	yavg = (ymax + ymin)/2;
	zavg = (zmax + zmin)/2;
	
	threesold = MAX(xavg, MAX(yavg, zavg));
	
	if(threesold > ((medThreesold * 2) / 3)){
		
		medThreesold = (medThreesold * medThreesoldC);
		medThreesoldC++;
		medThreesold = ( medThreesold / medThreesoldC ) + (threesold / medThreesoldC);
		
		static int16_t prev = 0;
		if(threesold == xavg)
			prev = data[0].x;
		else if(threesold == yavg)
			prev = data[0].y;
		else if(threesold == zavg)
			prev = data[0].z;

		int16_t prevStep = step;
		
		for(int i = 1; i< samples; i++){
			if(data[i].timestamp - lastTime > 2000){
				lastTime = data[i].timestamp;
			}	else if(data[i].timestamp - lastTime > 200) {
				if(threesold == xavg){
					if( prev > threesold && data[i].x < threesold)
						step++;	
					prev = data[i].x;
				}else if(threesold == yavg){
					if( prev > threesold && data[i].y < threesold )
						step++;	
					prev = data[i].y;
				}else if(threesold == zavg){
					if( prev > threesold && data[i].z < threesold )
						step++;
					lastTime = data[i].timestamp;
					prev = data[i].z;
				}
			}
		}
		
			AppWorkerMessage msg_data = {
				.data0 = step - prevStep,
				.data1 = medThreesold
			};

			app_worker_send_message(0, &msg_data);
		
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {	
	data_logging_log(my_data_log, &step, 1);
	
	step = 0;
}

static void worker_init() {
	
	medThreesold = persist_exists(PERSIST_KEY_MEDTHRESSOLD) ? persist_read_int(PERSIST_KEY_MEDTHRESSOLD) : 500;
	medThreesoldC = persist_exists(PERSIST_KEY_MEDTHRESSOLDC) ? persist_read_int(PERSIST_KEY_MEDTHRESSOLDC) : 1;
	
	my_data_log = data_logging_create(2210, DATA_LOGGING_INT, sizeof(int16_t), false);
	
	accel_data_service_subscribe(samples, data_handler);
	accel_service_set_sampling_rate(ACCEL_SAMPLING_50HZ);
	
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void worker_deinit() {
  accel_data_service_unsubscribe();
	tick_timer_service_unsubscribe();
	data_logging_finish(my_data_log);
	persist_write_int(PERSIST_KEY_MEDTHRESSOLD, medThreesold);
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}