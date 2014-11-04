/*
* Copyright (c) 2009-2011, A. Buades <toni.buades@uib.es>
* All rights reserved.
*
*
* Patent warning:
*
* This file implements algorithms possibly linked to the patents
*
* # J. Hamilton Jr and J. Adams Jr, “Adaptive color plan interpolation
* in single sensor color electronic camera,” 1997, US Patent 5,629,734.
*
* # D. Cok, “Signal processing method and apparatus for producing
* interpolated chrominance values in a sampled color image signal”,
* 1987, US Patent 4,642,678.
*
* # A. Buades, T. Coll and J.M. Morel, Image data processing method by
* reducing image noise, and camera integrating means for implementing
* said method, EP Patent 1,749,278 (Feb. 7, 2007).
*
* This file is made available for the exclusive aim of serving as
* scientific tool to verify the soundness and completeness of the
* algorithm description. Compilation, execution and redistribution
* of this file may violate patents rights in certain countries.
* The situation being different for every country and changing
* over time, it is your responsibility to determine which patent
* rights restrictions apply to you before you compile, use,
* modify, or redistribute this file. A patent lawyer is qualified
* to make this determination.
* If and only if they don't conflict with any patent terms, you
* can benefit from the following license terms attached to this
* file.
*
* License:
*
* This program is provided for scientific and educational only:
* you can use and/or modify it for these purposes, but you are
* not allowed to redistribute this work or derivative works in
* source or executable form. A license must be obtained from the
* patent right holders for any other use.
*
*
*/

#include <algorithm>
#include "libdemosaicking.h"


#define BLANK 0
#define GREENPOSITION 1
#define REDPOSITION 2
#define BLUEPOSITION 3

#define DIAG 1.4142136

#undef DEBUG_GREEN1

/**
 * @file   libdemosaicking.cpp
 * @brief  Demosaicking functions: HAmilton-Adams algorithm, NLmeans based demosaicking, Chromatic components filtering
 *
 *
 *
 * @author Antoni Buades <toni.buades@uib.es>
 */


/**
 * \brief  Classical Adams-Hamilton demosaicking algorithm
 *
 *  The green channel is interpolated directionally depending on the green first and red and blue second directional derivatives.
 *  The red and blue differences with the green channel are interpolated bilinearly.
 *
 * @param[in]  ired, igreen, iblue  original cfa image
 * @param[out] ored, ogreen, oblue  demosaicked output
 * @param[in]  (redx, redy)  coordinates of the red pixel: (0,0), (0,1), (1,0), (1,1)
 * @param[in]  threshold value to consider horizontal and vertical variations equivalent and average both estimates
 * @param[in]  width, height size of the image
 *
 */

void demosaicking_adams(
  float threshold,
  float *input,
  float *ored,
  float *ogreen,
  float *oblue,
  int width,
  int height,
  int origWidth,
  int origHeight
) {

  wxCopy(input, ored, width * height);
  wxCopy(input, ogreen, width * height);
  wxCopy(input, oblue, width * height);

  // CFA Mask of color per pixel
  unsigned char* mask = (unsigned char *) malloc(width * height * sizeof(unsigned char));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (
        x + y >= origWidth - 1 &&                   // NW boundary
        y > x - origWidth - 1 &&                    // NE boundary
        x + y < origWidth + 2 * origHeight - 1 &&   // SE boundary
        x > y - origWidth                           // SW boundary
      ) {
        // The raw image has zeroes in all channels outside the sensor area
        if (y % 2 == 0) {
          mask[y * width + x] = GREENPOSITION;
          // ored[y * width + x] = 500;
          // ogreen[y * width + x] = 65535;
          // oblue[y * width + x] = 500;
        }
        else {
          if ((x / 2 % 2 + y / 2 % 2) % 2 == 0) {
            mask[y * width + x] = BLUEPOSITION;
            // ored[y * width + x] = 500;
            // ogreen[y * width + x] = 500;
            // oblue[y * width + x] = 65535;
          }
          else {
            mask[y * width + x] = REDPOSITION;
            // ored[y * width + x] = 65535;
            // ogreen[y * width + x] = 500;
            // oblue[y * width + x] = 500;
          }
        }
      }
      else {
        mask[y * width + x] = BLANK;
      }
    }
  }

  // Interpolate the green channel in the 4-pixel-wide boundary by inverse
  // distance weighting.
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if (
        (
         mask[y * width + x] == BLUEPOSITION ||
         mask[y * width + x] == REDPOSITION
        )
        // &&
        // (
        //  x + y < origWidth + 3 ||                    // NW boundary
        //  x >= y + origWidth - 3 ||                   // NE boundary
        //  x + y >= origWidth + 2 * origHeight - 5 ||  // SE boundary
        //  y >= x + origWidth - 4                      // SW boundary
        // )
      ) {
        float avg = 0;
        float weight = 0;
        int
          n = (y - 1) * width + x,
          s = (y + 1) * width + x,
          ne = (y - 1) * width + x + 1,
          se = (y + 1) * width + x + 1,
          sw = (y + 1) * width + x - 1,
          nw = (y - 1) * width + x - 1;

        if (x == 0) { // West corner red
          ogreen[y * width + x] = (ogreen[ne] + ogreen[se]) / 2;
        }
        else if (x == width - 1) { // East corner blue
          ogreen[y * width + x] = (ogreen[nw] + ogreen[sw]) / 2;
        }
        else {
          if (mask[nw] != BLANK) {
            avg += ogreen[nw] / DIAG;
            weight += 1 / DIAG;
          }
          if (mask[n] != BLANK) {
            avg += ogreen[n];
            weight += 1;
          }
          if (mask[ne] != BLANK) {
            avg += ogreen[ne] / DIAG;
            weight += 1 / DIAG;
          }

          if (mask[sw] != BLANK) {
            avg += ogreen[sw] / DIAG;
            weight += 1 / DIAG;
          }
          if (mask[s] != BLANK) {
            avg += ogreen[s];
            weight += 1;
          }
          if (mask[se] != BLANK) {
            avg += ogreen[se] / DIAG;
            weight += 1 / DIAG;
          }
          ogreen[y * width + x] = avg / weight;
        }
      }
    }
  }

#ifdef OK
  // Interpolate the green by Adams algorithm inside the image.
  // First interpolate green directionally.
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int p = y * width + x;
      if (
        mask[p] != GREENPOSITION &&
        x + y >= origWidth - 1 + 4 &&                   // NW boundary
        y > x - origWidth - 1 + 4 &&                    // NE boundary
        x + y < origWidth + 2 * origHeight - 1 - 4 &&   // SE boundary
        x > y - origWidth + 4                          // SW boundary
      ) {
        int
          n = (y - 1) * width + x,         // lm1
          s = (y + 1) * width + x,         // lp1
          e = y * width + x + 1,
          w = y * width + x - 1,
          ne = (y - 1) * width + x + 1,
          se = (y + 1) * width + x + 1,
          sw = (y + 1) * width + x - 1,
          nw = (y - 1) * width + x - 1,
          n2 = (y - 2) * width + x,        // lm2
          s2 = (y + 2) * width + x,        // lp2
          ne2 = (y - 2) * width + x + 2,
          se2 = (y + 2) * width + x + 2,
          sw2 = (y + 2) * width + x - 2,
          nw2 = (y - 2) * width + x - 2;

        // Compute vertical, horizontal, and diagonal gradients in the green channel.
        //
        // Horizontal: interpolate the greens
        //
        // G   G
        // |-*-|
        // G   G
        //
        float gh = fabsf((ogreen[nw] + ogreen[sw]) / 2 - (ogreen[ne] + ogreen[se]) / 2);

        // Vertical
        float gv = fabsf(ogreen[s] - ogreen[n]);

        // NW - SE
        float gnw = fabsf(ogreen[nw] - ogreen[se]) / DIAG; // diagonal distance is longer

        // NE - SW
        float gne = fabsf(ogreen[ne] - ogreen[sw]) / DIAG;

        float d2h, d2v, d2nw, d2ne;
        float min, max;

        // Compute vertical, horizontal, and diagonal second derivatives in the same channel as
        // current pixel.
        if (mask[p] == REDPOSITION) {
          // Horizontal: interpolate green and red values
          //
          //    Even       Odd
          //
          // . R . . .   . . . R .
          // . | . . .   . . . | .
          // . + * R .   . R * + .
          // . | . . .   . . . | .
          // . R . . .   . . . R .
          //
          if (x % 2 == 0) {
            d2h = 2.0 * ored[p] - (ored[nw2 + 1] + ored[sw2 + 1] / 2.0) - ored[e];
          }
          else {
            d2h = 2.0 * ored[p] - ored[w] - (ored[ne2 - 1] + ored[se2 - 1]) / 2.0;
          }

          // Vertical: interpolate green and red values
          //
          //    Even       Odd
          //
          // . R-+---R   R---+-R .   <- unequal distance, interpolate with IDW
          // . . | . .   . . | . .
          // . . * . .   . . * . .
          // . . | . .   . . | . .
          // . R-+---R   R---+-R .
          //
          if (x % 2 == 0) {
            d2v = (
              2.0 * ored[p] -
              (ored[nw2 + 1] + ored[ne2] / 2.0) / 1.5 -
              (ored[sw2 + 1] + ored[se2] / 2.0) / 1.5
            ) / 4.0; // step = 2
          }
          else {
            d2v = (
              2.0 * ored[p] -
              (ored[nw2] / 2.0 + ored[ne2 - 1]) / 1.5 -
              (ored[sw2] / 2.0 + ored[se2 - 1]) / 1.5
            ) / 4.0;
          }

          // Diagonal
          //
          //     NW         NE
          //
          // R . . . .   . . . . R
          // . . . . .   . . . . .
          // . . * . .   . . * . .
          // . . . . .   . . . . .
          // . . . . R   R . . . .
          //
          d2nw = (2.0 * ored[p] - ored[nw2] - ored[se2]) / 8.0; // step = 2 * DIAG
          d2ne = (2.0 * ored[p] - ored[ne2] - ored[sw2]) / 8.0;
        }
        else { // BLUEPOSITION
          // Horizontal: interpolate green and red values
          //
          //    Even       Odd
          //
          // . R . . .   . . . R .
          // . | . . .   . . . | .
          // . + * R .   . R * + .
          // . | . . .   . . . | .
          // . R . . .   . . . R .
          //
          if (x % 2 == 0) {
            d2h = 2.0 * oblue[p] - (oblue[nw2 + 1] + oblue[sw2 + 1] / 2.0) - oblue[e];
          }
          else {
            d2h = 2.0 * oblue[p] - oblue[w] - (oblue[ne2 - 1] + oblue[se2 - 1]) / 2.0;
          }

          // Vertical: interpolate green and red values
          //
          //    Even       Odd
          //
          // . R-+---R   R---+-R .   <- unequal distance, interpolate with IDW
          // . . | . .   . . | . .
          // . . * . .   . . * . .
          // . . | . .   . . | . .
          // . R-+---R   R---+-R .
          //
          if (x % 2 == 0) {
            d2v = (
              2.0 * oblue[p] -
              (oblue[nw2 + 1] + oblue[ne2] / 2.0) / 1.5 -
              (oblue[sw2 + 1] + oblue[se2] / 2.0) / 1.5
            ) / 4.0; // step = 2
          }
          else {
            d2v = (
              2.0 * oblue[p] -
              (oblue[nw2] / 2.0 + oblue[ne2 - 1]) / 1.5 -
              (oblue[sw2] / 2.0 + oblue[se2 - 1]) / 1.5
            ) / 4.0;
          }

          // Diagonal
          //
          //     NW         NE
          //
          // R . . . .   . . . . R
          // . . . . .   . . . . .
          // . . * . .   . . * . .
          // . . . . .   . . . . .
          // . . . . R   R . . . .
          //
          d2nw = (2.0 * oblue[p] - oblue[nw2] - oblue[se2]) / 8.0; // step = 2 * DIAG
          d2ne = (2.0 * oblue[p] - oblue[ne2] - oblue[sw2]) / 8.0;
        }

        // Add second differences to gradients
        gh += fabsf(d2h);
        gv += fabsf(d2v);
        gnw += fabsf(d2nw);
        gne += fabsf(d2ne);

        min = std::min( std::min(gh, gv), std::min(gnw, gne) );
        max = std::max( std::max(gh, gv), std::max(gnw, gne) );

#ifdef DEBUG_GREEN1
        if (mask[p] == REDPOSITION) {
          printf("\x1b[31m--------------------------------------------------------------\x1b[0m\n");
          printf("\x1b[31mred\x1b[0m, %s (%d, %d)\n", x % 2 ? "odd" : "even", x, y);
          if (x % 2 == 0) {
            printf("\x1b[31m%4.0f%4.0f\x1b[34m%4.0f%4.0f\x1b[31m%4.0f\x1b[0m\n",   input[nw2], input[nw2 + 1], input[n2], input[ne2 - 1], input[ne2]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[nw],      input[n],  input[ne]);
            printf( "    \x1b[34m%4.0f\x1b[1m\x1b[31m%4.0f\x1b[0m\x1b[31m%4.0f\x1b[0m\n",                                    input[w],       input[p],  input[e]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[sw],      input[s],  input[se]);
            printf("\x1b[31m%4.0f%4.0f\x1b[34m%4.0f%4.0f\x1b[31m%4.0f\x1b[0m\n",   input[sw2], input[sw2 + 1], input[s2], input[se2 - 1], input[se2]);
          }
          else {
            printf("\x1b[31m%4.0f\x1b[34m%4.0f%4.0f\x1b[31m%4.0f%4.0f\x1b[0m\n",   input[nw2], input[nw2 + 1], input[n2], input[ne2 - 1], input[ne2]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[nw],      input[n],  input[ne]);
            printf( "    \x1b[31m%4.0f\x1b[1m%4.0f\x1b[0m\x1b[34m%4.0f\x1b[0m\n",                                    input[w],       input[p],  input[e]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[sw],      input[s],  input[se]);
            printf("\x1b[31m%4.0f\x1b[34m%4.0f%4.0f\x1b[31m%4.0f%4.0f\x1b[0m\n",   input[sw2], input[sw2 + 1], input[s2], input[se2 - 1], input[se2]);
          }
        }
        else { // blue
          printf("\x1b[34m--------------------------------------------------------------\x1b[0m\n");
          printf("\x1b[34mblue\x1b[0m, %s (%d, %d)\n", x % 2 ? "odd" : "even", x, y);
          if (x % 2 == 0) {
            printf("\x1b[34m%4.0f%4.0f\x1b[31m%4.0f%4.0f\x1b[34m%4.0f\x1b[0m\n",   input[nw2], input[nw2 + 1], input[n2], input[ne2 - 1], input[ne2]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[nw],      input[n],  input[ne]);
            printf( "    \x1b[31m%4.0f\x1b[1m\x1b[34m%4.0f\x1b[0m\x1b[34m%4.0f\x1b[0m\n",                                    input[w],       input[p],  input[e]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[sw],      input[s],  input[se]);
            printf("\x1b[34m%4.0f%4.0f\x1b[31m%4.0f%4.0f\x1b[34m%4.0f\x1b[0m\n",   input[sw2], input[sw2 + 1], input[s2], input[se2 - 1], input[se2]);
          }
          else {
            printf("\x1b[34m%4.0f\x1b[31m%4.0f%4.0f\x1b[34m%4.0f%4.0f\x1b[0m\n",   input[nw2], input[nw2 + 1], input[n2], input[ne2 - 1], input[ne2]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[nw],      input[n],  input[ne]);
            printf( "    \x1b[34m%4.0f\x1b[1m%4.0f\x1b[0m\x1b[31m%4.0f\x1b[0m\n",                                    input[w],       input[p],  input[e]);
            printf( "    \x1b[32m%4.0f%4.0f%4.0f\x1b[0m\n",                                    input[sw],      input[s],  input[se]);
            printf("\x1b[34m%4.0f\x1b[31m%4.0f%4.0f\x1b[34m%4.0f%4.0f\x1b[0m\n",   input[sw2], input[sw2 + 1], input[s2], input[se2 - 1], input[se2]);
          }
        }
#endif

        // If all differences are similar, compute an isotropic average.
        // if (max - min < threshold) {
        if (1) {
#ifdef DEBUG_GREEN1
          printf(  "g # (- %f, | %f, \\ %f, / %f)\n", gh, gv, gnw, gne);
#endif
          //ogreen[p] =
          //  (ogreen[nw] +  ogreen[n] + ogreen[ne] + ogreen[se] + ogreen[s] + ogreen[sw]) / 6.0 +
          //  (d2h + d2v + d2nw + d2ne) / 4.0;
          ogreen[p] = (ogreen[nw] + ogreen[n] + ogreen[ne] + ogreen[se] + ogreen[s] + ogreen[sw]) / 6.0;
        }
        else {
          if (min == gh) {
            // Compute horizontal average (interpolated for green, so almost same as isotropic).
            //
            // G . G
            // |-*-|
            // G . G
            //
#ifdef DEBUG_GREEN1
            printf(  "g - (- %f, | %f, \\ %f, / %f)\n", gh, gv, gnw, gne);
#endif
            // ogreen[p] = (ogreen[nw] + ogreen[sw] + (ogreen[ne] + ogreen[se]) / 4.0) + d2h;
            ogreen[p] = (ogreen[nw] + ogreen[sw] + (ogreen[ne] + ogreen[se]) / 4.0);
          }
          if (min == gv) {
            // Vertical average
#ifdef DEBUG_GREEN1
            printf(  "g | (- %f, | %f, \\ %f, / %f)\n", gh, gv, gnw, gne);
#endif
            // ogreen[p] = (ogreen[s] + ogreen[n])/2.0 + d2v;
            ogreen[p] = (ogreen[s] + ogreen[n])/2.0;
          }
          if (min == gnw) {
            // NW average
#ifdef DEBUG_GREEN1
            printf(  "g \\ (- %f, | %f, \\ %f, / %f)\n", gh, gv, gnw, gne);
#endif
            // ogreen[p] = (ogreen[nw] + ogreen[se])/2.0 + d2nw;
            ogreen[p] = (ogreen[nw] + ogreen[se])/2.0;
          }
          if (min == gne) {
            // NE
#ifdef DEBUG_GREEN1
            printf(  "g / (- %f, | %f, \\ %f, / %f)\n", gh, gv, gnw, gne);
#endif
            // ogreen[p] = (ogreen[ne] + ogreen[sw])/2.0 + d2ne;
            ogreen[p] = (ogreen[ne] + ogreen[sw])/2.0;
          }
        }
#ifdef DEBUG_GREEN1
        printf("    \x1b[32mavg: %f\x1b[0m\n", ogreen[p]);
#endif
      }
    }
  }
#endif

  // compute the bilinear on the differences of the red and blue with the already interpolated green
  //demosaicking_bilinear_red_blue(redx,redy,ored,ogreen,oblue,width,height);


  free(mask);
}




/**
 * \brief  Classical bilinear interpolation of red and blue differences with the green channel
 *
 *
 * @param[in]  ored, ogreen, oblue  original cfa image with green interpolated
 * @param[out] ored, ogreen, oblue  demosaicked output
 * @param[in]  (redx, redy)  coordinates of the red pixel: (0,0), (0,1), (1,0), (1,1)
 * @param[in]  width, height size of the image
 *
 */

void demosaicking_bilinear_red_blue(
  float *ored,
  float *ogreen,
  float *oblue,
  int width,
  int height,
  int origWidth,
  int origHeight
) {


  //Initializations
  int bluex, redx;
  int bluey, redy;


  // Mask of color per pixel
  unsigned char* mask = (unsigned char *) malloc(width*height*sizeof(unsigned char));

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (
        x + y >= origWidth - 1 &&                   // NW boundary
        y > x - origWidth - 1 &&                    // NE boundary
        x + y < origWidth + 2 * origHeight - 1 &&   // SE boundary
        x > y - origWidth                           // SW boundary
      ) {
        // The raw image has zeroes in all channels outside the sensor area
        if (y % 2 == 0) {
          mask[y * width + x] = GREENPOSITION;
          // ored[y * width + x] = 500;
          // ogreen[y * width + x] = 65535;
          // oblue[y * width + x] = 500;
        }
        else {
          if ((x / 2 % 2 + y / 2 % 2) % 2 == 0) {
            mask[y * width + x] = BLUEPOSITION;
            // ored[y * width + x] = 500;
            // ogreen[y * width + x] = 500;
            // oblue[y * width + x] = 65535;
          }
          else {
            mask[y * width + x] = REDPOSITION;
            // ored[y * width + x] = 65535;
            // ogreen[y * width + x] = 500;
            // oblue[y * width + x] = 500;
          }
        }
      }
      else {
        mask[y * width + x] = BLANK;
      }
    }
  }



  // Compute the differences
  for(int i=0; i < width*height;i++){

    ored[i] -= ogreen[i];
    oblue[i] -= ogreen[i];

  }



  // Interpolate the blue differences making the average of possible values depending on the CFA structure
  for(int x=0; x < width;x++)
    for(int y=0; y < height;y++)
      if (mask[y*width+x] != BLUEPOSITION){

        int gn, gs, ge, gw;

        // Compute north, south, west, east positions
        // taking a mirror symmetry at the boundaries
        if (y > 0)  gn = y-1; else    gn = 1;
        if (y < height-1) gs = y+1;  else  gs = height - 2 ;
        if (x < width-1)  ge = x+1;  else  ge = width-2;
        if (x > 0) gw = x-1;  else  gw = 1;

        if (mask[y*width+x] == GREENPOSITION && y % 2 == bluey)
          oblue[y*width+x] = ( oblue[y*width+ge] + oblue[y*width+gw])/2.0;
        else if (mask[y*width+x] == GREENPOSITION  && x % 2 == bluex)
          oblue[y*width+x] = ( oblue[gn*width+x] + oblue[gs*width+x])/2.0;
        else {
          oblue[y*width+x] =  (oblue[gn*width+ge] + oblue[gn*width + gw]  +  oblue[gs*width + ge] +  oblue[gs*width +gw])/4.0;
        }

      }




  // Interpolate the red differences making the average of possible values depending on the CFA structure
  for(int x=0;x<width;x++)
    for(int y=0;y<height;y++)
      if (mask[y*width+x] != REDPOSITION){

        int gn, gs, ge, gw;

        // Compute north, south, west, east positions
        // taking a mirror symmetry at the boundaries
        if (y > 0)  gn = y-1; else    gn = 1;
        if (y < height-1) gs = y+1;  else  gs = height - 2 ;
        if (x < width-1)  ge = x+1;  else  ge = width-2;
        if (x > 0) gw = x-1;  else  gw = 1;

        if (mask[y*width+x] == GREENPOSITION && y % 2 == redy)
          ored[y*width+x] = ( ored[y*width+ge] + ored[y*width+gw])/2.0;
        else if (mask[y*width+x] == GREENPOSITION  && x % 2 == redx)
          ored[y*width+x] = ( ored[gn*width+x] + ored[gs*width+x])/2.0;
        else {
          ored[y*width+x] =  (ored[gn*width+ge] + ored[gn*width + gw]  +  ored[gs*width + ge] +  ored[gs*width +gw])/4.0;
        }

      }


  // Make back the differences
  for(int i=0;i<width*height;i++){

    ored[i] += ogreen[i];
    oblue[i] += ogreen[i];
  }



  free(mask);

}







/**
 * \brief  NLmeans based demosaicking
 *
 * For each value to be filled, a weigthed average of original CFA values of the same channel is performed.
 * The weight depends on the difference of a 3x3 color patch
 *
 * @param[in]  ired, igreen, iblue  initial demosaicked image
 * @param[out] ored, ogreen, oblue  demosaicked output
 * @param[in]  (redx, redy)  coordinates of the red pixel: (0,0), (0,1), (1,0), (1,1)
 * @param[in]  bloc  research block of size (2+bloc+1) x (2*bloc+1)
 * @param[in]  h kernel bandwidth
 * @param[in]  width, height size of the image
 *
 */


void demosaicking_nlmeans(int bloc, float h,int redx,int redy,float *ired,float *igreen,float *iblue,float *ored,float *ogreen,float *oblue,int width,int height)
{


  // Initializations
  int bluex = 1 - redx;
  int bluey = 1 - redy;



  // CFA Mask of color per pixel
  unsigned char *cfamask = new unsigned char[width*height];


  for(int x=0;x<width;x++)
    for(int y=0;y<height;y++){

      if (x%2 == redx && y%2 == redy) cfamask[y*width+x] = REDPOSITION;
      else  if (x%2 == bluex && y%2 == bluey) cfamask[y*width+x] = BLUEPOSITION;
      else  cfamask[y*width+x] = GREENPOSITION;

    }


  wxCopy(ired,ored,width*height);
  wxCopy(igreen,ogreen,width*height);
  wxCopy(iblue,oblue,width*height);



  // Tabulate the function Exp(-x) for x>0.
  int luttaille = (int) (LUTMAX*LUTPRECISION);
  float *lut = new float[luttaille];

  sFillLut(lut, luttaille);




  // for each pixel
  for(int y=2; y <height-2; y++)
    for(int x=2; x<width-2; x++)
    {
      // index of current pixel
      int l=y*width+x;


      // Learning zone depending on the window size
      int imin=MAX(x-bloc,1);
      int jmin=MAX(y-bloc,1);

      int imax=MIN(x+bloc,width-2);
      int jmax=MIN(y+bloc,height-2);


      // auxiliary variables for computing average
      float red=0.0;
      float green=0.0;
      float blue=0.0;

      float rweight=0.0;
      float gweight=0.0;
      float bweight=0.0;


      // for each pixel in the neighborhood
      for(int j=jmin;j<=jmax;j++)
        for(int i=imin;i<=imax;i++) {

          // index of neighborhood pixel
          int l0=j*width+i;

          // We only interpolate channels differents of the current pixel channel
          if (cfamask[l]!=cfamask[l0]) {


            // Distances computed on color
            float some = 0.0;

            some = l2_distance_r1(ired,  x, y, i,
                        j, width);
            some += l2_distance_r1(igreen,  x, y, i,
                         j, width);
            some += l2_distance_r1(iblue,  x, y, i,
                         j, width);



            // Compute weight
            some= some / (27.0 * h);

            float weight = sLUT(some,lut);

            // Add pixel to corresponding channel average

            if (cfamask[l0] == GREENPOSITION)  {

              green += weight*igreen[l0];
              gweight+= weight;

            } else if (cfamask[l0] == REDPOSITION) {

              red += weight*ired[l0];
              rweight+= weight;

            } else {

              blue += weight*iblue[l0];
              bweight+= weight;
            }

          }

        }


      // Set value to current pixel
      if (cfamask[l] != GREENPOSITION && gweight > fTiny)  ogreen[l]  =   green / gweight;
      else  ogreen[l] = igreen[l];

      if ( cfamask[l] != REDPOSITION && rweight > fTiny)  ored[l]  =  red / rweight ;
      else    ored[l] = ired[l];

      if  (cfamask[l] != BLUEPOSITION && bweight > fTiny)   oblue[l] =  blue / bweight;
      else  oblue[l] = iblue[l];


    }

  delete[] cfamask;
  delete[] lut;

}




/**
 * \brief  Iterate median filter on chromatic components of the image
 *
 *
 * @param[in]  ired, igreen, iblue  initial  image
 * @param[in]  iter  number of iteracions
 * @param[out] ored, ogreen, oblue  filtered output
 * @param[in]  (redx, redy)  coordinates of the red pixel: (0,0), (0,1), (1,0), (1,1)
 * @param[in]  side  median in a (2*side+1) x (2*side+1) window
 * @param[in]  projflag if not zero, values of the original CFA are kept
 * @param[in]  width, height size of the image
 *
 */


void chromatic_median(int iter,int redx,int redy,int projflag,float side,float *ired,float *igreen, float *iblue,float *ored,float *ogreen,float *oblue,int width,int height)
{


  int size=height*width;

  // Auxiliary variables for computing chromatic components
  float *y=new float[size];
  float *u=new float[size];
  float *v=new float[size];
  float *u0=new float[size];
  float *v0=new float[size];

  int bluex=1-redx;
  int bluey=1-redy;


  // For each iteration
  for(int i=1;i<=iter;i++){


    // Transform to YUV
    wxRgb2Yuv(ired,igreen,iblue,y,u,v,width,height);

    // Perform a Median on YUV component
    wxMedian(u,u0,side,1 ,width,height);
    wxMedian(v,v0,side,1 ,width,height);


    // Transform back to RGB
    wxYuv2Rgb(ored,ogreen,oblue,y,u0,v0,width,height);

    // If projection flag activated put back original CFA values
    if (projflag)
    {

      for(int x=0;x<width;x++)
        for(int y=0;y<height;y++){

          int l=y*width+x;

          if (x%2==redx && y%2==redy) ored[l]=ired[l];

          else if (x%2==bluex && y%2==bluey) oblue[l]=iblue[l];

          else ogreen[l]=igreen[l];

        }

    }


    wxCopy(ored,ired,size);
    wxCopy(ogreen,igreen,size);
    wxCopy(oblue,iblue,size);

  }


  // delete auxiliary memory
  delete[] y;
  delete[] u;
  delete[] u0;
  delete[] v;
  delete[] v0;

}


/**
 * \brief Demosaicking chain
 *
 *
 *
 * Compute initialization by Adams-Hamilton algorithm (u0)
 *
 * for h in {16,4,1} do
 * {
 *
 *    u <- NL_h(u0);      Apply NLmeans demosaicking
 *
 *    u <- CR(u);       Apply chromatic regularization
 *
 *    u0 <- u;
 *
 * }
 *
 * Output <- u;
 *
 *
 * @param[in]  ired, igreen, iblue  initial  image
 * @param[out] ored, ogreen, oblue  filtered output
 * @param[in]  (redx, redy)  coordinates of the red pixel: (0,0), (0,1), (1,0), (1,1)
 * @param[in]  width, height size of the image
 *
 */



void ssd_demosaicking_chain(
  float *input,
  float *ored,
  float *ogreen,
  float *oblue,
  int width,
  int height,
  int origWidth,
  int origHeight
) {

  ////////////////////////////////////////////// Process

  float h;
  int dbloc = 7;
  float side = 1.5;
  int iter = 1;
  int projflag = 1;
  float threshold = 2.0;


  demosaicking_adams(threshold, input, ored, ogreen, oblue, width, height, origWidth, origHeight);
  printf("demosaicking_adams() -> %f\n", ogreen[8990]);

//
//
//   h = 16.0;
//   demosaicking_nlmeans(dbloc,h,redx,redy,ored,ogreen,oblue,ired,igreen,iblue,width,height);
//   chromatic_median(iter,redx,redy,projflag,side,ired,igreen,iblue,ored,ogreen,oblue,width,height);
//
//
//
//   h = 4.0;
//   demosaicking_nlmeans(dbloc,h,redx,redy,ored,ogreen,oblue,ired,igreen,iblue,width,height);
//   chromatic_median(iter,redx,redy,projflag,side,ired,igreen,iblue,ored,ogreen,oblue,width,height);
//
//
//
//   h = 1.0;
//   demosaicking_nlmeans(dbloc,h,redx,redy,ored,ogreen,oblue,ired,igreen,iblue,width,height);
//   chromatic_median(iter,redx,redy,projflag,side,ired,igreen,iblue,ored,ogreen,oblue,width,height);
}


