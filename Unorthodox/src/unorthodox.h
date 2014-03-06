
/*
  nano operating system for the Arduino (Leonardo)  (c) Jeremy Lee 2013
 */

#ifndef unorthodox_h
#define unorthodox_h

// AVR classes
#if defined(__AVR__)
#include <Arduino.h>
#include <unorthodox_page.h>
#include <unorthodox_queues.h>
#include <unorthodox_trees.h>
#include <unorthodox_device.h>
#include <unorthodox_drivers.h>
#include <unorthodox_drivers_i2c.h>
#include <unorthodox_drivers_spi.h>
#include <unorthodox_cursor.h>
#include <unorthodox_raster.h>
#include <unorthodox_droid.h>
// #include <unorthodox_droid_gfx.h>
// #include <unorthodox_droid_raster.h>
#endif

#endif
