/*
 * Copyright (c) 20xx ??
 * Copyright (c) 2013 Riccardo Lucchese, riccardo.lucchese at gmail.com
 * 
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include <assert.h>

void UYVY2BGR(unsigned char *buffer, int len, unsigned char *out) {
	assert(buffer);
	assert(out);
	assert(!(len & 0x3));

	/* nb len è la lunghezza del buffer uyuv*/
	int j,R,G,B,U,Y1,Y2,V;
	for (j = 0; j < len/4 ; j++) {
		/* Every 4 bytes: UYUV */
		U =  *(buffer + j*4)  ;
		Y1 = *(buffer + j*4 + 1);
		V =  *(buffer + j*4 + 2);
		Y2 = *(buffer + j*4 + 3);

		/* Convert YUV to RGB */
		B = (( 298*(Y1-16)               + 409*(V-128) + 128) >> 8);
		G = (( 298*(Y1-16) - 100*(U-128) - 208*(V-128) + 128) >> 8);
		R = (( 298*(Y1-16) + 516*(U-128)               + 128) >> 8) ;

		if (R < 0)
			R=0;
		else if (R > 255)
			R = 255;
		if (G < 0)
			G=0;
		else if (G > 255)
			G=255;
		if (B < 0)
			B=0;
		else if (B > 255)
			B=255;

		*(out + j*6 ) = R;
		*(out + j*6 + 1) = G;
		*(out + j*6 + 2) = B;

		B = (( 298*(Y2-16)               + 409*(V-128) + 128) >> 8);
		G = (( 298*(Y2-16) - 100*(U-128) - 208*(V-128) + 128) >> 8);
		R = (( 298*(Y2-16) + 516*(U-128)               + 128) >> 8) ;

		if (R < 0)
			R=0;
		else if (R > 255)
			R=255;
		if (G < 0)
			G=0;
		else if (G > 255)
			G=255;
		if (B < 0)
			B=0;
		else if (B > 255)
			B=255;

		*(out+ j*6 + 3) = R;
		*(out+ j*6 + 4) = G;
		*(out+ j*6 + 5) = B;
	}
}

