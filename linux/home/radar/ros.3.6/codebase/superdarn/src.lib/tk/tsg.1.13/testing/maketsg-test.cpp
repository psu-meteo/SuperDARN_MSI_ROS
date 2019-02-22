// maketsg-test.c
// Author: Bryant Klug
// Created: 2/19/2019

extern "C" {
#include "../include/maketsg.h"
#include "../include/tsg.h"
}
#include "catch.hpp"
#include <cstring>

extern "C" struct TSGbuf * TSGMake(struct TSGprm *, int *);

TEST_CASE( "Test TSGMake flags" ){

  struct TSGprm test_tsg;
  memset(&test_tsg, 0, sizeof(struct TSGprm));
  test_tsg.nrang   = 200;
  test_tsg.frang   = 180;
  test_tsg.rsep    = 45;
  test_tsg.txpl    = 300;
  test_tsg.mppul   = 8;
  test_tsg.mpinc   = 1500;
  test_tsg.nbaud   = 1;
  test_tsg.stdelay = 18 + 2;
  test_tsg.gort    = 1;
  test_tsg.rtoxmin = 0;

  int pulse_table[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  test_tsg.pat     = (int*)malloc(4*8);
  test_tsg.code    = pulse_table;

  for(int i = 0; i < test_tsg.mppul; i++) {
    test_tsg.pat[i] = pulse_table[i];
  }

  struct TSGbuf * test_buffer = NULL;
  int flag = 0;

  SECTION( "Invalid range separation, negative distance to first range" ) {
    INFO( "Flag 1, TSG_INV_RSEP is invalid range separation" );
    test_tsg.frang = -1;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_RSEP );
  }
  SECTION( "Invalid range separation, negative range gate separation" ) {
    INFO( "Flag 1, TSG_INV_RSEP is invalid range separation" );
    test_tsg.rsep = -1;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_RSEP );
  }
  SECTION( "Invalid range separation, zero range gate separation" ) {
    INFO( "Flag 1, TSG_INV_RSEP is invalid range separation" );
    test_tsg.rsep = 0;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_RSEP );
  }

  SECTION( "No sample separation, " ) {
    INFO( "Flag 2, TSG_NO_SMSEP is no sample separation" );
    test_tsg.smsep = 0;
    test_tsg.txpl  = 0;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_NO_SMSEP );
  }

  SECTION( "Invalid number of pulses and sample separation, " ) {
    INFO( "Flag 3, TSG_INV_MPPUL_SMSEP" );
    test_tsg.mppul = 0;
    test_tsg.smsep = 0;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_MPPUL_SMSEP );
  }

  SECTION( "Invalid pulse pattern, " ) {
    INFO( "Flag 4, TSG_INV_PAT" );
    test_tsg.pat[1] = 10;
    test_tsg.pat[2] = 9;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_PAT );
  }

  SECTION( "Pulse increment is smaller than the sample separation" ) {
    INFO( "Flag 5, TSG_INV_MPINC_SMSEP" );
    INFO( "smsep is set the code to be txpl / nbaud and divisible by CLOCK_PERIOD");
    test_tsg.mpinc = 299;
    test_tsg.smsep = 300; /* txpl and nbaud give this smsep */
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_MPINC_SMSEP );
  }

  SECTION( "Lag to the first range is not divisible by the sample separation" ) {
    INFO( "Flag 6, TSG_INV_LAGFR_SMSEP" );
    INFO( "lagfr is set in the code to be frang * 20 / 3" );
    test_tsg.frang = 180;
    test_tsg.lagfr = 1200; /* frang gives this lagfr */
    test_tsg.txpl = 500;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_LAGFR_SMSEP );
  }

// TODO: test the rest of the variables in the duty cycle test
  SECTION( "The duty cycle of the sequence is too high" ) {
    INFO( "Flag 7, TSG_INV_DUTY_CYCLE" );
    test_tsg.nrang = 116;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_DUTY_CYCLE );
  }

  SECTION( "The sample separation is an odd number" ) {
    INFO( "Flag 8, TSG_INV_ODD_SMSEP" );
    test_tsg.nbaud = 2;
    test_buffer = TSGMake(&test_tsg, &flag);
    REQUIRE( flag == TSG_INV_ODD_SMSEP );
  }
  INFO( "Flag 9, TSG_INV_TXPL_BAUD, UNTESTED" );
  INFO( "Flag 10, TSG_INV_MEMORY, UNTESTED" );
  INFO( "Flag 11, TSG_INV_PHASE_DELAY, UNTESTED" ); //phase_delay == 0

  SECTION( "TSGMake works if the parameters aren't bad" ){
    test_buffer = TSGMake(&test_tsg, &flag);
    INFO( "nrange: " << test_tsg.nrang << " frang: " << test_tsg.frang);
    INFO( "rsep: " << test_tsg.rsep << " smsep: " << test_tsg.smsep);
    INFO( "lagfr: " << test_tsg.lagfr << " txpl: " << test_tsg.txpl);
    INFO( "mppul: " << test_tsg.mppul << " mpinc: " << test_tsg.mpinc);
    INFO( "mlag: " << test_tsg.mlag << " nbaud: " << test_tsg.nbaud);
    INFO( "samples: " << test_tsg.samples << " smdelay: " << test_tsg.smdelay);
    INFO( "stdelay: " << test_tsg.stdelay << " gort: " << test_tsg.gort);

    REQUIRE( flag == TSG_OK);
  }

  free(test_tsg.pat);
}
