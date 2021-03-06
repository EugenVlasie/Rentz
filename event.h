#ifndef EVENT_H
#define EVENT_H
#include <limits.h>
#include <vector>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include "util/debug.h"

const uint8_t FLAG = 0x7E;
const uint8_t ESC = 0x7D;
const uint8_t MAGIC = 0x20;

struct event {
#define append(b) \
      if(b == FLAG || b == ESC) {\
        message.push_back(ESC);\
        message.push_back(b ^ MAGIC);\
      } else {\
        message.push_back(b);\
      }

  enum EType {
    sendCards,
    sendHand,
    //both of these events include an array of unsigned bytes
    //of codified cards

    sendScores,
    //the scores are encoded as 4 byte integers

    getGameChoice,
    sendGameChoice,
    //game choice is represented as an unsigned byte

    getCardChoice,
    //card choice is 1 unsigned byte codified card

    getNVChoice,
    //NV choice is a 1 byte boolean

    sendIndex,
    //the index is a 4 byte unsigned integer
    
    requestName,

    null
  };

  uint32_t type;
  size_t len;
  uint8_t *data;

  uint32_t getInt(uint8_t * ptr) {
    uint32_t ret = *((uint32_t *)ptr);
    return ntohl(ret);
  }

  void send(int fd) {
    std::vector<uint8_t> header(8, 0);
    ((uint32_t *)header.data())[0] = htonl(type);
    ((uint32_t *)header.data())[1] = htonl(len);

    std::vector<uint8_t> message;
    message.reserve(8 + len + 2);
    message.push_back(FLAG);

    for(auto &b: header) {
      append(b);
    }

    for(size_t i = 0; i < len; ++i) {
      uint8_t b = data[i];
      append(b);
    }

    message.push_back(FLAG);

    int rem = message.size();
    uint8_t *at = message.data();

    while(rem > 0) {
      const int n = ::send(fd, at, rem, 0);
      debug("Sent %d bytes\n", n);
      rem -= n;
      at += n;
    }
  }

  void init(uint8_t *ptr) {
    assert(ptr[0] == FLAG);
    //first FLAG should mark the frame's beginning
    
    ++ptr;

    type = getInt(ptr);
    ptr += 4;
    len = getInt(ptr);
    ptr += 4;

    data = new uint8_t[len];
    uint8_t * at = (uint8_t *)ptr;

    for(size_t i = 0; i < len; ++i) {
      data[i] = *at;
      ++at;
    }

    assert(*at == FLAG);
    //last FLAG should mark the frame's end
  }

  void free() {
      delete[] data;
  }
};

#endif
