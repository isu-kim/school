//
// @file : utils.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements all utility functions.
//


#include "utils.h"


/**
 * A simple function that converts little endian to original byte order.
 * @deprecated Since Ubuntu is by default little endian.
 * @param org The little endian value to convert.
 * @return The converted byte order from little endian.
 */
unsigned int convert_endian(const unsigned char* arr) {
    unsigned int tmp[4];
    tmp[0] = (arr[0] & 0xFF) << 12;
    tmp[1] = ((arr[1] >> 4) & 0xFF) << 8;
    tmp[2] = ((arr[2] >> 8) & 0xFF) << 4;
    tmp[3] = ((arr[3] >> 12) & 0xFF);

    return tmp[0] | tmp[1] | tmp[2] | tmp[3];
}