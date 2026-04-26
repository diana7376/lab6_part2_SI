/* ==========================================================================
 *  ed_dht.h
 *  ECAL-layer driver for the DHT11 / DHT22 temperature-humidity sensor.
 *  Wraps Adafruit's DHT library behind a thin C interface so the rest of
 *  the application does not pull in C++ classes.
 * ========================================================================== */
#ifndef ED_DHT_H
#define ED_DHT_H

#ifdef __cplusplus
extern "C" {
#endif

/*  Initialise the DHT driver on the pin declared in app_config.h.           */
void ed_dht_setup(void);

/*  Read one temperature sample.
 *  @param out_temp_c  Pointer that receives the value in degrees Celsius.
 *  @return            1 on success, 0 on checksum / timeout failure.        */
int  ed_dht_read_temperature(float *out_temp_c);

/*  Read one humidity sample.
 *  @param out_humidity  Pointer that receives relative humidity in %.
 *  @return              1 on success, 0 on failure.                         */
int  ed_dht_read_humidity(float *out_humidity);

#ifdef __cplusplus
}
#endif

#endif /* ED_DHT_H */
