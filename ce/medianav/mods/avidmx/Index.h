//
// Index.h: declarations of classes for index management of mpeg-4 files.
//
// Geraint Davies, April 2004
//
// Copyright (c) GDCL 2004-6. All Rights Reserved. 
// You are free to re-use this as the basis for your own filter development,
// provided you retain this copyright notice in the source.
// http://www.gdcl.co.uk
//////////////////////////////////////////////////////////////////////

#pragma once

#include "demuxtypes.h"
#include <aviriff.h>

// -- all times in 100ns units
class AviTrackIndex: public TrackIndex
{
public:
    AviTrackIndex();

    bool Parse(const AVISTREAMHEADER& streamHeader, unsigned int streamIdx, const AVIOLDINDEX* pIndexArray, unsigned int offsetOfOffset);
    bool Parse(const AVISTREAMHEADER& streamHeader, unsigned int streamIdx, const AtomCache& pODMLIndex, const AtomReaderPtr& pRoot);
    DWORD Size(DWORD nSample) const;
    LONGLONG Offset(DWORD nSample) const;
    LONGLONG TotalFrames() const 
    {
        return m_totalFrames;
    }
    DWORD Next(DWORD nSample) const;

    DWORD SyncFor(DWORD nSample) const;
    DWORD NextSync(DWORD nSample) const;

    DWORD CTSToSample(LONGLONG nSample) const { return DTSToSample(nSample);}
    DWORD DTSToSample(LONGLONG tStart) const;
    LONGLONG SampleToCTS(DWORD nSample) const;
    LONGLONG Duration(DWORD nSample) const;

    bool HasCTSTable() const { return false; }

private:
    struct SampleRec
    {
        SampleRec(DWORD offset, DWORD size, DWORD framesPerSample, DWORD totalFramesSoFar, DWORD keyFrameSample)
            :offset(offset), size(size), framesPerSample(framesPerSample), totalFramesSoFar(totalFramesSoFar), keyFrameSample(keyFrameSample)
        {
        }
        DWORD offset;
        DWORD size;
        DWORD framesPerSample;
        DWORD totalFramesSoFar;
        DWORD keyFrameSample;
    };
    typedef std::vector<SampleRec> SamplesArray;

    SamplesArray m_samplesArray;
    DWORD m_totalFrames;
    DWORD m_start;        // Offset of first sample
    bool m_oneFramePerSample;
};

