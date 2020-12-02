#include "AP_GPS.h"
//#include <random>
#include <GCS_MAVLink/GCS.h>

#define STL_ATK_TYPE 4

int getRandomInt(int a, int b);

int getRandomInt(int a, int b){
    return (rand() % (b-a + 1)) + a;
}

const Location & AP_GPS::location() const{

    //This attack is for a bit higher level signal. 
    //  location(uint8_t instance) is for lower level data, but I didn't change it,
    //  So, the log data in the bin file will give the correct GPS data, but EKF
    //  will use the wrong value.
    // This way can help us see the real location of the vehicle in the SITL map.

    
    const Location &true_loc = location(primary_instance);
    //============Bob: GPS Stealthy attack===============
    //gcs().send_text(MAV_SEVERITY_INFO, "True Location: lat: %d, lng: %d", true_loc.lat, true_loc.lng);
    //hal.console->printf("True Location2: lat: %d, lng: %d", true_loc.lat, true_loc.lng);
    // if(_ins_bob->check_stealthy_atk()){
    //     
    // }
    return get_fake_location(true_loc);
    //===================================================
    // return true_loc;
}

const Location & AP_GPS::get_fake_location(const Location& true_loc) const{

#if  STL_ATK_TYPE == 1
    //Attack type 1
    (*fake_loc_ptr) = true_loc;
    int range_down = 200;
    int range_up = 300;
    if(rand() % 2 == 0){
        fake_loc_ptr->lat += getRandomInt(range_down, range_up);
        fake_loc_ptr->lng += getRandomInt(range_down, range_up);
    }else{
        fake_loc_ptr->lat -= getRandomInt(range_down, range_up);
        fake_loc_ptr->lng -= getRandomInt(range_down, range_up);
    }

    // if(rand() % 5 == 0){
    //     gcs().send_text(MAV_SEVERITY_INFO, "random number: %d", getRandomInt(range_down, range_down));
    //     AP_HAL::get_HAL().console->printf("console random number: %d\n", getRandomInt(range_down, range_up));
    // }
#elif STL_ATK_TYPE == 2
    //Attack Type 2
    static Location fixed_loc = true_loc;
    static bool is_fix_loc = false;
    if(is_fix_loc){
        fixed_loc.lat -= 100;
        is_fix_loc = true;
    }
    (*fake_loc_ptr) = fixed_loc;

#elif STL_ATK_TYPE == 3
    static bool first = true;
    static Location fixed_fake_loc;
    if(first){
        fixed_fake_loc = true_loc;
        fixed_fake_loc.lng += 100;
        first = false;
    }
    //fix longitude to an offset
    (*fake_loc_ptr) = true_loc;
    fake_loc_ptr->lng = fixed_fake_loc.lng + getRandomInt(-10, 10);
    //we don't update from previous longitude because that can accumulate
    //and cause large offset and find by detector. Using fixed value can limit
    //longitude to a specific range

#elif STL_ATK_TYPE == 4
    static float increase_rate = 2; //2 --> 0.5m/s,  1 --> 0.25m/s
    static int last_inc = 0;
    static int frame_cnt = 0;
    static int frame_skip = -1;
    static int log_interval = 5;

    increase_rate = _ins_bob->gps_atk_rate;

    if(_ins_bob->stealthy_atk_param == 1){
        if(increase_rate < 1){
            if(frame_skip == -1){
                frame_skip = (int)(1/increase_rate);
            }
            if(frame_cnt == frame_skip){
                frame_cnt = 0;
                last_inc += 1;
            }else{
                frame_cnt++;
            }
        }else{
            last_inc += (int)increase_rate;
        }
    }
    
    (*fake_loc_ptr) = true_loc;
    fake_loc_ptr->lng += last_inc;

    log_interval--;
    if(log_interval <= 0){
        // DataFlash_Class::instance()->Write_BOBL(20, (int)(Location_Class(*fake_loc_ptr).get_distance(true_loc) * 100));
        DataFlash_Class::instance()->Write_BOBL(20, (int)(get_distance((*fake_loc_ptr), true_loc) * 100));
        log_interval = 5;
    }
    
#endif

    return (*fake_loc_ptr);
    //gcs().send_text(MAV_SEVERITY_INFO, "True data Lat: %d, Lng: %d", true_loc.lat, true_loc.lng);
    //gcs().send_text(MAV_SEVERITY_INFO, "Fake data Lat: %d, Lng: %d", fake_loc_ptr->lat, fake_loc_ptr->lng);
}