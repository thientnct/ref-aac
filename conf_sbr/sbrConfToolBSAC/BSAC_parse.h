#ifdef BSAC

#include "config.h"
#include "ct_sbrdecoder.h"
#include "ct_envextr.h"
#include "ct_sbrdec.h"
#include "ct_envdec.h"
#include "ct_sbrcrc.h"

SBR_ERROR readSBR_BSAC (SBRDECODER self, SBRBITSTREAM *stream, int nChannels);

#endif