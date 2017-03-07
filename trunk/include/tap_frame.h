/*
* ipop-project
* Copyright 2016, University of Florida
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/
#ifndef TINCAN_TAP_FRAME_H_
#define TINCAN_TAP_FRAME_H_
#include "tincan_base.h"
#include "async_io.h"
namespace tincan
{
/*
The TapFrameBuffer (TFB) is the byte container for a tincan frame's data. This
includes the tincan specific headers as well as the payload data received from
the TAP device or tincan link. A TFB facilitates
decoupling of the raw data from the meta-data required to manage it.
*/
//class TapFrameBuffer
//{
//  friend class TapFrame;
//  friend class TapFrameCache;
//  friend class TapFrameProperties;
//  TapFrameBuffer() = default;
//  ~TapFrameBuffer() = default;
//  uint8_t fb_[kTapBufferSize];
//};

using TapFrameBuffer = array<uint8_t, kTapBufferSize>;
/*
A TapFrame encapsulates a TapFrameBuffer and defines the control and access
semantics for that type. A single TapFrame can own and manage multiple TFBs
over its life time. It provides easy access to commonly used offsets and
indexing capabilty throught bytes [0..Capacity).
There are no copy semantics but move semantics are provided to support single
ownership of the TBF.
*/
class  TapFrame : virtual public AsyncIo
{
  friend class TapFrameCache;
  friend class TapFrameProperties;
public:
  TapFrame();

  TapFrame(const TapFrame & rhs);

  TapFrame(TapFrame && rhs);

  //Copies the specifed amount of data into the TFB.
  TapFrame(uint8_t* data, uint32_t len);

  virtual ~TapFrame();

  //Copies the TFB from the rhs frame to the lhs
  TapFrame &operator= (TapFrame & rhs);
  //Moves the TFB from the rhs frame to the lhs
  TapFrame &operator= (TapFrame && rhs);

  //Compares based on the pointer address of the TFB
  bool operator==(const TapFrame & rhs) const;

  bool operator!=(const TapFrame & rhs) const;

  bool operator !() const;

  //Used to byte address into the payload
  uint8_t & operator[](uint32_t index);

  const uint8_t & operator[](uint32_t const index) const;

  TapFrame & Initialize();
  
  TapFrame & Initialize(
    uint8_t * buffer_to_transfer,
    uint32_t bytes_to_transfer,
    AIO_OP flags = AIO_READ,
    uint32_t bytes_transferred = 0);

  void Header(uint16_t val);
  //First valid byte in tbf
  uint8_t * Begin();
  //after last valid byte in tbf
  uint8_t * End();
  //length of header + payload
  uint32_t Length();
  //Capacity is the maximum size ie., of the raw buffer 
  uint32_t Capacity() const;
 
  uint8_t * Payload();

  uint8_t * PayloadEnd();

  uint32_t PayloadLength();

  void PayloadLength(uint32_t length);

  uint32_t PayloadCapacity();

  void Dump(const string & label);
 protected:
   TapFrameBuffer * tfb_;
   uint32_t pl_len_;
};

///////////////////////////////////////////////////////////////////////////////
//IccMessage
class IccMessage :
  public TapFrame
{
public:
  void Message(
    uint8_t * in_buf,
    uint32_t buf_len);

};
///////////////////////////////////////////////////////////////////////////////
class DtfMessage :
  public TapFrame
{
public:
  void Message(
    uint8_t * in_buf,
    uint32_t buf_len);
};
///////////////////////////////////////////////////////////////////////////////
class FwdMessage :
  public TapFrame
{
public:
  void Message(
    uint8_t * in_buf,
    uint32_t buf_len);
};
///////////////////////////////////////////////////////////////////////////////
//TapFrameProperties is a ready only accessor class for querying compound
//properties of the TFB.
class TapFrameProperties
{
public:
  TapFrameProperties(TapFrame & tf) :
    tf_(tf)/*,
    eth_(tf.Payload()),
    ipp_(eth_.Payload())*/
  {}

  bool IsIccMsg() const
  {
    return memcmp(tf_.Begin(), &kIccMagic,
      kTapHeaderSize) == 0;
  }
  bool IsFwdMsg() const
  {
    return memcmp(tf_.Begin(), &kFwdMagic,
      kTapHeaderSize) == 0;
  }
  bool IsDtfMsg() const
  {
    return memcmp(tf_.Begin(), &kDtfMagic,
      kTapHeaderSize) == 0;
  }
  bool IsIp4()
  {
    EthOffsets eth = tf_.Payload();
    IpOffsets ipp = eth.Payload();
    return *(uint16_t*)eth.Type() == 0x0008 && *ipp.Version() >> 4 == 4;
  }

  bool IsIp6()
  {
    EthOffsets eth = tf_.Payload();
    IpOffsets ipp = eth.Payload();
    return *(uint16_t*)eth.Type() == 0x0008 && *ipp.Version() >> 4 == 6;
  }

  bool IsArpRequest()
  {
    EthOffsets eth = tf_.Payload();
    return *(uint16_t*)eth.Type() == 0x0608 && (*(eth.Payload()+7) == 0x01);
  }

  bool IsArpResponse()
  {
    EthOffsets eth = tf_.Payload();
    return *(uint16_t*)eth.Type() == 0x0608 && (*(eth.Payload() + 7) == 0x02);
  }

  bool IsEthernetBroadcast()
  {
    EthOffsets eth = tf_.Payload();
    return memcmp(eth.DestinationMac(), "\xFF\xFF\xFF\xFF\xFF\xFF", 6) == 0;
  }

  uint32_t DestinationIp4Address()
  {
    EthOffsets eth = tf_.Payload();
    IpOffsets ipp = eth.Payload();
    return *(uint32_t*)ipp.DestinationIp();
  }

  MacAddressType & DestinationMac()
  {
    EthOffsets eth = tf_.Payload();
    return *(MacAddressType *)(eth.DestinationMac());
  }

private:
  TapFrame & tf_;
 // EthOffsets eth_;
  //IpOffsets ipp_;
};
///////////////////////////////////////////////////////////////////////////////

//class TapArp4
//{
//public:
//  TapArp4(uint8_t * eth_data)
//  {
//    buf_ = eth_data;
//    //if((*(uint16_t*)&eth_header[12]) == 0x0608){
//    //}
//  }
//  void IsRquest(){}
//
//  void IsReply()
//  {}
//
//  uint32_t DestinationIp()
//  {
//      return *(uint32_t*)(&buf_[24]);
//  }
//
//  MacAddressType & DestinationMac()
//  {
//    return *(MacAddressType*)(&buf_[18]);
//  }
//  
//  uint32_t SourceIp()
//  {
//    return *(uint32_t*)(&buf_[14]);
//  }
//
//  MacAddressType & SourceMac()
//  {
//    return *(MacAddressType*)(&buf_[8]);
//  }
//private:
//  uint8_t * buf_;
//};
} //namespace tincan
#endif  // TINCAN_TAP_FRAME_H_