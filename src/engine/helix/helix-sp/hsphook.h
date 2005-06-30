/* **********
 *
 * This software is released under the provisions of the GPL version 2.
 * see file "COPYING".  If that file is not available, the full statement 
 * of the license can be found at
 *
 * http://www.fsf.org/licensing/licenses/gpl.txt
 *
 * Copyright (c) Paul Cifarelli 2005
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 * PCM time-domain equalizer:
 *    (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
 *    (c) 2004 Mark Kretschmann <markey@web.de>
 *
 * ********** */
#ifndef _HSPHOOK_H_INCLUDED_
#define _HSPHOOK_H_INCLUDED_

class HSPPreMixAudioHook : public IHXAudioHook
{
public:
   HSPPreMixAudioHook(HelixSimplePlayer *player, int playerIndex, IHXAudioStream *pAudioStream);
   virtual ~HSPPreMixAudioHook();
   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                               void** ppvObj);
   STDMETHOD_(ULONG32,AddRef)  (THIS);
   STDMETHOD_(ULONG32,Release) (THIS);
   /*
    * IHXAudioHook methods
    */
   STDMETHOD(OnBuffer) (THIS_
                        HXAudioData *pAudioInData,
                        HXAudioData *pAudioOutData);
   STDMETHOD(OnInit) (THIS_
                      HXAudioFormat *pFormat);


private:
   HSPPreMixAudioHook();

   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;
   int                m_index;
   IHXAudioStream    *m_stream;
   HXAudioFormat      m_format;
   int                m_count;
};



#define BAND_NUM 10
#define EQ_MAX_BANDS 10
#define EQ_CHANNELS 7   // anyone try this?  should work, but I only have a stereo card...

// Floating point
typedef struct
{
   float beta;
   float alpha;
   float gamma;
} sIIRCoefficients;

/* Coefficient history for the IIR filter */
typedef struct
{
   float x[3]; /* x[n], x[n-1], x[n-2] */
   float y[3]; /* y[n], y[n-1], y[n-2] */
} sXYData;

#include "iir_cf.h"         // IIR filter coefficients

class HSPPostMixAudioHook : public IHXAudioHook
{
public:
   HSPPostMixAudioHook(HelixSimplePlayer *player, int playerIndex);
   virtual ~HSPPostMixAudioHook();
   /*
    *  IUnknown methods
    */
   STDMETHOD(QueryInterface)   (THIS_
                               REFIID riid,
                               void** ppvObj);
   STDMETHOD_(ULONG32,AddRef)  (THIS);
   STDMETHOD_(ULONG32,Release) (THIS);
   /*
    * IHXAudioHook methods
    */
   STDMETHOD(OnBuffer) (THIS_
                        HXAudioData *pAudioInData,
                        HXAudioData *pAudioOutData);
   STDMETHOD(OnInit) (THIS_
                      HXAudioFormat *pFormat);

   void updateEQgains(int preamp, vector<int> &equalizerGains);

private:
   HSPPostMixAudioHook();

   void scopeify(unsigned long time, unsigned char *data, size_t len);
   void equalize(unsigned char *datain, unsigned char *dataout, size_t len);

   HelixSimplePlayer *m_Player;
   LONG32             m_lRefCount;
   int                m_index;
   HXAudioFormat      m_format;
   int                m_count;

   // equalizer

   // Gain for each band
   // values should be between -0.2 and 1.0
   float gain[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
   // Volume gain
   // values should be between 0.0 and 1.0
   float preamp[EQ_CHANNELS] __attribute__((aligned));
   // Coefficients
   sIIRCoefficients* iir_cf;
   sXYData data_history[EQ_MAX_BANDS][EQ_CHANNELS] __attribute__((aligned));
   // history indices
   int                m_i;
   int                m_j;
   int                m_k;

#ifdef TEST_APP
   int                buf[MAX_SCOPE_SAMPLES];
#endif
};


#endif
