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


#include "stdafx.h"
#include "Index.h"

// sample count and sizes ------------------------------------------------
/* Macro to make a numeric part of ckid for a chunk out of a stream number
** from 0-255.
*/
#define ToHex(n)	((BYTE) (((n) > 9) ? ((n) - 10 + 'A') : ((n) + '0')))
#define MAKEAVICKIDSTREAM(stream) MAKEWORD(ToHex(((stream) & 0xf0) >> 4), ToHex((stream) & 0x0f))

AviTrackIndex::AviTrackIndex()
: TrackIndex(),
  m_totalFrames(0),
  m_start(0),
  m_oneFramePerSample(false)
{}
     
bool 
AviTrackIndex::Parse(const AVISTREAMHEADER& streamHeader, unsigned int streamIdx, const AVIOLDINDEX* pIndexArray, unsigned int offsetOfOffset)
{
    unsigned int indexLength = pIndexArray->cb / sizeof(struct AVIOLDINDEX::_avioldindex_entry);
    if(streamHeader.dwSampleSize == 0)
    {
        // One chunk - one sample
        m_samplesArray.reserve(streamHeader.dwLength);
    }else
    {
        // One chunk - many samples, so index size/2 is a good approximation
        m_samplesArray.reserve(indexLength / 2);
    }
    
    // All samples are keys in auds streams.
    bool allSamplesAreKeys = streamHeader.fccType == streamtypeAUDIO;
    m_nMaxSize = 0;
    m_totalFrames = 0;

    WORD indexWord = MAKEAVICKIDSTREAM(streamIdx);
    debugPrintf(DBG, L"AviTrackIndex::Parse: indexLength = %u, indexWord = %hu\r\n", indexLength, indexWord);
    DWORD keySamplesCount = 0;
    DWORD curKeyFrameSample = 0;
    DWORD nCurSample = 0;
    for(unsigned int i = 0; i < indexLength; ++i)
    {
        if(static_cast<WORD>(pIndexArray->aIndex[i].dwChunkId) != indexWord)
            continue;
        DWORD offset = pIndexArray->aIndex[i].dwOffset + sizeof(RIFFCHUNK) + offsetOfOffset;
        DWORD size = pIndexArray->aIndex[i].dwSize;
        bool keyFrame = (pIndexArray->aIndex[i].dwFlags & AVIIF_KEYFRAME) != 0 || allSamplesAreKeys;
        if(keyFrame)
        {
            ++keySamplesCount;
            curKeyFrameSample = nCurSample;
        }
        if(m_nMaxSize < size)
            m_nMaxSize = size;
        DWORD framesPerSample = streamHeader.dwSampleSize == 0 ? 1 : (size / streamHeader.dwSampleSize);
        m_samplesArray.push_back(SampleRec(offset, size, framesPerSample, m_totalFrames, curKeyFrameSample));
        m_totalFrames += framesPerSample;
        ++nCurSample;
    }

    m_nSamples = static_cast<DWORD>(m_samplesArray.size());

    m_scale = streamHeader.dwScale;
    m_rate = streamHeader.dwRate;
    m_start = streamHeader.dwStart;
    m_total = TrackToReftime(m_totalFrames);
    m_oneFramePerSample = streamHeader.dwSampleSize == 0;

    debugPrintf(DBG, L"AviTrackIndex::Parse: \r\n\t m_nSamples = %u\r\n\t keySamplesCount=%u\r\n\t m_totalFrames=%u\r\n\t m_scale=%u\r\n\t m_rate=%u\r\n\t m_start=%u\r\n\t m_total=%I64u\r\n\t streamHeader.dwSampleSize=%u\r\n", m_nSamples, keySamplesCount, m_totalFrames, m_scale, m_rate, m_start, m_total, streamHeader.dwSampleSize);
    return true;
}

bool 
AviTrackIndex::Parse(const AVISTREAMHEADER& streamHeader, unsigned int streamIdx, const AtomCache& pODMLIndex, const AtomReaderPtr& pRoot)
{
    class IndexAtom : public Atom
    {
    public:
        IndexAtom(AtomReader* pReader, LONGLONG llOffset, LONGLONG llLength)
            :Atom(pReader, llOffset, llLength, ckidAVISUPERINDEX, sizeof(RIFFCHUNK), false)
        {
        }

        virtual void ScanChildrenAt(LONGLONG llOffset)
        {
        }

    };
    
    std::vector<AtomCache> idxs;
    const AVISUPERINDEX& superIndex = *reinterpret_cast<const AVISUPERINDEX *>(pODMLIndex.getRawBuffer());
    if(Valid_SUPERINDEX(&superIndex))
    {
        for(DWORD i = 0; i < superIndex.nEntriesInUse; ++i)
        {
            AtomCache pStdIdx(AtomPtr(new IndexAtom(pRoot.get(), superIndex.aIndex[i].qwOffset, superIndex.aIndex[i].dwSize)));
            const AVISTDINDEX& stdIndex = *reinterpret_cast<const AVISTDINDEX *>(pStdIdx.getRawBuffer());
            // Check whatever it is frames index.
            if(stdIndex.bIndexType != AVI_INDEX_OF_CHUNKS || stdIndex.bIndexSubType != 0)
                continue;
            idxs.push_back(pStdIdx);
        }
    }else
    {
        const AVISTDINDEX& stdIndex = *reinterpret_cast<const AVISTDINDEX *>(pODMLIndex.getRawBuffer());
        // Check whatever it is frames index.
        if(stdIndex.bIndexType != AVI_INDEX_OF_CHUNKS || stdIndex.bIndexSubType != 0)
        {
            debugPrintf(DBG, L"AviTrackIndex::Parse:OpenDML: Bad index!\r\n");
            return false;
        }
        idxs.push_back(pODMLIndex);
    }

    unsigned int indexLength = 0;
    for(DWORD i = 0; i < idxs.size(); ++i)
    {
        const AVISTDINDEX& stdIndex = *reinterpret_cast<const AVISTDINDEX *>(idxs[i].getRawBuffer());
        indexLength += stdIndex.nEntriesInUse;
    }

    if(streamHeader.dwSampleSize == 0)
    {
        // One chunk - one sample
        m_samplesArray.reserve(streamHeader.dwLength);
    }else
    {
        // One chunk - many samples, so index size/2 is a good approximation
        m_samplesArray.reserve(indexLength / 2);
    }

    // All samples are keys in auds streams.
    bool allSamplesAreKeys = streamHeader.fccType == streamtypeAUDIO;
    m_nMaxSize = 0;
    m_totalFrames = 0;

    debugPrintf(DBG, L"AviTrackIndex::Parse:OpenDML: indexLength = %u\r\n", indexLength);
    DWORD keySamplesCount = 0;
    DWORD curKeyFrameSample = 0;
    DWORD nCurSample = 0;
    for(DWORD idx = 0, i = 0; idx < idxs.size(); ++idx)
    {
        const AVISTDINDEX& stdIndex = *reinterpret_cast<const AVISTDINDEX *>(idxs[idx].getRawBuffer());
        for(DWORD u = 0; u < stdIndex.nEntriesInUse && i < indexLength; ++i, ++u)
        {
            DWORD offset = static_cast<DWORD>(stdIndex.qwBaseOffset + stdIndex.aIndex[u].dwOffset);
            DWORD size = stdIndex.aIndex[u].dwSize & AVISTDINDEX_SIZEMASK;
            bool keyFrame = (stdIndex.aIndex[u].dwSize & AVISTDINDEX_DELTAFRAME) == 0 || allSamplesAreKeys;
            if(keyFrame)
            {
                ++keySamplesCount;
                curKeyFrameSample = nCurSample;
            }
            if(m_nMaxSize < size)
                m_nMaxSize = size;
            DWORD framesPerSample = streamHeader.dwSampleSize == 0 ? 1 : (size / streamHeader.dwSampleSize);
            m_samplesArray.push_back(SampleRec(offset, size, framesPerSample, m_totalFrames, curKeyFrameSample));
            m_totalFrames += framesPerSample;
            ++nCurSample;
        }
    }

    m_nSamples = static_cast<DWORD>(m_samplesArray.size());

    m_scale = streamHeader.dwScale;
    m_rate = streamHeader.dwRate;
    m_start = streamHeader.dwStart;
    m_total = TrackToReftime(m_totalFrames);
    m_oneFramePerSample = streamHeader.dwSampleSize == 0;

    debugPrintf(DBG, L"AviTrackIndex::Parse:OpenDML: \r\n\t m_nSamples = %u\r\n\t keySamplesCount=%u\r\n\t m_totalFrames=%u\r\n\t m_scale=%u\r\n\t m_rate=%u\r\n\t m_start=%u\r\n\t m_total=%I64u\r\n\t streamHeader.dwSampleSize=%u\r\n", m_nSamples, keySamplesCount, m_totalFrames, m_scale, m_rate, m_start, m_total, streamHeader.dwSampleSize);
    return true;
}

DWORD 
AviTrackIndex::Size(DWORD nSample) const
{
    if(nSample >= m_nSamples)
        nSample = m_nSamples - 1;

    return m_samplesArray[nSample].size;
}
                 
LONGLONG 
AviTrackIndex::Offset(DWORD nSample) const
{
    if(nSample >= m_nSamples)
        nSample = m_nSamples - 1;

    return m_samplesArray[nSample].offset;
}

DWORD AviTrackIndex::Next(DWORD nSample) const
{
    return nSample + 1;
}

DWORD 
AviTrackIndex::SyncFor(DWORD nSample) const
{
    if(nSample >= m_nSamples)
        nSample = m_nSamples - 1;

    return m_samplesArray[nSample].keyFrameSample;
}

DWORD 
AviTrackIndex::NextSync(DWORD nSample) const
{
    for(;nSample < m_nSamples; ++nSample)
    {
        if(m_samplesArray[nSample].keyFrameSample == nSample)
            return nSample;
    }
    return m_nSamples - 1; // Or 0?
}

DWORD 
AviTrackIndex::DTSToSample(LONGLONG tStart) const
{
    DWORD frame = static_cast<DWORD>(ReftimeToTrack(tStart) - m_start);
    if(m_oneFramePerSample)
        return frame;
    DWORD nSample = 0;
    while(nSample < m_nSamples && (m_samplesArray[nSample].totalFramesSoFar + m_samplesArray[nSample].framesPerSample) <= frame)
    {
        ++nSample;
    }
    return nSample;
}

LONGLONG 
AviTrackIndex::SampleToCTS(DWORD nSample) const
{
    if(nSample >= m_nSamples)
        nSample = m_nSamples - 1;

    DWORD frame = 0;
    if(!m_oneFramePerSample)
    {
        frame = m_samplesArray[nSample].totalFramesSoFar;
    }else
    {
        frame = nSample;
    }
    return TrackToReftime(m_start + frame);
}

LONGLONG 
AviTrackIndex::Duration(DWORD nSample) const
{
    if(nSample >= m_nSamples)
        nSample = m_nSamples - 1;

    if(!m_oneFramePerSample)
        return TrackToReftime(m_samplesArray[nSample].framesPerSample);
    else
        return TrackToReftime(1);
}
