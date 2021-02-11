/*
 * curve25519.cpp
 *
 * Generic 64-bit integer implementation of Curve25519 ECDH
 * adapted from Matthijs van Duin's public domain
 * C implementation version 200608242056, which is based on
 * work by Daniel J. Bernstein (http://cr.yp.to/ecdh.html),
 * which is also in public domain.
 *
 */

#include "x25519dh.h"

namespace top
{
    namespace utl
    {
        
        typedef int i25519[10];
        
        /********************* radix 2^25.5 GF(2^255-19) math *********************/
        
        #define P25 33554431    // (1 << 25) - 1
        #define P26 67108863    // (1 << 26) - 1
        
        // convenience macros
        #define M(i) ((unsigned int) m[i])
        #define X(i) ((int64_t) x[i])
        #define m64(arg1,arg2) ((int64_t) (arg1) * (arg2))
        
        
        // Convert to internal format from little-endian uint8 format
        static void unpack25519(i25519 x, const key25519 m)
        {
            x[0] =  M( 0)         | M( 1)<<8 | M( 2)<<16 | (M( 3)& 3)<<24;
            x[1] = (M( 3)&~ 3)>>2 | M( 4)<<6 | M( 5)<<14 | (M( 6)& 7)<<22;
            x[2] = (M( 6)&~ 7)>>3 | M( 7)<<5 | M( 8)<<13 | (M( 9)&31)<<21;
            x[3] = (M( 9)&~31)>>5 | M(10)<<3 | M(11)<<11 | (M(12)&63)<<19;
            x[4] = (M(12)&~63)>>6 | M(13)<<2 | M(14)<<10 |  M(15)    <<18;
            x[5] =  M(16)         | M(17)<<8 | M(18)<<16 | (M(19)& 1)<<24;
            x[6] = (M(19)&~ 1)>>1 | M(20)<<7 | M(21)<<15 | (M(22)& 7)<<23;
            x[7] = (M(22)&~ 7)>>3 | M(23)<<5 | M(24)<<13 | (M(25)&15)<<21;
            x[8] = (M(25)&~15)>>4 | M(26)<<4 | M(27)<<12 | (M(28)&63)<<20;
            x[9] = (M(28)&~63)>>6 | M(29)<<2 | M(30)<<10 |  M(31)    <<18;
        }
        
        
        // Check if reduced-form input >= 2^255-19
        static inline bool isOverflow(const i25519 x)
        {
            return ((x[0] > (P26 - 19))
                    && ((x[1] & x[3] & x[5] & x[7] & x[9]) == P25)
                    && ((x[2] & x[4] & x[6] & x[8]) == P26))
            || (x[9] > P25);
        }
        
        
        /*
         * Convert from internal format to little-endian uint8 format.  The
         * number must be in a reduced form which is output by the following ops:
         *     unpack, mul, sqr
         *     set --  if input in range 0 .. P25
         * If you're unsure if the number is reduced, first multiply it by 1.
         */
        static void pack25519(const i25519 x, key25519 m)
        {
            int ld = 0, ud = 0;
            int64_t t;
            ld = (isOverflow(x) ? 1 : 0) - ((x[9] < 0) ? 1 : 0);
            ud = ld * -(P25+1);
            ld *= 19;
            t = ld + X(0) + (X(1) << 26);
            m[ 0] = (uint8_t) t; m[ 1] = (uint8_t) (t >> 8); m[ 2] = (uint8_t) (t >> 16); m[ 3] = (uint8_t) (t >> 24);
            t = (t >> 32) + (X(2) << 19);
            m[ 4] = (uint8_t) t; m[ 5] = (uint8_t) (t >> 8); m[ 6] = (uint8_t) (t >> 16); m[ 7] = (uint8_t) (t >> 24);
            t = (t >> 32) + (X(3) << 13);
            m[ 8] = (uint8_t) t; m[ 9] = (uint8_t) (t >> 8); m[10] = (uint8_t) (t >> 16); m[11] = (uint8_t) (t >> 24);
            t = (t >> 32) + (X(4) <<  6);
            m[12] = (uint8_t) t; m[13] = (uint8_t) (t >> 8); m[14] = (uint8_t) (t >> 16); m[15] = (uint8_t) (t >> 24);
            t = (t >> 32) + X(5) + (X(6) << 25);
            m[16] = (uint8_t) t; m[17] = (uint8_t) (t >> 8); m[18] = (uint8_t) (t >> 16); m[19] = (uint8_t) (t >> 24);
            t = (t >> 32) + (X(7) << 19);
            m[20] = (uint8_t) t; m[21] = (uint8_t) (t >> 8); m[22] = (uint8_t) (t >> 16); m[23] = (uint8_t) (t >> 24);
            t = (t >> 32) + (X(8) << 12);
            m[24] = (uint8_t) t; m[25] = (uint8_t) (t >> 8); m[26] = (uint8_t) (t >> 16); m[27] = (uint8_t) (t >> 24);
            t = (t >> 32) + ((X(9) + ud) << 6);
            m[28] = (uint8_t) t; m[29] = (uint8_t) (t >> 8); m[30] = (uint8_t) (t >> 16); m[31] = (uint8_t) (t >> 24);
        }
        
        // Copy a number
        static inline void cpy25519(i25519 out, const i25519 in)
        {
            for (int i = 0; i < 10; i++)
                out[i] = in[i];
        }
        
        // Set a number to value, which must be in range -185861411 .. 185861411
        static inline void set25519(i25519 out, const int in)
        {
            out[0] = in;
            for (int i = 1; i < 10; i++)
                out[i] = 0;
        }
        
        /*
         * Add/subtract two numbers.  The inputs must be in reduced form, and the
         * output isn't, so to do another addition or subtraction on the output,
         * first multiply it by one to reduce it.
         */
        static void add25519(i25519 xy, const i25519 x, const i25519 y)
        {
            xy[0] = x[0] + y[0];    xy[1] = x[1] + y[1];
            xy[2] = x[2] + y[2];    xy[3] = x[3] + y[3];
            xy[4] = x[4] + y[4];    xy[5] = x[5] + y[5];
            xy[6] = x[6] + y[6];    xy[7] = x[7] + y[7];
            xy[8] = x[8] + y[8];    xy[9] = x[9] + y[9];
        }
        
        static void sub25519(i25519 xy, const i25519 x, const i25519 y)
        {
            xy[0] = x[0] - y[0];    xy[1] = x[1] - y[1];
            xy[2] = x[2] - y[2];    xy[3] = x[3] - y[3];
            xy[4] = x[4] - y[4];    xy[5] = x[5] - y[5];
            xy[6] = x[6] - y[6];    xy[7] = x[7] - y[7];
            xy[8] = x[8] - y[8];    xy[9] = x[9] - y[9];
        }
        
        /*
         * Multiply a number by a small integer in range -185861411 .. 185861411.
         * The output is in reduced form, the input x need not be.  x and xy may point
         * to the same buffer.
         */
        static void mul25519small(i25519 xy, const i25519 x, const int y)
        {
            int64_t t;
            t = m64(x[8],y);
            xy[8] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[9],y);
            xy[9] = (int) (t & ((1 << 25) - 1));
            t = 19 * (t >> 25) + m64(x[0],y);
            xy[0] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[1],y);
            xy[1] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[2],y);
            xy[2] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[3],y);
            xy[3] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[4],y);
            xy[4] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[5],y);
            xy[5] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[6],y);
            xy[6] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[7],y);
            xy[7] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + xy[8];
            xy[8] = (int) (t & ((1 << 26) - 1));
            xy[9] += (int)(t >> 26);
        }
        
        /*
         * Multiply two numbers.  The output is in reduced form, the inputs need not
         * be.
         */
        static void mul25519(i25519 xy, const i25519 x, const i25519 y)
        {
            int64_t t;
            t = m64(x[0],y[8]) + m64(x[2],y[6]) + m64(x[4],y[4]) + m64(x[6],y[2]) +
            m64(x[8],y[0]) + 2 * (m64(x[1],y[7]) + m64(x[3],y[5]) +
                                  m64(x[5],y[3]) + m64(x[7],y[1])) + 38 *
            m64(x[9],y[9]);
            xy[8] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[0],y[9]) + m64(x[1],y[8]) + m64(x[2],y[7]) +
            m64(x[3],y[6]) + m64(x[4],y[5]) + m64(x[5],y[4]) +
            m64(x[6],y[3]) + m64(x[7],y[2]) + m64(x[8],y[1]) +
            m64(x[9],y[0]);
            xy[9] = (int) (t & ((1 << 25) - 1));
            t = m64(x[0],y[0]) + 19 * ((t >> 25) + m64(x[2],y[8]) + m64(x[4],y[6])
                                       + m64(x[6],y[4]) + m64(x[8],y[2])) + 38 *
            (m64(x[1],y[9]) + m64(x[3],y[7]) + m64(x[5],y[5]) +
             m64(x[7],y[3]) + m64(x[9],y[1]));
            xy[0] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[0],y[1]) + m64(x[1],y[0]) + 19 * (m64(x[2],y[9])
                                                                    + m64(x[3],y[8]) + m64(x[4],y[7]) + m64(x[5],y[6]) +
                                                                    m64(x[6],y[5]) + m64(x[7],y[4]) + m64(x[8],y[3]) +
                                                                    m64(x[9],y[2]));
            xy[1] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[0],y[2]) + m64(x[2],y[0]) + 19 * (m64(x[4],y[8])
                                                                    + m64(x[6],y[6]) + m64(x[8],y[4])) + 2 * m64(x[1],y[1])
            + 38 * (m64(x[3],y[9]) + m64(x[5],y[7]) +
                    m64(x[7],y[5]) + m64(x[9],y[3]));
            xy[2] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[0],y[3]) + m64(x[1],y[2]) + m64(x[2],y[1]) +
            m64(x[3],y[0]) + 19 * (m64(x[4],y[9]) + m64(x[5],y[8]) +
                                   m64(x[6],y[7]) + m64(x[7],y[6]) +
                                   m64(x[8],y[5]) + m64(x[9],y[4]));
            xy[3] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[0],y[4]) + m64(x[2],y[2]) + m64(x[4],y[0]) + 19 *
            (m64(x[6],y[8]) + m64(x[8],y[6])) + 2 * (m64(x[1],y[3]) +
                                                     m64(x[3],y[1])) + 38 *
            (m64(x[5],y[9]) + m64(x[7],y[7]) + m64(x[9],y[5]));
            xy[4] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[0],y[5]) + m64(x[1],y[4]) + m64(x[2],y[3]) +
            m64(x[3],y[2]) + m64(x[4],y[1]) + m64(x[5],y[0]) + 19 *
            (m64(x[6],y[9]) + m64(x[7],y[8]) + m64(x[8],y[7]) +
             m64(x[9],y[6]));
            xy[5] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[0],y[6]) + m64(x[2],y[4]) + m64(x[4],y[2]) +
            m64(x[6],y[0]) + 19 * m64(x[8],y[8]) + 2 * (m64(x[1],y[5]) +
                                                        m64(x[3],y[3]) + m64(x[5],y[1])) + 38 *
            (m64(x[7],y[9]) + m64(x[9],y[7]));
            xy[6] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + m64(x[0],y[7]) + m64(x[1],y[6]) + m64(x[2],y[5]) +
            m64(x[3],y[4]) + m64(x[4],y[3]) + m64(x[5],y[2]) +
            m64(x[6],y[1]) + m64(x[7],y[0]) + 19 * (m64(x[8],y[9]) +
                                                    m64(x[9],y[8]));
            xy[7] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + xy[8];
            xy[8] = (int) (t & ((1 << 26) - 1));
            xy[9] += (int)(t >> 26);
        }
        
        // Square a number.  Optimization of  mul25519(x2, x, x)
        static void sqr25519(i25519 x2, const i25519 x)
        {
            int64_t t;
            t = m64(x[4],x[4]) + 2 * (m64(x[0],x[8]) + m64(x[2],x[6])) + 38 *
            m64(x[9],x[9]) + 4 * (m64(x[1],x[7]) + m64(x[3],x[5]));
            x2[8] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + 2 * (m64(x[0],x[9]) + m64(x[1],x[8]) + m64(x[2],x[7]) +
                                 m64(x[3],x[6]) + m64(x[4],x[5]));
            x2[9] = (int) (t & ((1 << 25) - 1));
            t = 19 * (t >> 25) + m64(x[0],x[0]) + 38 * (m64(x[2],x[8]) +
                                                        m64(x[4],x[6]) + m64(x[5],x[5])) + 76 * (m64(x[1],x[9])
                                                                                                 + m64(x[3],x[7]));
            x2[0] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + 2 * m64(x[0],x[1]) + 38 * (m64(x[2],x[9]) +
                                                       m64(x[3],x[8]) + m64(x[4],x[7]) + m64(x[5],x[6]));
            x2[1] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + 19 * m64(x[6],x[6]) + 2 * (m64(x[0],x[2]) +
                                                       m64(x[1],x[1])) + 38 * m64(x[4],x[8]) + 76 *
            (m64(x[3],x[9]) + m64(x[5],x[7]));
            x2[2] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + 2 * (m64(x[0],x[3]) + m64(x[1],x[2])) + 38 *
            (m64(x[4],x[9]) + m64(x[5],x[8]) + m64(x[6],x[7]));
            x2[3] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + m64(x[2],x[2]) + 2 * m64(x[0],x[4]) + 38 *
            (m64(x[6],x[8]) + m64(x[7],x[7])) + 4 * m64(x[1],x[3]) + 76 *
            m64(x[5],x[9]);
            x2[4] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + 2 * (m64(x[0],x[5]) + m64(x[1],x[4]) + m64(x[2],x[3]))
            + 38 * (m64(x[6],x[9]) + m64(x[7],x[8]));
            x2[5] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + 19 * m64(x[8],x[8]) + 2 * (m64(x[0],x[6]) +
                                                       m64(x[2],x[4]) + m64(x[3],x[3])) + 4 * m64(x[1],x[5]) +
            76 * m64(x[7],x[9]);
            x2[6] = (int) (t & ((1 << 26) - 1));
            t = (t >> 26) + 2 * (m64(x[0],x[7]) + m64(x[1],x[6]) + m64(x[2],x[5]) +
                                 m64(x[3],x[4])) + 38 * m64(x[8],x[9]);
            x2[7] = (int) (t & ((1 << 25) - 1));
            t = (t >> 25) + x2[8];
            x2[8] = (int) (t & ((1 << 26) - 1));
            x2[9] += (int) (t >> 26);
        }
        
        /*
         * Calculates a reciprocal.  The output is in reduced form, the inputs need not
         * be.  Simply calculates  y = x^(p-2)  so it's not too fast.
         */
        static void recip25519(i25519 y, const i25519 x)
        {
            i25519 t0, t1, t2, t3, t4;
            int i;
            // the chain for x^(2^255-21) is straight from djb's implementation
            sqr25519(t1, x);        //  2 == 2 * 1
            sqr25519(t2, t1);       //  4 == 2 * 2
            sqr25519(t0, t2);       //  8 == 2 * 4
            mul25519(t2, t0, x);    //  9 == 8 + 1
            mul25519(t0, t2, t1);   // 11 == 9 + 2
            sqr25519(t1, t0);       // 22 == 2 * 11
            mul25519(t3, t1, t2);   // 31 == 22 + 9
            //    == 2^5 - 2^0
            sqr25519(t1, t3);       // 2^6   - 2^1
            sqr25519(t2, t1);       // 2^7   - 2^2
            sqr25519(t1, t2);       // 2^8   - 2^3
            sqr25519(t2, t1);       // 2^9   - 2^4
            sqr25519(t1, t2);       // 2^10  - 2^5
            mul25519(t2, t1, t3);   // 2^10  - 2^0
            sqr25519(t1, t2);       // 2^11  - 2^1
            sqr25519(t3, t1);       // 2^12  - 2^2
            for (i = 1; i < 5; i++)
            {
                sqr25519(t1, t3);
                sqr25519(t3, t1);
            }                       // 2^20  - 2^10
            mul25519(t1, t3, t2);   // 2^20  - 2^0
            sqr25519(t3, t1);       // 2^21  - 2^1
            sqr25519(t4, t3);       // 2^22  - 2^2
            for (i = 1; i < 10; i++)
            {
                sqr25519(t3, t4);
                sqr25519(t4, t3);
            }                       // 2^40  - 2^20
            mul25519(t3, t4, t1);   // 2^40  - 2^0
            for (i = 0; i < 5; i++)
            {
                sqr25519(t1, t3);
                sqr25519(t3, t1);
            }                       // 2^50  - 2^10
            mul25519(t1, t3, t2);   // 2^50  - 2^0
            sqr25519(t2, t1);       // 2^51  - 2^1
            sqr25519(t3, t2);       // 2^52  - 2^2
            for (i = 1; i < 25; i++)
            {
                sqr25519(t2, t3);
                sqr25519(t3, t2);
            }                       // 2^100 - 2^50
            mul25519(t2, t3, t1);   // 2^100 - 2^0
            sqr25519(t3, t2);       // 2^101 - 2^1
            sqr25519(t4, t3);       // 2^102 - 2^2
            for (i = 1; i < 50; i++)
            {
                sqr25519(t3, t4);
                sqr25519(t4, t3);
            }                       // 2^200 - 2^100
            mul25519(t3, t4, t2);   // 2^200 - 2^0
            for (i = 0; i < 25; i++)
            {
                sqr25519(t4, t3);
                sqr25519(t3, t4);
            }                       // 2^250 - 2^50
            mul25519(t2, t3, t1);   // 2^250 - 2^0
            sqr25519(t1, t2);       // 2^251 - 2^1
            sqr25519(t2, t1);       // 2^252 - 2^2
            sqr25519(t1, t2);       // 2^253 - 2^3
            sqr25519(t2, t1);       // 2^254 - 2^4
            sqr25519(t1, t2);       // 2^255 - 2^5
            mul25519(y, t1, t0);    // 2^255 - 21
        }
        
        /********************* Elliptic curve *********************/
        
        // y^2 = x^3 + 486662 x^2 + x  over GF(2^255-19)
        
        
        /*
         * t1 = ax + az
         * t2 = ax - az
         */
        static inline void mont_prep(i25519 t1, i25519 t2, i25519 ax, i25519 az)
        {
            add25519(t1, ax, az);
            sub25519(t2, ax, az);
        }
        
        /*
         * A = P + Q   where
         *  X(A) = ax/az
         *  X(P) = (t1+t2)/(t1-t2)
         *  X(Q) = (t3+t4)/(t3-t4)
         *  X(P-Q) = dx
         * clobbers t1 and t2, preserves t3 and t4
         */
        static inline void mont_add(i25519 t1, i25519 t2, i25519 t3, i25519 t4, i25519 ax, i25519 az, const i25519 dx)
        {
            mul25519(ax, t2, t3);
            mul25519(az, t1, t4);
            add25519(t1, ax, az);
            sub25519(t2, ax, az);
            sqr25519(ax, t1);
            sqr25519(t1, t2);
            mul25519(az, t1, dx);
        }
        
        /*
         * B = 2 * Q   where
         *  X(B) = bx/bz
         *  X(Q) = (t3+t4)/(t3-t4)
         * clobbers t1 and t2, preserves t3 and t4
         */
        static inline void mont_dbl(i25519 t1, i25519 t2, i25519 t3, i25519 t4, i25519 bx, i25519 bz)
        {
            sqr25519(t1, t3);
            sqr25519(t2, t4);
            mul25519(bx, t1, t2);
            sub25519(t2, t1, t2);
            mul25519small(bz, t2, 121665);
            add25519(t1, t1, bz);
            mul25519(bz, t1, t2);
        }
        
        static void curve25519(key25519 Px, const key25519 k, const key25519 Gx)
        {
            i25519 dx, x0, x1, z0, z1, t1, t2, t3, t4;
            
            // unpack the base
            if (Gx)
                unpack25519(dx, Gx);
            else
                set25519(dx, 9);
            
            // 0G = point-at-infinity
            set25519(x0, 1);
            set25519(z0, 0);
            
            // 1G = G
            cpy25519(x1, dx);
            set25519(z1, 1);
            
            for (int i = 31; i >= 0; i--)
            {
                for (int j = 7; j >= 0; j--)
                {
                    // swap arguments depending on bit
                    if (((k[i] >> j) & 1) == 1)
                    {
                        // a' = a + b
                        // b' = 2 b
                        mont_prep(t1, t2, x0, z0);
                        mont_prep(t3, t4, x1, z1);
                        mont_add(t1, t2, t3, t4, x0, z0, dx);
                        mont_dbl(t1, t2, t3, t4, x1, z1);
                    }
                    else
                    {
                        // a' = a + b
                        // b' = 2 b
                        mont_prep(t1, t2, x1, z1);
                        mont_prep(t3, t4, x0, z0);
                        mont_add(t1, t2, t3, t4, x1, z1, dx);
                        mont_dbl(t1, t2, t3, t4, x0, z0);
                    }
                }
            }
            
            recip25519(t1, z0);
            mul25519(dx, x0, t1);
            pack25519(dx, Px);
        }
        
        
        void secret25519(const key25519 k, const key25519 P, key25519 Z)
        {
            curve25519(Z, k, P);
        }
        
        void keygen25519(key25519 k, key25519 P)
        {
            k[31] &= 0x7F;
            k[31] |= 0x40;
            k[ 0] &= 0xF8;
            curve25519(P, k, 0);
        }
    };
};  // !namespace top

/////////////////////////////////////////////////////////////////////////////
