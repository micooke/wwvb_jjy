#ifndef wwvb_jjy_structs_h
#define wwvb_jjy_structs_h

#ifndef WWVB_TIMECODE
#define WWVB_TIMECODE 1
#endif

#include <stdint.h>

// enforce 1 byte bit packing for the structs, otherwise the
// struct -> union alignment will fail
#pragma pack(push, 1)

#if (WWVB_TIMECODE==1)
struct timecode_frame{
  //-----------------
  //b59 (lsb first)
  uint8_t        P0 : 1;
  uint8_t       dst : 2; // * 00 - no DST, 11 - DST
  uint8_t       lsw : 1; //
  uint8_t       lyi : 1; // * 
  uint8_t        U4 : 1;
  uint16_t     year : 9; // * P5 marker at b4
  uint8_t        U3 : 1;
  uint8_t dut1value : 4; //
  //b40
  //-----------------
  //b39
  uint8_t        P4 : 1;
  uint8_t      dut1 : 3; // * = 2 (-ve) or 5 (+ve)
  uint8_t        U2 : 2;
  uint16_t     doty : 12; // * P3 marker at b4
  uint8_t        U1 : 2;
  //b20
  //-----------------
  //b19
  uint8_t        P2 : 1;
  uint8_t     hours : 7; // * 
  uint8_t        U0 : 2;
  uint8_t        P1 : 1;
  uint8_t   minutes : 8; // * 
  uint8_t         M : 1;
  //b0
  //-----------------
  //uint8_t   _unused : 4; // 4b packing to make 64b total
};
#else
struct timecode_frame{
  //-----------------
  //b59 (lsb first)
  uint8_t        P0 : 1;
  uint8_t        U3 : 4;
  uint8_t       ls2 : 1; // * 
  uint8_t       ls1 : 1; // * 
  uint8_t      dotw : 3; // * 
  uint8_t        P5 : 1;
  uint8_t      year : 8; // * 
  uint8_t       SU2 : 1;
  //b40
  //-----------------
  //b39
  uint8_t        P4 : 1;
  uint8_t       SU1 : 1;
  uint8_t       pa2 : 1; // * 
  uint8_t       pa1 : 1; // * 
  uint8_t        U2 : 2;
  uint16_t     doty : 12; // * P3 marker at b4
  uint8_t        U1 : 2;
  //b20
  //-----------------
  //b19
  uint8_t        P2 : 1;
  uint8_t     hours : 7; // * 
  uint8_t        U0 : 2;
  uint8_t        P1 : 1;
  uint8_t   minutes : 8; // * 
  uint8_t         M : 1;
  //b0
  //-----------------
  //uint8_t   _unused : 4; // 4b packing to make 64b total
};
#endif

#pragma pack(pop) // back to default bitpacking

class bool_buffer_64b{
  public: 
    uint8_t operator[](uint8_t i)
		{
			return (uint8_t)((_buffer >> (59 - i)) & 0x01);
		}
    
    void operator=(const uint64_t r)
		{
			_buffer = r;
		}
  private:
    uint64_t _buffer;
};

union wwvb_jjy_timecode{
  timecode_frame timecode;
  bool_buffer_64b buffer;
};

#endif
