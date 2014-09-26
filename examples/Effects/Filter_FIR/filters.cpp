#include "filters.h"
short low_pass[NUM_COEFFS] = {
#include "lopass_1000_44100.h"
};

short band_pass[NUM_COEFFS] = {
#include "bandp_1200_1700.h"
};
