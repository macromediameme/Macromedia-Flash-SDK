// This file has been rearranged from the code posted
// on news:forums.macromedia.com by Jonathan Gay.
// Courtesy of Macromedia
//
// The original spec contains no information about Compressed sound data.
//
// Here is a brief outline of how the ADPCM samples are encoded: 
// The first two bits of the ADPCM data specify whether the ADPCM
// samples are 2,3,4, or 5 bit.   (so nBits = GetBits(2)+2)
//
// The ADPCM data is broken up into blocks of 4K samples.  At the
// start of each block is a 16-bit value (the initial sample), and
// a 6-bit value which is the initial index into the stepSizeTable.
//
// Stereo samples are interleaved (LRLRLR), as are block headers.
// So the data looks something like this:
//
//  Left  Header (16-bit inital sample, 6-bit initial index)
//  Right Header (16-bit inital sample, 6-bit initial index)
//
//  L[nBits]R[nBits]L[nBits]R[nBits]L[nBits]R[nBits]....
//  ...thru to a total of 4K samples (actually 8K because its stereo).
//
// So total data size is: 8K * nBits.


//
// If you are interested in decoding (decompressing) ADPCM sound data,
// take a look at the source code for Oliver Debon's Linux plugin.
//


//
// ADPCM tables
//

static const int piIndexTable2[2] =
{
    -1, 2,
};

static const int piIndexTable3[4] =
{
    -1, -1, 2, 4,
};

static const int piIndexTable4[8] =
{
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int piIndexTable5[16] =
{
 -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16,
};

static const int* ppiIndexTables[] =
{
    piIndexTable2,
    piIndexTable3,
    piIndexTable4,
    piIndexTable5
};

static const int piStepSizeTable[89] =
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

void CExportSwf::PutDefineSound(CSwfSound* pSound)
{
    PutTag(stagDefineSound);
    PutWord(pSound->m_iID);

    WAVEFORMATEX* pwfFormat = pSound->m_pwfFormat;

    PutBits(pSound->m_fCompressed ? 1 : 0, 4);  // 0=uncompressed 1=ADPCM compressed

    // Find nearest sample rate (5.5K, 11K, 22K or 44K)

    int iSampRate = pwfFormat->nSamplesPerSec;
    int iSampRates[] = {5500, 11025, 22050, 44100};

    int iSampIndex = -1;
    int iSampDiff = 99999;

    for (int i=0; i<4; i++)
    {
        int iDiff = abs(iSampRate - iSampRates[i]);
        if (iDiff < iSampDiff)
        {
            iSampDiff = iDiff;
            iSampIndex = i; 
        }
    }


    // Write sample rate
    PutBits(iSampIndex, 2);

    // Bits/sample (0 = 8bit, 1 = 16 bit)
    PutBits(pSound->m_fCompressed || pwfFormat->wBitsPerSample == 16 ? 1 : 0, 1);

    // Mono or Stereo? (0 = mono, 1 = stereo)
    PutBits(pwfFormat->nChannels == 2 ? 1 : 0, 1);

    // Sample count
    PutDWord(pSound->m_iSampleCount);

    if (pSound->m_fCompressed)
    {
        // Write compressed data
        PutADPCMData(pSound);
    }
    else
    {
        // Copy uncompressed sample data to filebuffer
        memcpy(m_fileBuf + m_filePos, pSound->m_pbRawSamples, pSound->m_iSamplesLength);
        m_filePos += pSound->m_iSamplesLength;
    }

    PutTagLen();
}

void CExportSwf::PutADPCMData(CSwfSound* pSound)
{
    pSound->Convert8to16bit();

    int         nBits = 4;                                  // number of bits per ADPCM sample
    int         iSignBit = 1 << (nBits-1);                  // Sign bit mask
    const int*  piIndexTable = ppiIndexTables[nBits-2];     // Select index table to use

    // Write number of bits per ADPCM sample
    PutBits(nBits-2, 2);

    int iValPred[2] = {0, 0};   // Predicted output value(s)
    int iIndex[2]   = {0, 0};   // Indeces int StepSizeTable
    int iStep[2]    = {0, 0};   // Step size
    
    int     iSampleCount = pSound->m_iSampleCount;              // Number of samples.
    int     iChannelCount = pSound->m_pwfFormat->nChannels;     // Number of channels (mono, stereo)

    short*  psSample = pSound->m_ps16bitSamples;                // Pointer to start of 16-bit/sample data

    for (int i=0; i < iSampleCount; i++)
    {
        if ((i & 0xfff) == 0)
        {
            for (int c=0; c<iChannelCount; c++)
            {
                // First sample in the block, so no delta
                short sSample = *psSample++;

                // Write full 16-bit sample
                PutBits((int)sSample, 16);
                iValPred[c] = sSample;

                // Calculate initial index & step
                int iDiff = abs(*psSample - sSample);
                for (iIndex[c]=0; iIndex[c] < 88; iIndex[c]++)
                    if (iDiff <= piStepSizeTable[iIndex[c]])
                        break;

                if (iIndex[c] > 63)
                    iIndex[c] = 63;

                iStep[c] = piStepSizeTable[iIndex[c]];

                // Write initial index into StepSizeTable
                PutBits((int)iIndex[c], 6);
            }
        }
        else
        {
            for (int c=0; c<iChannelCount; c++)
            {
                short sSample = *psSample++;

                // Step 1 - compute difference with previous value
                int iDiff = sSample - iValPred[c];

                // Make iDiff absolute value
                int iSign = (iDiff < 0) ? iSignBit : 0;
                if (iSign)
                    iDiff = (-iDiff);

                // Step 2 - Divide and clamp
                // Note:
                // This code *approximately* computes:
                //    iDelta = iDiff*4/iStep;
                //    iVPDiff = (iDelta+0.5)*iStep/4;
                // but in shift step bits are dropped. The net result of this is
                // that even if you have fast mul/div hardware you cannot put it to
                // good use since the fixup would be too expensive.

                int iDelta  = 0;
                int iVPDiff = (iStep[c] >> (nBits-1));

                for (int k = nBits-2; k >= 0; k--, iStep[c] >>= 1)
                {
                    if (iDiff >= iStep[c])
                    {
                        iDelta |= (1<<k);
                        iVPDiff += iStep[c];
                        if (k > 0)
                            iDiff -= iStep[c];
                    }
                }

                // Step 3 - Update previous value
                if (iSign)
                  iValPred[c] -= iVPDiff;
                else
                  iValPred[c] += iVPDiff;

                // Step 4 - Clamp previous value to 16 bits
                if (iValPred[c] > 32767)
                  iValPred[c] = 32767;
                else if (iValPred[c] < -32768)
                  iValPred[c] = -32768;

                // Step 5 - Assemble value, update index and step values
                iIndex[c] += piIndexTable[iDelta];
                iDelta |= iSign;                    // Or with iSign *after* indexing

                // Clamp StepSizeTable index
                if (iIndex[c] < 0)      iIndex[c] = 0;
                if (iIndex[c] > 88) iIndex[c] = 88;
                iStep[c] = piStepSizeTable[iIndex[c]];

                // Step 6 - Output value
                PutBits(iDelta, nBits);
            }
        }
    }
}

