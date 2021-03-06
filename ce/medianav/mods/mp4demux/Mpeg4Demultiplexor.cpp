//
// Mpeg4Demultiplexor.cpp
// 
// Implementation of classes for DirectShow Mpeg-4 Demultiplexor filter
//
// Geraint Davies, April 2004
//
// Copyright (c) GDCL 2004-6. All Rights Reserved. 
// You are free to re-use this as the basis for your own filter development,
// provided you retain this copyright notice in the source.
// http://www.gdcl.co.uk
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Mpeg4Demultiplexor.h"

// the class factory calls this to create the filter
//static 
CUnknown* WINAPI 
Mpeg4Demultiplexor::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
    return new Mpeg4Demultiplexor(pUnk, phr);
}

Mpeg4Demultiplexor::Mpeg4Demultiplexor(LPUNKNOWN pUnk, HRESULT* phr)
:DShowDemultiplexor(pUnk, phr, __uuidof(Mpeg4Demultiplexor))
{
    setAlwaysSeekToKeyFrame(Utils::RegistryAccessor::getBool(HKEY_LOCAL_MACHINE, TEXT("\\SOFTWARE\\Microsoft\\DirectX\\DirectShow\\MP4Demux"), TEXT("KeyFrameSeeking"), true));
}

MoviePtr Mpeg4Demultiplexor::createMovie(const AtomReaderPtr& pRoot)
{
    return MoviePtr(new Mpeg4Movie(pRoot));
}
